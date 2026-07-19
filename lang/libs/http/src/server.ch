// ===== Server wiring all pieces together =====
public namespace server {
    public struct ServerConfig {
        var addr: std::string;
        var worker_count: uint;
        var header_timeout_secs: long;
        var max_header_bytes: usize;
        var max_headers: uint;
        var max_body_bytes: usize;
        @constructor func constructor() {
            return ServerConfig {
                addr = std::string::make_no_len(":8080");
                worker_count = std.concurrent.hardware_threads() as uint;
                header_timeout_secs = 5;
                max_header_bytes = 64u * 1024u;
                max_headers = 512u;
                max_body_bytes = 10u * 1024u * 1024u
            }
        }
    }

    public struct Server {
        var cfg: ServerConfig;
        var pool: std.concurrent.ThreadPool;
        var listen_sock: net::Socket;
        var router: web.Router;
        var run: bool;

        @constructor func constructor(cfg_: ServerConfig) {
            const count = cfg_.worker_count
            return Server {
                cfg = cfg_;
                pool : std.concurrent.create_pool(count);
                listen_sock = 0u;
                router = web.Router();
                run = false;
            }
        }

        func handle_conn(&self, s: net::Socket) {
            if (s == 0u || (s as longlong) < 0) {
                printf("handle_conn: invalid socket {}\n");
                net::close_socket(s); return;
            }

            var buf = net::Buffer();
            var req_opt = http::read_request_incremental(s, &mut buf, self.cfg.header_timeout_secs, self.cfg.max_header_bytes, self.cfg.max_headers);
            if (req_opt is std::Option.None) {
                net::close_socket(s); return;
            }
            var Some(req) = req_opt else unreachable;

            var body_len: isize = -1;
            var chunked = false;
            if(req.body_len > 0u) { body_len = req.body_len as isize; }
            var te_opt = req.headers.get("Transfer-Encoding");
            if(te_opt is std::Option.Some) {
                var Some(te) = te_opt else unreachable;
                if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
            }

            req.body = http::Body.make_body(s, &mut buf as *mut net::Buffer, body_len, chunked, self.cfg.header_timeout_secs * 4, self.cfg.max_body_bytes);

            var params = std::vector<std::pair<std::string,std::string>>();
            var route = self.router.match_route(&req.method, &req.path, &raw mut params);

            var resw = http::ResponseWriter(s, &req.method);

            if (route != null) {
                route.handler(req_opt.take(), resw);
            } else {
                resw.status = 404u;
                resw.set_header(std::string::make_no_len("Content-Type"), std::string::make_no_len("text/plain; charset=utf-8"));
                resw.write_string(std::string::make_no_len("Not Found\n"));
            }

            net::close_socket(s);
        }

        func accept_main(arg : *void) : *void {
            var S = arg as *mut Server;
            while (S.run) {
                var s = net::accept_socket(S.listen_sock);
                if (s == 0u || (s as longlong) < 0) {
                    if (!S.run) { break; }
                    std.concurrent.sleep_ms(10u);
                    continue;
                }

                net::set_keep_alive(s, true);

                S.pool.submit_void(|S, s|() => {
                    S.handle_conn(s);
                });
            }
            net::close_socket(S.listen_sock);
            S.listen_sock = 0u;
            return null;
        }

