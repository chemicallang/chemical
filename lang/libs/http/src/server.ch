// ===== Server wiring all pieces together =====
public namespace server {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

public struct ServerConfig {
    var addr: string;
    var worker_count: uint;
    var header_timeout_secs: long;
    var max_header_bytes: usize;
    var max_headers: uint;
    var max_body_bytes: usize;
    var cert_file: string;
    var key_file: string;
    @constructor func constructor() {
        return ServerConfig {
            addr = http::http_lit(":8080");
            worker_count = std.concurrent.hardware_threads() as uint;
            header_timeout_secs = 5;
            max_header_bytes = 64u * 1024u;
            max_headers = 512u;
            max_body_bytes = 10u * 1024u * 1024u,
            cert_file = string(),
            key_file = string()
        }
    }
}

public struct Server {
    internal var cfg: ServerConfig;
    internal var pool: std.concurrent.ThreadPool;
    internal var listener: net::TcpListener;
    var router: web.Router;
    var run: bool;
    internal var tls_chain: tls::CertificateChain;
    internal var tls_key: tls::PrivateKey;
    internal var tls_enabled: bool;

    @constructor func constructor(cfg_: ServerConfig) {
        const count = cfg_.worker_count
        return Server {
            cfg = cfg_;
            pool : std.concurrent.create_pool(count);
            listener = net::TcpListener();
            router = web.Router();
            run = false,
            tls_chain = tls::CertificateChain(),
            tls_key = tls::PrivateKey(),
            tls_enabled = false
        }
    }

    func handle_conn(&self, tcp : net::TcpStream) { unsafe {
        var transport_box : *mut http::Transport = null
        if(self.tls_enabled) {
            var scfg = tls::server_config(&self.tls_chain, &self.tls_key)
            var accepted = tls::accept(tcp, &scfg)
            if(accepted is Result.Err) {
                return
            }
            var Ok(tls_stream) = accepted else unsafe { unreachable }
            transport_box = http::transport_box_tls_server(tls::TlsServerStream.into_box(&mut tls_stream))
        } else {
            var fd = net::raw::release_tcp(&mut tcp)
            transport_box = http::transport_box_plain(fd)
        }

        var buf = net::Buffer();
        var req_opt = http::read_request_head(&mut *transport_box, &raw mut buf,
                                               self.cfg.header_timeout_secs,
                                               self.cfg.max_header_bytes,
                                               self.cfg.max_headers);
        if(req_opt is Result.Err) {
            var _ = transport_box.shutdown()
            return
        }
        var Ok(req_ptr) = req_opt else unsafe { unreachable }
        var req = &mut *req_ptr
        var body_len: isize = 0;
        var chunked = false;
        if(req.body_len > 0u) { body_len = req.body_len as isize; }
        var te_opt = req.headers.get("Transfer-Encoding");
        if(te_opt is Option.Some) {
            var Some(te) = te_opt else unsafe { unreachable };
            if(te.equals(string_view("chunked"))) { chunked = true; body_len = -1; }
        }

        std::replace(&mut req.body, http::Body.make_borrowed(transport_box, &raw mut buf, body_len, chunked,
                                             self.cfg.header_timeout_secs * 4,
                                             self.cfg.max_body_bytes));

        var params = std.vector<std::pair<string, string>>();
        var route = self.router.match_route(&req.method, &req.path, &raw mut params);

        var resw = http::ResponseWriter(transport_box, &req.method);

        if (route != null) {
            std::replace(&mut req.route_params, params);
            route.handler(req, &mut resw);
        } else {
            resw.status = 404u;
            resw.set_header(http::http_lit("Content-Type"), http::http_lit("text/plain; charset=utf-8"));
            resw.write_string(http::http_lit("Not Found\n"));
        }

        var _ = transport_box.shutdown()
        delete transport_box
        delete req_ptr
    }}

    func accept_main(arg : *void) : *void { unsafe {
        var S = arg as *mut Server;
        while (S.run) {
            var accepted = S.listener.accept();
            if (accepted is Result.Err) {
                if (!S.run) { break; }
                std.concurrent.sleep_ms(10u);
                continue;
            }
            var Ok(tcp) = accepted else unsafe { unreachable }
            tcp.set_keep_alive(true);
            var fd = unsafe(net::raw::release_tcp(&mut tcp))
            var s = S
            unsafe(S.pool.submit_void(|s, fd|() => {
                var conn = net::raw::stream_from_fd(fd)
                s.handle_conn(conn);
            }));
        }
        S.listener.close()
        return null;
    }}

