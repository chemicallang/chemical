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

        // production handler: parse request, route, and respond
        func handle_conn(&self, s: net::Socket) {
            if (s == 0u || (s as longlong) < 0) {
                printf("handle_conn: invalid socket {}\n");
                net::close_socket(s); return;
            }

            var buf = io.Buffer();
            var req_opt = http.read_request_incremental(s, buf, self.cfg.header_timeout_secs, self.cfg.max_header_bytes, self.cfg.max_headers);
            if (req_opt is std::Option.None) {
                // printf("handle_conn: read_request_incremental -> None for socket=%d\n", s);
                net::close_socket(s); return;
            }
            // printf("handle_conn: parsed request for socket=%d\n", s);
            var Some(req) = req_opt else unreachable;

            // do NOT read the body yet — create a lazy Body and give it to the handler via req.body_source
            var body_len: isize = -1;
            var chunked = false;
            // set body_len from req.body_len (was parsed from Content-Length), else -1
            if(req.body_len > 0u) { body_len = req.body_len as isize; }
            // detect chunked
            var te_opt = req.headers.get("Transfer-Encoding");
            if(te_opt is std::Option.Some) {
                var Some(te) = te_opt else unreachable;
                if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
            }

            // attach body source to request (we avoid copying the buffer: pass pointer)
            req.body = http.Body.make_body(s, &mut buf as *mut io::Buffer, body_len, chunked, self.cfg.header_timeout_secs * 4, self.cfg.max_body_bytes);

            // printf("handle_conn: method='%s' path='%s'\n", req.method.data(), req.path.data());

            var params = std::vector<std::pair<std::string,std::string>>();
            var route = self.router.match_route(req.method, req.path, &mut params);

            var resw = http.ResponseWriter(s);

            // printf("handle_conn: total headers in response writer = %d\n", resw.headers.headers.size());

            if (route != null) {
                // printf("handle_conn: invoking handler for socket=%d\n", s);

                // call the handler with the actual Request object
                route.handler(req_opt.take(), resw);

                // printf("handle_conn: handler returned for socket=%d\n", s);
            } else {
                // printf("handle_conn: no handler matched, sending 404 for socket=%d\n", s);
                resw.status = 404u;
                resw.set_header(std::string::make_no_len("Content-Type"), std::string::make_no_len("text/plain; charset=utf-8"));
                resw.write_string(std::string::make_no_len("Not Found\n"));
            }

            net::close_socket(s);
            // printf("handle_conn: closed socket=%d\n", s);
        }

        // accept loop — submit work to threadpool
        // TEMP: accept inline to isolate threadpool / closure issues
        func accept_main(arg : *void) : *void {
            var S = arg as *mut Server;
            while (S.run) {
                var s = net.accept_socket(S.listen_sock);
                if (s == 0u || (s as longlong) < 0) {
                    std.concurrent.sleep_ms(1u);
                    continue;
                }

                // Set Keep-Alive
                net::set_keep_alive(s, true);

                // submit work to threadpool
                S.pool.submit_void(|S, s|() => {
                    S.handle_conn(s);
                });
            }
            // printf("accept_main: exiting\n");
            return null;
        }

        func start(&mut self, port : uint = 8080u) {
            // parse addr into host and port
            // acceptor will call net.listen_addr
            self.listen_sock = net.listen_addr("0.0.0.0", port);
            self.run = true;
        }

        func serve_non_iocp(&mut self, port : uint = 8080u) {
            start(port);
            accept_main(&mut self as *void);
        }

        func serve_async(&mut self, port : uint = 8080u) : std.concurrent.Thread {
            start(port);
            return std.concurrent.spawn(accept_main, &mut self as *void);
        }

        func serve(&mut self, port : uint = 8080u) {
            comptime if(def.windows) {
                start(port);
                // Create Completion Port
                var cp = malloc(sizeof(net.iocp.CompletionPort)) as *mut net.iocp.CompletionPort;
                new (cp) net.iocp.CompletionPort(self.cfg.worker_count);

                // Register workers within logic (submit to standard pool or spawn new)
                var i = 0u;
                while(i < self.cfg.worker_count) {
                    self.pool.submit_void(|cp|() => {
                        while(true) { cp.poll(500); }
                    });
                    i = i + 1u;
                }

                printf("Server running with IOCP on port %d\n", port);

                // Accept loop
                while (self.run) {
                    var s = net.accept_socket(self.listen_sock);
                    if (s == 0u || (s as longlong) < 0) {
                         std.concurrent.sleep_ms(1u);
                         continue;
                    }

                    net.set_keep_alive(s, true);

                    if(cp.register(s, s as usize)) {
                         // Allocate context and buffer
                         var buf_sz = 4096u;
                         var buf = malloc(buf_sz) as *mut char;
                         // AsyncContext on heap
                         var ctx = malloc(sizeof(net.iocp.AsyncContext)) as *mut net.iocp.AsyncContext;
                         new (ctx) net.iocp.AsyncContext(buf, buf_sz);

                         ctx.callback = |s, self, buf_sz|(ctx, bytes, ok) => {
                             if(!ok || bytes == 0) {
                                 net.close_socket(s);
                                 free(ctx.buffer.buf);
                                 delete ctx;
                                 return;
                             }

                             // Append read bytes to accumulated buffer (manual vector push)
                             var input_ptr = ctx.buffer.buf as *u8;
                             var k = 0u;
                             while(k < bytes) {
                                 ctx.accumulated.push_back(input_ptr[k]);
                                 k = k + 1u;
                             }

                             // Check for headers end in accumulated (starting from read_pos)
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
                                 // Not full headers yet
                                 if (abuf.size() > 65536) { // Max header size check
                                     net.close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                                 }
                                 // Read more
                                 ctx.overlapped.Internal = 0;
                                 ctx.overlapped.InternalHigh = 0;
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                                 net.iocp.async_recv(s, ctx);
                                 return;
                             }

                             // Headers complete: parse
                             // parse_request_from_bytes expects raw pointer to start of headers
                             // In our flattened vector, headers start at 0 (since we clear after request)
                             var ptr = abuf.data();

                             var req_opt = http.parse_request_from_bytes(ptr, crlfpos, s);

                             if (req_opt is std::Option.None) {
                                 net.close_socket(s); free(ctx.buffer.buf); delete ctx; return;
                             }

                             var Some(req) = req_opt else unreachable;

                             // Prepare Body
                             var body_len: isize = -1;
                             var chunked = false;
                             if(req.body_len > 0u) { body_len = req.body_len as isize; }
                             var te_opt = req.headers.get("Transfer-Encoding");
                             if(te_opt is std::Option.Some) {
                                var Some(te) = te_opt else unreachable;
                                if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
                             }

                             // To support Body reading from leftovers, we pass a temporary io::Buffer containing leftovers.
                             // Leftovers are bytes from crlfpos to end of abuf.
                             var leftovers_sz = abuf.size() - crlfpos;

                             // Construct temporary io::Buffer on heap to pass to Body
                             var body_buf = new io::Buffer();
                             if(leftovers_sz > 0) {
                                 body_buf.append_bytes((ptr + crlfpos) as *u8, leftovers_sz);
                             }

                             // req.body needs *mut io::Buffer
                             req.body = http.Body.make_body(s, body_buf, body_len, chunked, 30, 1024u*1024u*10u);

                             // Route
                             var params = std::vector<std::pair<std::string,std::string>>();
                             var route = self.router.match_route(req.method, req.path, &mut params);
                             var resw = http.ResponseWriter(s);

                             if (route != null) {
                                 route.handler(req_opt.take(), resw);
                             } else {
                                 resw.status = 404u;
                                 resw.set_header(std::string::make_no_len("Content-Type"), std::string::make_no_len("text/plain; charset=utf-8"));
                                 resw.write_string(std::string::make_no_len("Not Found\n"));
                             }

                             // Cleanup body buffer
                             delete body_buf;

                             // Handler finished
                             // Simple Keep-Alive check
                             var ka = true;
                             var c_opt = req.headers.get("Connection");
                             if(c_opt is std::Option.Some) {
                                 var Some(cv) = c_opt else unreachable;
                                 if(cv.equals_with_len("close", 5)) { ka = false; }
                             }

                             if(ka) {
                                 // Reset context for next request
                                 ctx.accumulated.clear();
                                 ctx.read_pos = 0u;

                                 ctx.overlapped.Internal = 0;
                                 ctx.overlapped.InternalHigh = 0;
                                 ctx.overlapped.Offset = 0;
                                 ctx.overlapped.OffsetHigh = 0;
                                 net.iocp.async_recv(s, ctx);
                             } else {
                                 net.close_socket(s);
                                 free(ctx.buffer.buf);
                                 delete ctx;
                             }
                         }

                         net.iocp.async_recv(s, ctx);
                    } else {
                        net.close_socket(s);
                    }
                }
            } else {
                serve_non_iocp(port);
            }
        }

        func shutdown(&mut self) {
            run = false;
            net.close_socket(listen_sock);
        }
    }
}