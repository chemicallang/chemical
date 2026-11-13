// ===== Server =====
//
// One request pipeline, three drivers. `serve_ready` handles exactly one HTTP
// message on an already-owned `Stream`; the blocking accept loop, the Windows
// IOCP completion loop and the coroutine loop all call it. Before this, each
// driver had its own copy of the request/response code and they had drifted.
public namespace server {

using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;

public struct ServerConfig {
    /// `host:port` (an IPv6 literal must be bracketed: `[::1]:8080`).
    var addr: string;
    var worker_count: uint;
    var header_timeout_secs: long;
    /// Deadline for reading a complete request body. Independent of
    /// `header_timeout_secs` because a body may be large and slow, while a head
    /// must arrive quickly. A peer that stalls longer is rejected with `400`.
    var body_timeout_secs: long;
    var max_header_bytes: usize;
    var max_headers: uint;
    var max_body_bytes: usize;
    var cert_file: string;
    var key_file: string;

    @constructor func constructor() {
        return ServerConfig {
            addr = http::http_lit("0.0.0.0:8080");
            worker_count = std::concurrent.hardware_threads() as uint;
            header_timeout_secs = 5;
            body_timeout_secs = 30;
            max_header_bytes = 64u * 1024u;
            max_headers = 512u;
            max_body_bytes = 10u * 1024u * 1024u;
            cert_file = string();
            key_file = string();
        }
    }
}

/// Stop flag shared between a serving `Server` and its `ShutdownHandle`. It is
/// an `Arc` so the handle stays valid after `serve`/`serve_async` consumes the
/// server, and so the flag can be set from any thread.
public struct ShutdownHandle {
    internal var flag : std::Arc<std::Mutex<bool>>;

    public func shutdown(&self) {
        var g = self.flag.get().lock()
        var on = g.get()
        *on = true;
    }

    public func is_stopping(&self) : bool {
        var g = self.flag.get().lock()
        var on = g.get()
        return *on;
    }
}

public struct Server {
    internal var cfg: ServerConfig;
    internal var pool: std::concurrent.ThreadPool;
    internal var listener: net::TcpListener;
    /// The route table. Mutating it requires an owned `Server`, which only
    /// exists before `serve`/`serve_async` consumes it, so registering routes
    /// during serving is impossible by construction.
    var router: web.Router;
    internal var stopping: std::Arc<std::Mutex<bool>>;
    internal var tls_chain: tls::CertificateChain;
    internal var tls_key: tls::PrivateKey;
    internal var tls_enabled: bool;
    internal var bound: bool;
    internal var local: net::SocketAddr;

    @constructor func constructor(cfg_: ServerConfig) {
        const count = cfg_.worker_count
        return Server {
            cfg = cfg_;
            pool : std::concurrent.create_pool(count);
            listener = net::TcpListener();
            router = web.Router();
            stopping : std::arc(std::new_mutex(false)),
            tls_chain = tls::CertificateChain();
            tls_key = tls::PrivateKey();
            tls_enabled = false;
            bound = false;
            local = net::SocketAddr();
        }
    }

    /// Bind the configured address now, so a failure is reported at the call
    /// site instead of being swallowed inside `serve`. The bound address is
    /// available from `local_addr` (useful with port 0).
    public func bind(cfg : ServerConfig) : Result<Server, net::NetError> {
        var s = Server(cfg)
        var r = s.bind_socket()
        if(r is Result.Err) {
            return Result.Err(net::NetError.Kind(std::io::IoErrorKind::AddrInUse()))
        }
        return Result.Ok(s)
    }