    func start(&mut self, default_port : uint = 8080u) {
        var host = string()
        var port = default_port

        var pos = self.cfg.addr.find(":")
        if (pos != std::NPOS) {
            if (pos > 0u) {
                host = self.cfg.addr.substring(0u, pos)
            }
            var pstr = self.cfg.addr.substring(pos + 1u, self.cfg.addr.size())
            if (!pstr.empty()) {
                var pval = 0u
                for(var i=0u; i<pstr.size(); i++) {
                    var c = pstr.get(i)
                    if(c >= '0' && c <= '9') { pval = pval * 10u + (c as uint - '0' as uint) }
                }
                if (pval > 0u) port = pval
            }
        }
        if(host.empty()) { host = http::http_lit("0.0.0.0") }

        var host_str = std::str.make_view(host.to_view())
        var listener_r = net::TcpListener.bind(host_str, port as u16)
        if(listener_r is Result.Err) {
            self.run = false
            return
        }
        var Ok(l) = listener_r else unsafe { unreachable }
        self.listener = std::replace(&mut l, net::TcpListener())
        comptime if(def.windows) {
            self.listener.set_nonblocking()
        } else {
            self.listener.set_recv_timeout_ms(100u)
        }

        if(!self.cfg.cert_file.empty() && !self.cfg.key_file.empty()) {
            var cert_path = std::str.make_view(self.cfg.cert_file.to_view())
            var key_path = std::str.make_view(self.cfg.key_file.to_view())
            var cert = tls::load_certificate(cert_path)
            var key = tls::load_rsa_private_key(key_path)
            if(cert is Result.Err || key is Result.Err) {
                self.tls_enabled = false
            } else {
                var Ok(chain) = cert else unsafe { unreachable }
                var Ok(private_key) = key else unsafe { unreachable }
                self.tls_chain = std::replace(&mut chain, tls::CertificateChain())
                self.tls_key = std::replace(&mut private_key, tls::PrivateKey())
                self.tls_enabled = true
            }
        }

        self.run = true;
    }

    func serve_non_iocp(&mut self, port : uint = 8080u) {
        start(port);
        accept_main(&raw mut self);
    }

    func serve_async(&mut self, port : uint = 8080u) : std.concurrent.Thread {
        start(port);
        return unsafe(std.concurrent.spawn(accept_main, &raw mut self));
    }

