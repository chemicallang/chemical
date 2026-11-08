// ===== HTTP Client =====
public namespace http {

using std::Option;
using std::Result;
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

    public func parse(url_str: &string_view) : Option<URL> {
        var u = *url_str;
        var res = URL();

        if(u.size() >= 7 && (u.get(0) == 'h' || u.get(0) == 'H') &&
           (u.get(1) == 't' || u.get(1) == 'T') &&
           (u.get(2) == 't' || u.get(2) == 'T') &&
           (u.get(3) == 'p' || u.get(3) == 'P')) {
            if(u.size() >= 8 && (u.get(4) == 's' || u.get(4) == 'S') && u.get(5) == ':' && u.get(6) == '/' && u.get(7) == '/') {
                res.scheme = http_lit("https");
                res.port = 443u;
                u = u.skip(8u);
            } else if(u.get(4) == ':' && u.get(5) == '/' && u.get(6) == '/') {
                res.scheme = http_lit("http");
                u = u.skip(7u);
            }
        }

        var slash = u.find("/");
        var host_port = string_view();
        if(slash == std::NPOS) {
            host_port = u;
            res.path = http_lit("/");
        } else {
            host_port = u.subview(0u, slash);
            var full_path = u.subview(slash, u.size());
            var hash_pos = full_path.find("#");
            var path_end = hash_pos;
            if(hash_pos == std::NPOS) { path_end = full_path.size() }
            var qmark = full_path.find("?");
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

        var colon = host_port.find(":");
        if(colon == std::NPOS) {
            res.host = string::view_make(&host_port);
        } else {
            res.host = string::view_make(host_port.subview(0u, colon));
            var pstr = host_port.subview(colon + 1u, host_port.size());
            var pval = 0u;
            for(var i=0u; i<pstr.size(); i++) {
                var c = pstr.get(i);
                if(c >= '0' && c <= '9') { pval = pval * 10u + (c as uint - '0' as uint) }
            }
            if(pval > 0u) { res.port = pval }
        }

        if(res.host.empty()) { return Option.None<URL>() }
        return Option.Some<URL>(res);
    }
}

func http_base64(data : &[u8]) : string {
    var table = http_lit("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")
    var out = string()
    var i : size_t = 0
    while(i + 2u < data.size()) {
        var n = ((data.get(i) as u32) << 16) | ((data.get(i + 1u) as u32) << 8) | (data.get(i + 2u) as u32)
        out.append(table.get(((n >> 18) & 63u) as size_t))
        out.append(table.get(((n >> 12) & 63u) as size_t))
        out.append(table.get(((n >> 6) & 63u) as size_t))
        out.append(table.get((n & 63u) as size_t))
        i += 3u
    }
    var rem = data.size() - i
    if(rem == 1u) {
        var n = (data.get(i) as u32) << 16
        out.append(table.get(((n >> 18) & 63u) as size_t))
        out.append(table.get(((n >> 12) & 63u) as size_t))
        out.append('=')
        out.append('=')
    } else if(rem == 2u) {
        var n = ((data.get(i) as u32) << 16) | ((data.get(i + 1u) as u32) << 8)
        out.append(table.get(((n >> 18) & 63u) as size_t))
        out.append(table.get(((n >> 12) & 63u) as size_t))
        out.append(table.get(((n >> 6) & 63u) as size_t))
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

    public func header_str(&mut self, k : &str, v : &str) : &mut RequestBuilder {
        return self.header(k, v)
    }

    public func header_view(&mut self, k : &string_view, v : &string_view) : &mut RequestBuilder {
        headers.insert(string::view_make(k), string::view_make(v));
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

    public func build(&self, absolute: bool = false) : string {
        var out = string();
        out.append_string(&method);
        out.append(' ');
        if(absolute) {
            out.append_string(&url.scheme);
            out.append_view("://");
            out.append_string(&url.host);
            if((url.scheme.equals_with_len("http", 4) && url.port != 80u) || (url.scheme.equals_with_len("https", 5) && url.port != 443u)) {
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
        if((url.scheme.equals_with_len("http", 4) && url.port != 80u) || (url.scheme.equals_with_len("https", 5) && url.port != 443u)) {
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

        var i = 0u;
        while(i < headers.headers.size()) {
            var p = headers.headers.get_ptr(i);
            unsafe { out.append_string(&p.first); }
            out.append_view(": ");
            unsafe { out.append_string(&p.second); }
            out.append_view("\r\n");
            i++;
        }
        out.append_view("Connection: close\r\n\r\n");
        if(body.size() > 0u) {
            out.append_string(&body);
        }
        return out;
    }
}

public struct Client {
    internal var default_timeout_secs: long;
    internal var max_response_header_bytes: usize;
    var max_body_len: usize;
    internal var ca_chain: *mut tls::raw::X509Cert;
    internal var tls_skip_verify: bool;
    internal var proxy_host: string;
    internal var proxy_port: uint;

    @constructor func constructor() {
        return Client {
            default_timeout_secs = 10,
            max_response_header_bytes = 64u * 1024u,
            max_body_len = DEFAULT_MAX_BODY_LEN,
            ca_chain = null,
            tls_skip_verify = false,
            proxy_host = string(),
            proxy_port = 0u
        }
    }

    /// Use a specific CA chain for HTTPS verification instead of the system
    /// bundle. The chain must outlive every request made with this client.
    public func set_ca_chain(&mut self, chain : &tls::CertificateChain) : &mut Client {
        ca_chain = unsafe(chain.raw_handle())
        return self;
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
        if(self.ca_chain != null) {
            var set = unsafe(cfg.set_ca_chain_ptr(self.ca_chain))
            if(set is Result.Err) {
                var Err(e) = set else unsafe { unreachable }
                return Result.Err(HttpError.Tls(e))
            }
        } else {
            var ca = tls::load_system_ca_bundle()
            if(ca is Result.Err) {
                var Err(e) = ca else unsafe { unreachable }
                return Result.Err(HttpError.Tls(e))
            }
            var Ok(chain) = ca else unsafe { unreachable }
            var set = cfg.set_ca_chain(&chain)
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

    public func request(&self, req_builder : &RequestBuilder) : Result<Response, HttpError> {
        var host_str = std::str.make_view(req_builder.url.host.to_view())
        var is_https = req_builder.url.scheme.equals_with_len("https", 5)
        var use_proxy = proxy_host.size() > 0u && proxy_port > 0u
        var transport_box : *mut Transport = null

        if(use_proxy) {
            var proxy_host_str = std::str.make_view(proxy_host.to_view())
            var tcp_r = net::TcpStream.connect(proxy_host_str, proxy_port as u16)
            if(tcp_r is Result.Err) {
                return Result.Err(HttpError.Connect(http_make_string("failed to connect to proxy")))
            }
            var Ok(tcp) = tcp_r else unsafe { unreachable }
            if(is_https) {
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
                var wr = tcp.write_all(unsafe(std::span<u8>(connect_req.data() as *u8, connect_req.size())))
                if(wr is Result.Err) {
                    return Result.Err(HttpError.Connect(http_make_string("proxy CONNECT write failed")))
                }
                // Read and validate the CONNECT response head.
                var cbuf = net::Buffer()
                var connect_ok = false
                var reads = 0
                while(true) {
                    var i = 0u
                    var hdr_end = 0u
                    var found = false
                    while(i + 3u < cbuf.len()) {
                        if(cbuf.get_byte(i) == '\r' as u8 && cbuf.get_byte(i+1u) == '\n' as u8 &&
                           cbuf.get_byte(i+2u) == '\r' as u8 && cbuf.get_byte(i+3u) == '\n' as u8) {
                            found = true
                            hdr_end = i
                            break
                        }
                        i = i + 1u
                    }
                    if(found) {
                        var j = 0u
                        while(j < hdr_end && cbuf.get_byte(j) != '\r' as u8) { j = j + 1u }
                        var sp = 0u
                        while(sp < j && cbuf.get_byte(sp) != ' ' as u8) { sp = sp + 1u }
                        sp = sp + 1u
                        var code = 0u
                        while(sp < j && cbuf.get_byte(sp) >= '0' as u8 && cbuf.get_byte(sp) <= '9' as u8) {
                            code = code * 10u + (cbuf.get_byte(sp) as uint - '0' as uint)
                            sp = sp + 1u
                        }
                        if(code >= 200u && code < 300u) { connect_ok = true }
                        break
                    }
                    var tmp : [1024]u8
                    var n = tcp.read(unsafe(std::mut_span<u8>(&raw mut tmp[0], 1024 as size_t)))
                    if(n is Result.Err) { break }
                    var Ok(got) = n else unsafe { unreachable }
                    if(got == 0u) { break }
                    cbuf.append_bytes(&raw mut tmp[0], got)
                    reads = reads + 1
                    if(reads > 128) { break }
                }
                if(!connect_ok) {
                    return Result.Err(HttpError.Connect(http_make_string("proxy CONNECT failed")))
                }
                var cfg = self.tls_config(host_str)
                if(cfg is Result.Err) {
                    var Err(e) = cfg else unsafe { unreachable }
                    return Result.Err(e)
                }
                var Ok(tcfg) = cfg else unsafe { unreachable }
                var tls_r = tls::TlsStream.wrap(tcp, &tcfg)
                if(tls_r is Result.Err) {
                    var Err(e) = tls_r else unsafe { unreachable }
                    return Result.Err(HttpError.Tls(e))
                }
                var Ok(ts) = tls_r else unsafe { unreachable }
                transport_box = transport_box_tls(tls::TlsStream.into_box(&mut ts))
            } else {
                var pfd = unsafe(net::raw::release_tcp(&mut tcp))
                transport_box = transport_box_plain(pfd)
            }
        } else if(is_https) {
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
            transport_box = transport_box_tls(tls::TlsStream.into_box(&mut ts))
        } else {
            var tcp_r = net::TcpStream.connect(host_str, req_builder.url.port as u16)
            if(tcp_r is Result.Err) {
                return Result.Err(HttpError.Connect(http_make_string("failed to connect")))
            }
            var Ok(tcp) = tcp_r else unsafe { unreachable }
            var tfd = unsafe(net::raw::release_tcp(&mut tcp))
            transport_box = transport_box_plain(tfd)
        }

        var req_data = req_builder.build(use_proxy && !is_https)
        var wr = unsafe(transport_box.write_all_bytes(unsafe(std::span<u8>(req_data.data() as *u8, req_data.size()))))
        if(wr is Result.Err) {
            return Result.Err(HttpError.Io(http_io_error()))
        }

        var buf_ptr = new net::Buffer()
        var transport_ref = unsafe(&mut *transport_box)
        var head = read_response_head(transport_ref, buf_ptr, req_builder.timeout_secs,
                                       self.max_response_header_bytes)
        if(head is Result.Err) {
            unsafe { delete buf_ptr }
            var Err(e) = head else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(res) = head else unsafe { unreachable }
        var is_head = req_builder.method.equals_with_len("HEAD", 4)
        var body_len : isize = 0
        var chunked = false
        if(!is_head) {
            body_len = -1
            if(res.body_len > 0u) { body_len = res.body_len as isize }
            var te = res.headers.get("Transfer-Encoding")
            if(te is Option.Some) {
                var Some(tv) = te else unsafe { unreachable }
                if(tv.equals(string_view("chunked"))) { chunked = true; body_len = -1 }
            }
        }
        std::replace(&mut res.body, Body.make_owned(transport_box, buf_ptr, body_len, chunked,
                                   req_builder.timeout_secs * 4, self.max_body_len))
        unsafe { delete buf_ptr }
        return Result.Ok(res)
    }

    public func get(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.get_view(&sv)
    }

    public func get_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("GET", std::replace(&mut u, URL()));
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }

    public func post(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.post_view(&sv, body, content_type)
    }

    public func post_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("POST", std::replace(&mut u, URL()));
        rb.set_body(body, content_type);
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }

    public func put(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.put_view(&sv, body, content_type)
    }

    public func put_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("PUT", std::replace(&mut u, URL()));
        rb.set_body(body, content_type);
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }

    public func patch(&self, url_str: &str, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.patch_view(&sv, body, content_type)
    }

    public func patch_view(&self, url_str: &string_view, body: &string_view, content_type: &str = "text/plain") : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("PATCH", std::replace(&mut u, URL()));
        rb.set_body(body, content_type);
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }

    public func delete(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.delete_view(&sv)
    }

    public func delete_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("DELETE", std::replace(&mut u, URL()));
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }

    public func head(&self, url_str: &str) : Result<Response, HttpError> {
        var sv = unsafe(string_view(url_str.data(), url_str.size()))
        return self.head_view(&sv)
    }

    public func head_view(&self, url_str: &string_view) : Result<Response, HttpError> {
        var u_opt = URL::parse(url_str);
        if(u_opt is Option.None) { return Result.Err(HttpError.InvalidUrl()) }
        var Some(u) = u_opt else unsafe { unreachable };
        var rb = RequestBuilder("HEAD", std::replace(&mut u, URL()));
        rb.timeout(self.default_timeout_secs);
        return self.request(&rb);
    }
}

} // namespace http
