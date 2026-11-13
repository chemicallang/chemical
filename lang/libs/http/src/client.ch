// ===== HTTP Client =====
public namespace http {

using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;

public struct URL {
    var scheme: string;
    var host: string;
    var port: uint;
    var path: string;
    var query: string;

    @constructor func constructor() {
        return URL {
            scheme = http_lit("http"),
            host = string(),
            port = 80u,
            path = http_lit("/"),
            query = string()
        }
    }

    public func is_https(&self) : bool {
        return self.scheme.equals_with_len("https", 5)
    }

    /// Whether the port differs from the scheme default (and must be emitted).
    public func has_explicit_port(&self) : bool {
        if(self.is_https()) { return self.port != 443u }
        return self.port != 80u
    }

    public func parse(url_str: &string_view) : Option<URL> {
        var u = *url_str;
        var res = URL();

        if(u.size() >= 7 && (u.at(0) == 'h' || u.at(0) == 'H') &&
           (u.at(1) == 't' || u.at(1) == 'T') &&
           (u.at(2) == 't' || u.at(2) == 'T') &&
           (u.at(3) == 'p' || u.at(3) == 'P')) {
            if(u.size() >= 8 && (u.at(4) == 's' || u.at(4) == 'S') && u.at(5) == ':' && u.at(6) == '/' && u.at(7) == '/') {
                res.scheme = http_lit("https");
                res.port = 443u;
                u = u.skip(8u);
            } else if(u.at(4) == ':' && u.at(5) == '/' && u.at(6) == '/') {
                res.scheme = http_lit("http");
                u = u.skip(7u);
            }
        }

        var slash = u.find_or_npos("/");
        var host_port = string_view();
        if(slash == std::NPOS) {
            host_port = u;
            res.path = http_lit("/");
        } else {
            host_port = u.subview(0u, slash);
            var full_path = u.subview(slash, u.size());
            var hash_pos = full_path.find_or_npos("#");
            var path_end = hash_pos;
            if(hash_pos == std::NPOS) { path_end = full_path.size() }
            var qmark = full_path.find_or_npos("?");
            if(qmark == std::NPOS || qmark > path_end) {
                res.path = string::view_make(full_path.subview(0u, path_end));
            } else {
                res.path = string::view_make(full_path.subview(0u, qmark));
                var query_end = path_end;
                if(query_end > qmark + 1u) {
                    res.query = string::view_make(full_path.subview(qmark + 1u, query_end));
                }
            }
        }

        // Split host/port. An IPv6 literal is bracketed: [::1]:8080.
        if(host_port.size() > 0u && host_port.at(0u) == '[') {
            var close = host_port.find_or_npos("]");
            if(close != std::NPOS) {
                res.host = string::view_make(host_port.subview(1u, close));
                if(close + 1u < host_port.size() && host_port.at(close + 1u) == ':') {
                    var pv = url_port(host_port.subview(close + 2u, host_port.size()));
                    if(pv is Option.None) { return Option.None<URL>() }
                    var Some(p) = pv else unsafe { unreachable }
                    res.port = p as uint;
                }
            }
        } else {
            var colon = host_port.find_last_or_npos(":");
            if(colon == std::NPOS) {
                res.host = string::view_make(&host_port);
            } else {
                res.host = string::view_make(host_port.subview(0u, colon));
                var pv = url_port(host_port.subview(colon + 1u, host_port.size()));
                if(pv is Option.None) { return Option.None<URL>() }
                var Some(p) = pv else unsafe { unreachable }
                res.port = p as uint;
            }
        }

        if(res.host.empty()) { return Option.None<URL>() }
        return Option.Some<URL>(res);
    }
}

func url_port(v : string_view) : Option<u16> {
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

func http_base64(data : &[u8]) : string {
    var table = http_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
    var out = string()
    var i : size_t = 0
    while(i + 2u < data.size()) {
        var n = ((data.at(i) as u32) << 16) | ((data.at(i + 1u) as u32) << 8) | (data.at(i + 2u) as u32)
        out.append(table.at(((n >> 18) & 63u) as size_t))
        out.append(table.at(((n >> 12) & 63u) as size_t))
        out.append(table.at(((n >> 6) & 63u) as size_t))
        out.append(table.at((n & 63u) as size_t))
        i += 3u
    }
    var rem = data.size() - i
    if(rem == 1u) {
        var n = (data.at(i) as u32) << 16
        out.append(table.at(((n >> 18) & 63u) as size_t))
        out.append(table.at(((n >> 12) & 63u) as size_t))
        out.append('=')
        out.append('=')
    } else if(rem == 2u) {
        var n = ((data.at(i) as u32) << 16) | ((data.at(i + 1u) as u32) << 8)
        out.append(table.at(((n >> 18) & 63u) as size_t))
        out.append(table.at(((n >> 12) & 63u) as size_t))
        out.append(table.at(((n >> 6) & 63u) as size_t))
        out.append('=')
    }
    return out
}

public struct RequestBuilder {
    internal var method: string;
    internal var url: URL;
    internal var headers: HeaderMap;
    internal var body: string;
    internal var timeout_secs: long;

    @constructor func constructor(m: *char, u: URL) {
        return RequestBuilder {
            method = http_lit(m),
            url = u,
            headers = HeaderMap(),
            body = string(),
            timeout_secs = 10
        }
    }

    public func new(m : &str, u : URL) : RequestBuilder {
        var made = std::cstring_make(m)
        if(made is Result.Err) {
            return RequestBuilder("GET", u)
        }
        var Ok(mz) = made else unsafe { unreachable }
        return RequestBuilder(unsafe(mz.as_ptr()), u)
    }

    public func header(&mut self, k: &str, v: &str) : &mut RequestBuilder {
        headers.insert(http_make_string(k), http_make_string(v));
        return self;
    }

    public func header_view(&mut self, k : &string_view, v : &string_view) : &mut RequestBuilder {
        headers.insert_view(k, v);
        return self;
    }

    public func query(&mut self, k : string_view, v : string_view) : &mut RequestBuilder {
        if(!url.query.empty()) { url.query.append('&') }
        url.query.append_view(&k);
        url.query.append('=');
        url.query.append_view(&v);
        return self;
    }

    public func set_body(&mut self, b : &string_view, content_type : &str = "text/plain") : &mut RequestBuilder {
        body = string::view_make(b);
        if(content_type.size() > 0u) {
            headers.insert(http_lit("Content-Type"), http_make_string(content_type));
        }
        return self;
    }

    public func set_body_bytes(&mut self, b : &[u8], content_type : &str = "text/plain") : &mut RequestBuilder {
        body = http_ctor(b.data() as *char, b.size())
        if(content_type.size() > 0u) {
            headers.insert(http_lit("Content-Type"), http_make_string(content_type));
        }
        return self;
    }

    public func set_body_str(&mut self, b : &str, content_type : &str = "text/plain") : &mut RequestBuilder {
        body = http_make_string(b)
        return self.header("Content-Type", content_type)
    }

    public func basic_auth(&mut self, user : &str, pass : &str) : &mut RequestBuilder {
        var raw = string()
        raw.append_view(user.to_view())
        raw.append(':')
        raw.append_view(pass.to_view())
        var encoded = http_base64(unsafe(std::span<u8>(raw.data() as *u8, raw.size())))
        var value = string()
        value.append_view("Basic ")
        value.append_string(&encoded)
        return self.header("Authorization", value.to_view())
    }

    public func timeout(&mut self, secs: long) : &mut RequestBuilder {
        timeout_secs = secs;
        return self;
    }

    public func is_head(&self) : bool {
        return self.method.equals_with_len("HEAD", 4);
    }

    public func build(&self, absolute: bool = false) : string {
        var out = string();
        out.append_string(&method);
        out.append(' ');
        if(absolute) {
            out.append_string(&url.scheme);
            out.append_view("://");
            out.append_string(&url.host);
            if(url.has_explicit_port()) {
                out.append(':');
                out.append_uinteger(url.port);
            }
        }
        out.append_string(&url.path);
        if(!url.query.empty()) {
            out.append('?');
            out.append_string(&url.query);
        }
        out.append_view(" HTTP/1.1\r\nHost: ");
        out.append_string(&url.host);
        if(url.has_explicit_port()) {
            out.append(':');
            out.append_uinteger(url.port);
        }
        out.append_view("\r\n");

        if(body.size() > 0u) {
            out.append_view("Content-Length: ");
            out.append_uinteger(body.size() as ubigint);
            out.append_view("\r\n");
        }

        if(headers.get("User-Agent") is Option.None) {
            out.append_view("User-Agent: chemical-client/0.1\r\n");
        }

        var iter = headers.iter()
        while(iter.valid()) {
            out.append_view(iter.name());
            out.append_view(": ");
            out.append_view(iter.value());
            out.append_view("\r\n");
            iter.next()
        }
        out.append_view("Connection: close\r\n\r\n");
        if(body.size() > 0u) {
            out.append_string(&body);
        }
        return out;
    }
}

// Trust material. Owned by the client and only ever read after configuration,
// which is why it can live behind a `Mutex` and be shared by concurrent
// requests. Public only because it is a field type of the public `Client`;
// its fields are internal, so safe code cannot construct or inspect one.
public struct TrustStore {
    internal var ca_chain: tls::CertificateChain;
    internal var has_ca: bool;
    internal var system_ca: tls::CertificateChain;
    internal var has_system_ca: bool;
}

// A parsed certificate chain is an immutable heap handle (exactly what a server
// already shares across its workers), so moving it between threads and reading
// it concurrently is sound. All mutation goes through `Mutex<TrustStore>`.
unsafe impl Send for TrustStore { }
unsafe impl Sync for TrustStore { }

public struct Client {
    internal var default_timeout_secs: long;
    internal var max_response_header_bytes: usize;
    var max_body_len: usize;
    // Owned trust store, guarded so `&self` request methods can read it.
    internal var trusted: std::Mutex<TrustStore>;
    internal var tls_skip_verify: bool;
    internal var proxy_host: string;
    internal var proxy_port: uint;

    @constructor func constructor() {
        return Client {
            default_timeout_secs = 10,
            max_response_header_bytes = 64u * 1024u,
            max_body_len = DEFAULT_MAX_BODY_LEN,
            trusted : std::new_mutex(TrustStore {
                ca_chain : tls::CertificateChain(),
                has_ca : false,
                system_ca : tls::CertificateChain(),
                has_system_ca : false
            }),
            tls_skip_verify = false,
            proxy_host = string(),
            proxy_port = 0u
        }
    }

    /// Use a specific CA chain for HTTPS verification instead of the system
    /// bundle. The client takes ownership of the chain and keeps it alive for
    /// every request.
    public func set_ca_chain(&mut self, chain : tls::CertificateChain) : &mut Client {
        var guard = self.trusted.lock()
        var store = guard.get()
        store.ca_chain = chain
        store.has_ca = true
        return self;
    }

    /// Load and cache the system CA bundle (once per client).
    func ensure_ca(&self) : Result<Unit, HttpError> {
        var guard = self.trusted.lock()
        var store = guard.get()
        if(store.has_ca || store.has_system_ca) {
            return Result.Ok(Unit { })
        }
        var ca = tls::load_system_ca_bundle()
        if(ca is Result.Err) {
            var Err(e) = ca else unsafe { unreachable }
            return Result.Err(HttpError.Tls(e))
        }
        var Ok(chain) = ca else unsafe { unreachable }
        store.system_ca = std::replace(&mut chain, tls::CertificateChain())
        store.has_system_ca = true
        return Result.Ok(Unit { })
    }

    public func insecure_skip_verify(&mut self, skip : bool = true) : &mut Client {
        tls_skip_verify = skip;
        return self;
    }

    public func set_proxy(&mut self, host: &str, port: uint) : &mut Client {
        proxy_host = http_make_string(host);
        proxy_port = port;
        return self;
    }

    func tls_config(&self, host : &str) : Result<tls::TlsConfig, HttpError> {
        var cfg = tls::TlsConfig.client()
        // Verification-disabled configs need no trust store at all.
        if(!self.tls_skip_verify) {
            var set : Result<Unit, tls::TlsError>
            var guard = self.trusted.lock()
            var store = guard.get()
            if(store.has_ca) {
                set = cfg.set_ca_chain(&store.ca_chain)
            } else {
                set = cfg.set_ca_chain(&store.system_ca)
            }
            if(set is Result.Err) {
                var Err(e) = set else unsafe { unreachable }
                return Result.Err(HttpError.Tls(e))
            }
        }
        if(self.tls_skip_verify) { cfg.insecure_skip_verify() }
        var host_set = cfg.set_hostname(host)
        if(host_set is Result.Err) {
            var Err(e) = host_set else unsafe { unreachable }
            return Result.Err(HttpError.Tls(e))
        }
        return Result.Ok(cfg)
    }

    /// Open the connection for one request: a fresh socket, a proxy tunnel, or a
    /// TLS client over either. Returned by value so the caller never declares an
    /// uninitialized `Stream`.
    func open_connection(&self, req_builder : &RequestBuilder, host_str : std::str,
                         is_https : bool, use_proxy : bool) : Result<Stream, HttpError> {
        if(use_proxy) {
            var proxy_host_str = std::str.make_view(proxy_host.to_view())
            var tcp_r = net::TcpStream.connect(proxy_host_str, proxy_port as u16)
            if(tcp_r is Result.Err) {
                return Result.Err(HttpError.Connect(http_make_string("failed to connect to proxy")))
            }
            var Ok(tcp) = tcp_r else unsafe { unreachable }
            if(!is_https) {
                return Result.Ok(Stream.Tcp(tcp))
            }
            var connect_req = string()
            connect_req.append_view("CONNECT ")
            connect_req.append_string(&req_builder.url.host)
            connect_req.append(':')
            connect_req.append_uinteger(req_builder.url.port)
            connect_req.append_view(" HTTP/1.1\r\nHost: ")
            connect_req.append_string(&req_builder.url.host)
            connect_req.append(':')
            connect_req.append_uinteger(req_builder.url.port)
            connect_req.append_view("\r\n\r\n")
            var tunnel = Stream.Tcp(tcp)
            var wr = tunnel.write_all_bytes(unsafe(std::span<u8>(connect_req.data() as *u8, connect_req.size())))
            if(wr is Result.Err) {
                return Result.Err(HttpError.Connect(http_make_string("proxy CONNECT write failed")))
            }
            if(!proxy_tunnel_ok(&mut tunnel)) {
                return Result.Err(HttpError.Connect(http_make_string("proxy CONNECT failed")))
            }
            var cfg = self.tls_config(host_str)
            if(cfg is Result.Err) {
                var Err(e) = cfg else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(tcfg) = cfg else unsafe { unreachable }
            var owned = tunnel.detach()
            var tcp2 = net::raw::stream_from_fd(owned)
            var tls_r = tls::TlsStream.wrap(tcp2, &tcfg)
            if(tls_r is Result.Err) {
                var Err(e) = tls_r else unsafe { unreachable }
                return Result.Err(HttpError.Tls(e))
            }
            var Ok(ts) = tls_r else unsafe { unreachable }
            return Result.Ok(Stream.TlsClient(ts))
        }
        if(is_https) {
            var cfg = self.tls_config(host_str)
            if(cfg is Result.Err) {
                var Err(e) = cfg else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(tcfg) = cfg else unsafe { unreachable }
            var tls_r = tls::TlsStream.connect(host_str, req_builder.url.port as u16, &tcfg)
            if(tls_r is Result.Err) {
                var Err(e) = tls_r else unsafe { unreachable }
                return Result.Err(HttpError.Tls(e))
            }
            var Ok(ts) = tls_r else unsafe { unreachable }
            return Result.Ok(Stream.TlsClient(ts))
        }
        var tcp_r = net::TcpStream.connect(host_str, req_builder.url.port as u16)
        if(tcp_r is Result.Err) {
            return Result.Err(HttpError.Connect(http_make_string("failed to connect")))
        }
        var Ok(tcp) = tcp_r else unsafe { unreachable }
        return Result.Ok(Stream.Tcp(tcp))
    }

    /// Perform one request on a fresh connection. `&self`, so one client can be
    /// shared by a worker pool; the trust store is read under its own lock.
    public func request(&self, req_builder : &RequestBuilder) : Result<Response, HttpError> {
        var host_str = std::str.make_view(req_builder.url.host.to_view())
        var is_https = req_builder.url.is_https()
        var use_proxy = proxy_host.size() > 0u && proxy_port > 0u
        // Plain HTTP never touches the trust store; load it only for a TLS hop
        // that verifies the peer.
        if(is_https && !self.tls_skip_verify) {
            var ca_ready = self.ensure_ca()
            if(ca_ready is Result.Err) {
                var Err(e) = ca_ready else unsafe { unreachable }
                return Result.Err(e)
            }
        }

        var opened = self.open_connection(req_builder, host_str, is_https, use_proxy)
        if(opened is Result.Err) {
            var Err(oe) = opened else unsafe { unreachable }
            return Result.Err(oe)
        }
        var Ok(stream) = opened else unsafe { unreachable }

        var req_data = req_builder.build(use_proxy && !is_https)
        var wr = stream.write_all_bytes(unsafe(std::span<u8>(req_data.data() as *u8, req_data.size())))
        if(wr is Result.Err) {
            return Result.Err(HttpError.Io(http_io_error()))
        }

        var buf = net::Buffer()
        var head = read_response_head(&mut stream, &mut buf, req_builder.timeout_secs,
                                      self.max_response_header_bytes)
        if(head is Result.Err) {
            var Err(e) = head else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(res) = head else unsafe { unreachable }
        if(req_builder.is_head()) {
            res.body = ResponseBody.Empty()
        } else {
            var body_r = read_response_body(&mut stream, &mut buf, &mut res,
                                            self.max_body_len,
                                            (req_builder.timeout_secs * 4 * 1000) as u64)
            if(body_r is Result.Err) {
                var Err(e) = body_r else unsafe { unreachable }
                return Result.Err(e)
            }
        }
        var _ = stream.shutdown()
        return Result.Ok(res)
    }

    // -----------------------------------------------------------------------
    // Convenience methods
    // -----------------------------------------------------------------------

    public func get(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.get_view(&sv)
    }

    public func get_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        var parsed = URL.parse(url_str)
        if(parsed is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(url) = parsed else unsafe { unreachable }
        var rb = RequestBuilder("GET", url)
        rb.timeout_secs = self.default_timeout_secs
        return self.request(&rb)
    }

    public func post(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.post_view(&sv, body, content_type)
    }

    public func post_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        return self.with_body("POST", url_str, body, content_type)
    }

    public func put(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.put_view(&sv, body, content_type)
    }

    public func put_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        return self.with_body("PUT", url_str, body, content_type)
    }

    public func patch(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.patch_view(&sv, body, content_type)
    }

    public func patch_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        return self.with_body("PATCH", url_str, body, content_type)
    }

    public func delete(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.delete_view(&sv)
    }

    public func delete_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        return self.simple("DELETE", url_str)
    }

    public func head(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.head_view(&sv)
    }

    public func head_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        return self.simple("HEAD", url_str)
    }