        func start(&mut self, default_port : uint = 8080u) {
            var host : *char = null
            var port = default_port

            var pos = self.cfg.addr.find(":")
            var host_str = std::string()
            if (pos != -1u) {
                if (pos > 0u) {
                    host_str = self.cfg.addr.substring(0u, pos)
                    host = host_str.data()
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

            self.listen_sock = net::listen_addr(host, port);
            comptime if(def.windows) {
                net::set_nonblocking(self.listen_sock);
            } else {
                net::set_recv_timeout(self.listen_sock, 0, 100000);
            }
            self.run = true;
        }

        func serve_non_iocp(&mut self, port : uint = 8080u) {
            start(port);
            accept_main(&raw mut self);
        }

        func serve_async(&mut self, port : uint = 8080u) : std.concurrent.Thread {
            start(port);
            return std.concurrent.spawn(accept_main, &raw mut self);
        }

        func serve(&mut self, port : uint = 8080u) {
            comptime if(def.windows) {
                start(port);
                var cp = malloc(sizeof(net::iocp::CompletionPort)) as *mut net::iocp::CompletionPort;
                new (cp) net::iocp::CompletionPort(self.cfg.worker_count);
                var i = 0u;
                while(i < self.cfg.worker_count) {
                    self.pool.submit_void(|cp|() => {
                        while(true) { cp.poll(500); }
                    });
                    i = i + 1u;
                }
                printf("Server running with IOCP on %s\n", self.cfg.addr.data());
                while (self.run) {
                    var s = net::accept_socket(self.listen_sock);
                    if (s == 0u || (s as longlong) < 0) {
                         std.concurrent.sleep_ms(1u);
                         continue;
                    }
                    net::set_keep_alive(s, true);
                    if(cp.register(s, s as usize)) {
                         var buf_sz = 4096u;
                         var buf = malloc(buf_sz) as *mut char;
                         var ctx = malloc(sizeof(net::iocp::AsyncContext)) as *mut net::iocp::AsyncContext;
                         new (ctx) net::iocp::AsyncContext(buf, buf_sz);
                         ctx.callback = |s, self, buf_sz|(ctx, bytes, ok) => {
                             if(!ok || bytes == 0) {
                                 net::close_socket(s);
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
                                     net::close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                                 }
                                 ctx.overlapped.Internal = 0;
                                 ctx.overlapped.InternalHigh = 0;
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                                 net::iocp::async_recv(s, ctx);
                                 return;
                             }
                             var ptr = abuf.data();
                             var req_opt = http::parse_request_from_bytes(ptr, crlfpos, s);
                             if (req_opt is std::Option.None) {
                                 net::close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                             }
                             var Some(req) = req_opt else unreachable;
                             var body_len: isize = -1;
                             var chunked = false;
                             if(req.body_len > 0u) { body_len = req.body_len as isize; }
                             var te_opt = req.headers.get("Transfer-Encoding");
                             if(te_opt is std::Option.Some) {
                                var Some(te) = te_opt else unreachable;
                                if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
                             }
                             var leftovers_sz = abuf.size() - crlfpos;
                             var body_buf = new net::Buffer();
                             if(leftovers_sz > 0) {
                                 body_buf.append_bytes((ptr + crlfpos) as *u8, leftovers_sz);
                             }
                             req.body = http::Body.make_body(s, body_buf, body_len, chunked, 30, 1024u*1024u*10u);
                             var params = std::vector<std::pair<std::string,std::string>>();
                             var route = self.router.match_route(&req.method, &req.path, &raw mut params);
                             var resw = http::ResponseWriter(s, req.method.copy());
                             if (route != null) {
                                 route.handler(req_opt.take(), resw);
                             } else {
                                 resw.status = 404u;
                                 resw.set_header(std::string::make_no_len("Content-Type"), std::string::make_no_len("text/plain; charset=utf-8"));
                                 resw.write_string(std::string::make_no_len("Not Found\n"));
                             }
                             delete body_buf;
                             var ka = true;
                             var c_opt = req.headers.get("Connection");
                             if(c_opt is std::Option.Some) {
                                 var Some(cv) = c_opt else unreachable;
                                 if(cv.equals_with_len("close", 5)) { ka = false; }
                             }
                             if(ka) {
                                 ctx.accumulated.clear();
                                 ctx.read_pos = 0u;
                                 ctx.overlapped.Internal = 0;
                                 ctx.overlapped.InternalHigh = 0;
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                                 net::iocp::async_recv(s, ctx);
                             } else {
                                 net::close_socket(s);
                                 free(ctx.buffer.buf);
                                 delete ctx;
                             }
                         }
                         net::iocp::async_recv(s, ctx);
                    } else {
                        net::close_socket(s);
                    }
                }
            } else {
                serve_non_iocp(port);
            }
        }

        func shutdown(&mut self) {
            run = false;
        }
    }
}