    /// The address actually bound (the ephemeral port when `addr` used port 0).
    public func local_addr(&self) : Result<net::SocketAddr, net::NetError> {
        if(!self.bound) {
            return Result.Err(net::NetError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var out = net::SocketAddr()
        out.ip.append_string(&self.local.ip)
        out.port = self.local.port
        return Result.Ok(out)
    }

    /// A handle that can stop a serving server from another thread. Take it
    /// before `serve`/`serve_async` consumes the server.
    public func handle(&self) : ShutdownHandle {
        return ShutdownHandle { flag : self.stopping.clone() }
    }

    public func is_running(&self) : bool {
        var g = self.stopping.get().lock()
        var on = g.get()
        return !*on;
    }

    // -----------------------------------------------------------------------
    // Binding
    // -----------------------------------------------------------------------

    func bind_socket(&mut self) : Result<Unit, net::NetError> {
        var want = parse_host_port(self.cfg.addr.to_view(), 8080u)
        if(want is Option.None) {
            return Result.Err(net::NetError.Kind(std::io::IoErrorKind::InvalidInput()))
        }
        var Some(target) = want else unsafe { unreachable }
        var host = std::str.make_view(target.ip.to_view())
        var bound = net::TcpListener.bind(host, target.port)
        if(bound is Result.Err) {
            return Result.Err(net::NetError.Kind(std::io::IoErrorKind::AddrInUse()))
        }
        var Ok(l) = bound else unsafe { unreachable }
        self.listener = std::replace(&mut l, net::TcpListener())

        // Poll mode: the accept loop must be able to notice `shutdown()`.
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

        var bound_addr = self.listener.local_addr()
        if(bound_addr is Result.Ok) {
            var Ok(addr) = bound_addr else unsafe { unreachable }
            self.local = std::replace(&mut addr, net::SocketAddr())
        }
        self.bound = true
        return Result.Ok(Unit { })
    }

    // -----------------------------------------------------------------------
    // Message pipeline
    // -----------------------------------------------------------------------

    /// Handle exactly one message on `stream`. Returns whether the connection
    /// should stay open for another message.
    func serve_ready(&self, stream : &mut http::Stream, buf : &mut net::Buffer) : bool {
        var head = http::read_request_head(stream, buf,
                                           self.cfg.header_timeout_secs,
                                           self.cfg.max_header_bytes,
                                           self.cfg.max_headers)
        if(head is Result.Err) {

            return false
        }
        var Ok(req) = head else unsafe { unreachable }

        var body_r = http::read_request_body(stream, buf, &mut req,
                                             self.cfg.max_body_bytes,
                                             (self.cfg.body_timeout_secs * 1000) as u64)
        if(body_r is Result.Err) {

            var err = http::response_text(400u, "Bad Request\n")
            var _ = err.write_to(stream, false, false)
            return false
        }

        var is_head = http::text_equals(req.method.to_view(), string_view("HEAD"))
        var keep = request_wants_keep_alive(&req)
        var res = self.dispatch(req)

        // A handler may opt out of keep-alive explicitly.
        if(keep) {
            var conn = res.headers.get("Connection")
            if(conn is Option.Some) {
                var Some(cv) = conn else unsafe { unreachable }
                if(http::text_equals(cv, string_view("close"))) { keep = false }
            }
        }

        var written = res.write_to(stream, keep, is_head)
        if(written is Result.Err) {
            return false
        }
        return keep
    }

    /// Serve a whole connection (blocking), honouring keep-alive and pipelining.
    func serve_connection(&self, connection : http::Stream) {
        var stream = connection
        var buf = net::Buffer()
        while(self.is_running()) {
            if(!self.serve_ready(&mut stream, &mut buf)) { break }
        }
        var _ = stream.shutdown()
    }

    /// Route + invoke the handler. The request is moved into the handler, which
    /// is what makes the callback free of lifetimes.
    func dispatch(&self, req : http::Request) : http::Response {
        var mutable_req = req
        var params = std::vector<std::pair<string, string>>()
        var route = self.router.match_route(&mutable_req.method, &mutable_req.path, &mut params)
        if(route is Option.Some) {
            var Some(matched) = route else unsafe { unreachable }
            mutable_req.route_params = std::replace(&mut params, std::vector<std::pair<string, string>>())
            return matched.handler(mutable_req)
        }
        return http::response_not_found()
    }

    // -----------------------------------------------------------------------
    // Drivers
    // -----------------------------------------------------------------------

    /// Bind if needed, then serve on this thread until `shutdown`.
    public func serve(self) {
        var s = self
        if(!s.bound) {
            var r = s.bind_socket()
            if(r is Result.Err) { return }
        }
        comptime if(def.windows) {
            s.serve_iocp()
        } else {
            s.serve_accept()
        }
        s.listener.close()
    }

    /// Serve on a background thread. Create a `ShutdownHandle` first: the server
    /// is consumed here.
    public func serve_async(self) : std.concurrent.Thread {
        var s = self
        return std.thread.spawn_void(serve_thread, s)
    }

    func serve_accept(&mut self) {
        var selfp : *Server = &raw mut self
        while(self.is_running()) {
            var accepted = self.listener.accept()
            if(accepted is Result.Err) {
                if(!self.is_running()) { break }
                continue
            }
            var Ok(tcp) = accepted else unsafe { unreachable }
            tcp.set_keep_alive(true)
            var fd = unsafe(net::raw::release_tcp(&mut tcp))
            // A non-blocking listener hands the flag to the accepted socket on
            // some platforms; the connection is served with blocking reads.
            net::raw::set_blocking(fd)
            unsafe(self.pool.submit_void(|selfp, fd|() => {
                unsafe {
                    var tcp2 = net::raw::stream_from_fd(fd)
                    tcp2.set_keep_alive(true)
                    var opened = selfp.open_stream(tcp2)
                    if(opened is Option.None) { return }
                    var Some(stream) = opened else unsafe { unreachable }
                    selfp.serve_connection(stream)
                }
            }))
        }
    }

    /// Windows: the socket is registered with a completion port and every
    /// completion runs one message through the shared pipeline.
    func serve_iocp(&mut self) {
        // The whole body is Windows-only: `net::iocp` does not exist on other
        // targets, and a comptime-false branch is never resolved.
        comptime if(def.windows) { unsafe {
        var selfp : *Server = &raw mut self
        var cp = unsafe(malloc(sizeof(net::iocp::CompletionPort))) as *mut net::iocp::CompletionPort
        if(cp == null) { return }
        unsafe { new (cp) net::iocp::CompletionPort(self.cfg.worker_count) }
        var i = 0u;
        while(i < self.cfg.worker_count) {
            unsafe(self.pool.submit_void(|cp, selfp|() => {
                unsafe { while(selfp.is_running()) { cp.poll(500); } }
            }))
            i = i + 1u;
        }
        var listen_fd = net::raw::listener_fd(&self.listener)
        while(self.is_running()) {
            var s = net::raw::accept_socket(listen_fd)
            if(s == 0u || (s as longlong) < 0) {
                std.concurrent.sleep_ms(1u)
                continue
            }
            net::raw::set_keep_alive(s, true)
            if(!cp.register(s, s as usize)) {
                net::raw::close_socket(s)
                continue
            }
            var buf_sz = 8192u;
            var buf = malloc(buf_sz) as *mut char;
            var ctx = malloc(sizeof(net::iocp::AsyncContext)) as *mut net::iocp::AsyncContext;
            if(buf == null || ctx == null) {
                if(buf != null) { free(buf) }
                if(ctx != null) { free(ctx) }
                net::raw::close_socket(s)
                continue
            }
            new (ctx) net::iocp::AsyncContext(buf, buf_sz);
            ctx.callback = |s, selfp|(ctx, bytes, ok) => {
                unsafe {
                    if(!ok || bytes == 0) {
                        net::raw::close_socket(s);
                        free(ctx.buffer.buf);
                        delete ctx;
                        return;
                    }
                    ctx.accumulated.append(std::span<u8>(ctx.buffer.buf as *u8, bytes as size_t))
                    var tcp = net::raw::stream_from_fd(s)
                    var opened = selfp.open_stream(tcp)
                    if(opened is Option.None) {
                        net::raw::close_socket(s);
                        free(ctx.buffer.buf);
                        delete ctx;
                        return;
                    }
                    var Some(stream) = opened else unsafe { unreachable }
                    var keep = selfp.serve_ready(&mut stream, &mut ctx.accumulated)
                    if(keep) {
                        var _ = stream.detach()
                        ctx.overlapped.Internal = 0;
                        ctx.overlapped.InternalHigh = 0;
                        ctx.overlapped.Offset = 0;
                        ctx.overlapped.OffsetHigh = 0;
                        net::iocp::async_recv(s, ctx);
                    } else {
                        net::raw::close_socket(s);
                        free(ctx.buffer.buf);
                        delete ctx;
                    }
                }
            };
            net::iocp::async_recv(s, ctx);
        }
        }}
    }
    /// Wrap an accepted socket as a plain stream or (when certificates are
    /// configured) perform the server handshake first.
    func open_stream(&self, tcp : net::TcpStream) : Option<http::Stream> {
        if(!self.tls_enabled) {
            return Option.Some<http::Stream>(http::Stream.Tcp(tcp))
        }
        var scfg = tls::server_config(&self.tls_chain, &self.tls_key)
        var accepted = tls::accept(tcp, &scfg)
        if(accepted is Result.Err) {
            return Option.None<http::Stream>()
        }
        var Ok(tls_stream) = accepted else unsafe { unreachable }
        return Option.Some<http::Stream>(http::Stream.TlsServer(tls_stream))
    }
}

// A `Server` owns a listener descriptor and a worker pool, and every field a
// worker reads is either immutable for the serving lifetime (`cfg`, `router`,
// TLS credentials) or atomic (`stopping`). `serve`/`serve_async` consume the
// value, so exactly one thread owns it while workers borrow it read-only.
unsafe impl Send for Server { }

func serve_thread(server : Server) {
    var s = server
    s.serve()
}

/// Split `host:port`. Returns `None` when the port is not a number.
public func parse_host_port(addr : string_view, default_port : u16) : Option<net::SocketAddr> {
    var host = string()
    var port : u16 = default_port
    if(addr.size() > 0u && addr.at(0u) == '[') {
        // IPv6 literal: [::1]:8080
        var close = addr.find_or_npos("]")
        if(close == std::NPOS) { return Option.None<net::SocketAddr>() }
        host.append_view(addr.subview(1u, close))
        if(close + 1u < addr.size() && addr.at(close + 1u) == ':') {
            var pv = parse_port(addr.subview(close + 2u, addr.size()))
            if(pv is Option.None) { return Option.None<net::SocketAddr>() }
            var Some(p) = pv else unsafe { unreachable }
            port = p
        }
    } else {
        var colon = addr.find_last_or_npos(":")
        if(colon == std::NPOS) {
            host.append_view(&addr)
        } else {
            host.append_view(addr.subview(0u, colon))
            var pv = parse_port(addr.subview(colon + 1u, addr.size()))
            if(pv is Option.None) { return Option.None<net::SocketAddr>() }
            var Some(p) = pv else unsafe { unreachable }
            port = p
        }
    }
    if(host.empty()) { host.append_view("0.0.0.0") }
    return Option.Some<net::SocketAddr>(net::SocketAddr { ip : host, port : port })
}

func parse_port(v : string_view) : Option<u16> {
    if(v.empty()) { return Option.None<u16>() }
    var p : u32 = 0u
    var i = 0u
    while(i < v.size()) {
        var c = v.at(i)
        if(c < '0' || c > '9') { return Option.None<u16>() }
        p = p * 10u + ((c as u32) - ('0' as u32))
        if(p > 65535u) { return Option.None<u16>() }
        i += 1u
    }
    return Option.Some<u16>(p as u16)
}

func request_wants_keep_alive(req : &http::Request) : bool {
    var conn = req.headers.get("Connection")
    if(conn is Option.Some) {
        var Some(cv) = conn else unsafe { unreachable }
        if(http::text_equals(cv, string_view("close"))) { return false }
        if(http::text_equals(cv, string_view("keep-alive"))) { return true }
    }
    if(http::text_equals(req.proto.to_view(), string_view("HTTP/1.0"))) { return false }
    return true
}

} // namespace server