    func simple(&self, method : &str, url_str : &string_view) : Result<Response, HttpError> {
        var parsed = URL.parse(url_str)
        if(parsed is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(url) = parsed else unsafe { unreachable }
        var m = http_make_string(method)
        var rb = RequestBuilder(unsafe(m.c_str()), url)
        rb.timeout_secs = self.default_timeout_secs
        return self.request(&rb)
    }

    func with_body(&self, method : &str, url_str : &string_view, body : &string_view, content_type : &str) : Result<Response, HttpError> {
        var parsed = URL.parse(url_str)
        if(parsed is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(url) = parsed else unsafe { unreachable }
        var m = http_make_string(method)
        var rb = RequestBuilder(unsafe(m.c_str()), url)
        rb.timeout_secs = self.default_timeout_secs
        rb.set_body(body, content_type)
        return self.request(&rb)
    }
}

// Read a proxy CONNECT response head and return whether it was 2xx.
func proxy_tunnel_ok(tunnel : &mut Stream) : bool {
    var buf = net::Buffer()
    var reads = 0
    while(true) {
        var i = 0u
        var n = buf.len()
        while(i + 3u < n) {
            if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i+1u) == '\n' as u8 &&
               buf.get_byte(i+2u) == '\r' as u8 && buf.get_byte(i+3u) == '\n' as u8) {
                // Status line: HTTP/1.1 200 ...
                var j = 0u
                while(j < i && buf.get_byte(j) != '\r' as u8) { j = j + 1u }
                var sp = 0u
                while(sp < j && buf.get_byte(sp) != ' ' as u8) { sp = sp + 1u }
                sp = sp + 1u
                var code : u32 = 0u
                while(sp < j && buf.get_byte(sp) >= '0' as u8 && buf.get_byte(sp) <= '9' as u8) {
                    code = code * 10u + ((buf.get_byte(sp) as u32) - ('0' as u32))
                    sp = sp + 1u
                }
                return code >= 200u && code < 300u
            }
            i = i + 1u
        }
        var tmp : [1024]u8
        var view = unsafe(std::mut_span<u8>(&raw mut tmp[0], 1024 as size_t))
        var r = tunnel.read_bytes(view)
        if(r is Result.Err) { return false }
        var Ok(got) = r else unsafe { unreachable }
        if(got == 0u) { return false }
        buf.append(unsafe(std::span<u8>(&raw mut tmp[0], got)))
        reads = reads + 1
        if(reads > 128) { return false }
    }
    return false
}

} // namespace http