    func serve(&mut self, port : uint = 8080u) {
        comptime if(def.windows) {
            unsafe {
            start(port);
            var cp = unsafe(malloc(sizeof(net::iocp::CompletionPort))) as *mut net::iocp::CompletionPort;
            if(cp == null) { panic("http server: failed to allocate completion port") }
            unsafe { new (cp) net::iocp::CompletionPort(self.cfg.worker_count) };
            var i = 0u;
            while(i < self.cfg.worker_count) {
                unsafe(self.pool.submit_void(|cp|() => {
                    while(true) { cp.poll(500); }
                }));
                i = i + 1u;
            }
            printf("Server running with IOCP on %s\n", self.cfg.addr.data());
            var listen_fd = net::raw::listener_fd(&self.listener)
            while (self.run) {
                var s = net::raw::accept_socket(listen_fd);
                if (s == 0u || (s as longlong) < 0) {
                     std.concurrent.sleep_ms(1u);
                     continue;
                }
                net::raw::set_keep_alive(s, true);
                if(cp.register(s, s as usize)) {
                     var buf_sz = 4096u;
                     var buf = malloc(buf_sz) as *mut char;
                     var ctx = malloc(sizeof(net::iocp::AsyncContext)) as *mut net::iocp::AsyncContext;
                     if(buf == null || ctx == null) {
                         if(buf != null) { free(buf) }
                         if(ctx != null) { free(ctx) }
                         net::raw::close_socket(s);
                         continue;
                     }
                     new (ctx) net::iocp::AsyncContext(buf, buf_sz);
                     ctx.callback = |s, self, buf_sz|(ctx, bytes, ok) => {
                         if(!ok || bytes == 0) {
                             net::raw::close_socket(s);
                             free(ctx.buffer.buf);
                             delete ctx;
                             return;
                         }
                         var input_ptr = ctx.buffer.buf as *u8;
                         var k = 0u;
                         while(k < bytes) {
                             ctx.accumulated.push_back(input_ptr[k]);
                             k = k + 1u;
                         }
                         var found = false;
                         var crlfpos = 0u;
                         var j = ctx.read_pos;
                         var abuf = &mut ctx.accumulated;
                         if(abuf.size() >= 4) {
                              while(j + 3 < abuf.size()) {
                                  if(abuf.get(j) == '\r' as u8 && abuf.get(j+1u) == '\n' as u8 &&
                                     abuf.get(j+2u) == '\r' as u8 && abuf.get(j+3u) == '\n' as u8) {
                                      found = true;
                                      crlfpos = j + 4u;
                                      break;
                                  }
                                  j++;
                              }
                         }
                         if (!found) {
                             if (abuf.size() > 65536) {
                                 net::raw::close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                             }
                             ctx.overlapped.Internal = 0;
                             ctx.overlapped.InternalHigh = 0;
                             unsafe {
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                             }
                             net::iocp::async_recv(s, ctx);
                             return;
                         }
                         var ptr = abuf.data();
                         var req_opt = http::parse_request_from_bytes(ptr, crlfpos);
                         if (req_opt is Option.None) {
                             net::raw::close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                         }
                         var req = req_opt.take_unchecked();
                         var body_len: isize = 0;
                         var chunked = false;
                         if(req.body_len > 0u) { body_len = req.body_len as isize; }
                         var te_opt = req.headers.get("Transfer-Encoding");
                         if(te_opt is Option.Some) {
                            var Some(te) = te_opt else unsafe { unreachable };
                            if(te.equals(string_view("chunked"))) { chunked = true; body_len = -1; }
                         }
                         var leftovers_sz = abuf.size() - crlfpos;
                         var body_buf = new net::Buffer();
                         if(leftovers_sz > 0) {
                             body_buf.append_bytes((ptr + crlfpos) as *u8, leftovers_sz);
                         }
                         var transport_box = http::transport_box_plain(s)
                         std::replace(&mut req.body, http::Body.make_borrowed(transport_box, body_buf, body_len, chunked, 30, 1024u*1024u*10u));
                         var params = std.vector<std::pair<string, string>>();
                         var route = self.router.match_route(&req.method, &req.path, &raw mut params);
                         var resw = http::ResponseWriter(transport_box, &req.method);
                         if (route != null) {
                             std::replace(&mut req.route_params, params);
                             route.handler(req, &mut resw);
                         } else {
                             resw.status = 404u;
                             resw.set_header(http::http_lit("Content-Type"), http::http_lit("text/plain; charset=utf-8"));
                             resw.write_string(http::http_lit("Not Found\n"));
                         }
                         delete body_buf;
                         var ka = true;
                         var c_opt = req.headers.get("Connection");
                         if(c_opt is Option.Some) {
                             var Some(cv) = c_opt else unsafe { unreachable };
                             if(cv.equals(string_view("close"))) { ka = false; }
                         }
                         // The Transport owns `s`; detach it before the IOCP
                         // path takes over the descriptor again.
                         var _ = transport_box.detach()
                         if(ka) {
                             ctx.accumulated.clear();
                             ctx.read_pos = 0u;
                             ctx.overlapped.Internal = 0;
                             ctx.overlapped.InternalHigh = 0;
                             unsafe {
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                             }
                             net::iocp::async_recv(s, ctx);
                         } else {
                             net::raw::close_socket(s);
                             free(ctx.buffer.buf);
                             delete ctx;
                         }
                     };
                     net::iocp::async_recv(s, ctx);
                } else {
                    net::raw::close_socket(s);
                }
            }
            }
        } else {
            serve_non_iocp(port);
        }
    }

    func shutdown(&mut self) {
        run = false;
        self.listener.close()
    }
}

} // namespace server
