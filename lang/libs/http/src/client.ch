// ===== HTTP Client =====
public namespace http {

    public struct URL {
        var scheme: std::string;
        var host: std::string;
        var port: uint;
        var path: std::string;
        var query: std::string;

        @constructor func constructor() {
            return URL {
                scheme = std::string::make_no_len("http"),
                host = std::string(),
                port = 80u,
                path = std::string::make_no_len("/"),
                query = std::string()
            }
        }

        public func parse(url_str: &std::string_view) : std::Option<URL> {
            var u = *url_str;
            var res = URL();

            if(u.size() >= 7 && (u.get(0) == 'h' || u.get(0) == 'H') &&
               (u.get(1) == 't' || u.get(1) == 'T') &&
               (u.get(2) == 't' || u.get(2) == 'T') &&
               (u.get(3) == 'p' || u.get(3) == 'P')) {
                if(u.size() >= 8 && (u.get(4) == 's' || u.get(4) == 'S') && u.get(5) == ':' && u.get(6) == '/' && u.get(7) == '/') {
                    res.scheme = std::string::make_no_len("https");
                    res.port = 443u;
                    u = u.skip(8u);
                } else if(u.get(4) == ':' && u.get(5) == '/' && u.get(6) == '/') {
                    res.scheme = std::string::make_no_len("http");
                    u = u.skip(7u);
                }
            }

            var slash = u.find("/");
            var host_port = std::string_view();
            if(slash == std::NPOS) {
                host_port = u;
                res.path = std::string::make_no_len("/");
            } else {
                host_port = u.subview(0u, slash);
                var full_path = u.subview(slash, u.size());
                var hash_pos = full_path.find("#");
                var path_end = hash_pos;
                if(hash_pos == std::NPOS) { path_end = full_path.size() }
                var qmark = full_path.find("?");
                if(qmark == std::NPOS || qmark > path_end) {
                    res.path = std::string::view_make(full_path.subview(0u, path_end));
                } else {
                    res.path = std::string::view_make(full_path.subview(0u, qmark));
                    var query_end = path_end;
                    if(query_end > qmark + 1u) {
                        res.query = std::string::view_make(full_path.subview(qmark + 1u, query_end));
                    }
                }
            }

            var colon = host_port.find(":");
            if(colon == std::NPOS) {
                res.host = std::string::view_make(&host_port);
            } else {
                res.host = std::string::view_make(host_port.subview(0u, colon));
                var pstr = host_port.subview(colon + 1u, host_port.size());
                var pval = 0u;
                for(var i=0u; i<pstr.size(); i++) {
                    var c = pstr.get(i);
                    if(c >= '0' && c <= '9') { pval = pval * 10u + (c as uint - '0' as uint) }
                }
                if(pval > 0u) { res.port = pval }
            }

            if(res.host.empty()) { return std::Option.None<URL>() }
            return std::Option.Some<URL>(res);
        }
    }

    public struct RequestBuilder {
        var method: std::string;
        var url: URL;
        var headers: HeaderMap;
        var body: std::string;
        var timeout_secs: long = 10;

        @constructor func constructor(m: *char, u: URL) {
            return RequestBuilder {
                method = std::string::make_no_len(m),
                url = u,
                headers = HeaderMap(),
                body = std::string()
            }
        }

        public func header(&mut self, k: *char, v: *char) : &mut RequestBuilder {
            headers.insert(std::string::make_no_len(k), std::string::make_no_len(v));
            return self;
        }

        public func header_view(&mut self, k: &std::string_view, v: &std::string_view) : &mut RequestBuilder {
            headers.insert(std::string::view_make(k), std::string::view_make(v));
            return self;
        }

        public func query(&mut self, k: std::string_view, v: std::string_view) : &mut RequestBuilder {
            if(!url.query.empty()) { url.query.append('&') }
            url.query.append_view(&k);
            url.query.append('=');
            url.query.append_view(&v);
            return self;
        }

        public func set_body(&mut self, b: &std::string_view, content_type: *char = null) : &mut RequestBuilder {
            body = std::string::view_make(b);
            if(content_type != null) {
                headers.insert(std::string::make_no_len("Content-Type"), std::string(content_type));
            }
            return self;
        }

        public func timeout(&mut self, secs: long) : &mut RequestBuilder {
            timeout_secs = secs;
            return self;
        }

        public func basic_auth(&mut self, user: *char, pass: *char) : &mut RequestBuilder {
            return self;
        }

        public func build(&self, absolute: bool = false) : std::string {
            var out = std::string();
            out.append_string(&method);
            out.append(' ');
            if(absolute) {
                // Proxy requests use an absolute-form request target
                // (RFC 9110 §7.3.2): METHOD http://host[:port]/path HTTP/1.1
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

            if(headers.get("User-Agent") is std::Option.None) {
                out.append_view("User-Agent: chemical-client/0.1\r\n");
            }

            var i = 0u;
            while(i < headers.headers.size()) {
                var p = headers.headers.get_ptr(i);
                out.append_string(&p.first);
                out.append_view(": ");
                out.append_string(&p.second);
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
}

public namespace http {

    public struct Client {
        var default_timeout_secs: long;
        var max_response_header_bytes: usize;
        var max_body_len: usize;   // 0 = unlimited streaming body (e.g. downloads); default 100MB
        // Optional TLS customization:
        //   ca_chain         - caller-owned trust anchor(s); used instead of the
        //                      system CA bundle for server certificate verification.
        //                      The Client does NOT take ownership (caller frees).
        //   tls_skip_verify  - disable certificate chain AND hostname verification
        //                      (self-signed local servers, test harnesses).
        var ca_chain: *mut tls::X509Cert;
        var tls_skip_verify: bool;
        var proxy_host: std::string;
        var proxy_port: uint;

        @constructor func constructor() {
            return Client {
                default_timeout_secs = 10,
                max_response_header_bytes = 64u * 1024u,
                max_body_len = DEFAULT_MAX_BODY_LEN,
                ca_chain = null,
                tls_skip_verify = false,
                proxy_host = std::string(),
                proxy_port = 0u
            }
        }

        // Use a specific CA certificate (chain) for HTTPS server verification
        // instead of the system bundle. The chain remains caller-owned.
        public func set_ca_chain(&mut self, ca: *mut tls::X509Cert) : &mut Client {
            ca_chain = ca;
            return self;
        }

        // Disable TLS certificate and hostname verification (insecure mode).
        public func insecure_skip_verify(&mut self, skip: bool = true) : &mut Client {
            tls_skip_verify = skip;
            return self;
        }

        // Route requests through an HTTP proxy (host:port). HTTPS targets use
        // a CONNECT tunnel; HTTP targets use an absolute-form request line.
        public func set_proxy(&mut self, host: &std::string_view, port: uint) : &mut Client {
            proxy_host = std::string::view_make(host);
            proxy_port = port;
            return self;
        }

        public func request(&self, req_builder: &RequestBuilder) : std::Result<Response, std::string> {
            var is_https = req_builder.url.scheme.equals_with_len("https", 5)
            var use_proxy = proxy_host.size() > 0u && proxy_port > 0u
            var s: net::Socket = 0
            var tls_ctx: *mut tls::SSLContext = null

            // Establish connection (direct or via proxy)
            if(use_proxy) {
                // Route through an HTTP proxy. Plain HTTP targets use an
                // absolute-form request line; HTTPS targets use a CONNECT
                // tunnel and then a TLS handshake over the tunnel.
                s = net::dial(proxy_host.data(), proxy_port)
                if(s == 0u || (s as longlong) < 0) {
                    return std::Result.Err<Response, std::string>(std::string::make_no_len("failed to connect to proxy"))
                }
                if(is_https) {
                    // CONNECT tunnel (RFC 9110 §9.3.6)
                    var connect_req = std::string()
                    connect_req.append_view("CONNECT ")
                    connect_req.append_string(&req_builder.url.host)
                    connect_req.append(':')
                    connect_req.append_uinteger(req_builder.url.port)
                    connect_req.append_view(" HTTP/1.1\r\nHost: ")
                    connect_req.append_string(&req_builder.url.host)
                    connect_req.append(':')
                    connect_req.append_uinteger(req_builder.url.port)
                    connect_req.append_view("\r\n\r\n")
                    net::send_all(s, connect_req.data(), connect_req.size() as int)

                    // Read the CONNECT response headers and verify a 2xx status.
                    net::set_recv_timeout(s, req_builder.timeout_secs, 0)
                    var cbuf = net::Buffer()
                    var connect_ok = false
                    var reads = 0
                    while(true) {
                        var i = 0u; var hdr_end = 0u; var found = false
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
                            // Parse the status code from the first line.
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
                        var n = net::recv_all(s, &raw mut tmp[0], 1024)
                        if(n <= 0) { break }
                        cbuf.append_bytes(&raw mut tmp[0], n as usize)
                        reads = reads + 1
                        if(reads > 128) { break }
                    }
                    if(!connect_ok) {
                        net::close_socket(s)
                        return std::Result.Err<Response, std::string>(std::string::make_no_len("proxy CONNECT failed"))
                    }

                    // TLS handshake over the tunnel (mirrors the direct path,
                    // except the socket is already connected to the proxy).
                    var ssl_ptr = malloc(sizeof(tls::SSLContext)) as *mut tls::SSLContext
                    tls::ssl_init(ssl_ptr)

                    // Create config with auto-loaded CA (or the caller's custom
                    // chain). The config is heap-allocated so it outlives this
                    // function (the Body's TLS context keeps pointing at it);
                    // ssl_free releases it via conf_owned.
                    var config = tls::ssl_config_init(tls::SSL_IS_CLIENT)
                    var custom_ca = ca_chain != null
                    var ca: *mut tls::X509Cert = null
                    if(custom_ca) {
                        ca = ca_chain
                    } else {
                        ca = tls::load_system_ca_bundle()
                    }
                    if(ca != null) {
                        tls::ssl_set_ca_chain(&raw mut config, ca)
                    }
                    if(tls_skip_verify) {
                        config.authmode = tls::SSL_VERIFY_NONE
                    }
                    var config_mem = malloc(sizeof(tls::SSLConfig)) as *mut tls::SSLConfig
                    if(config_mem == null) {
                        if(ca != null) { tls::cert_chain_free(ca) }
                        tls::ssl_free(ssl_ptr)
                        unsafe { dealloc ssl_ptr }
                        return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS config alloc failed"))
                    }
                    *config_mem = config
                    tls::ssl_set_config(ssl_ptr, config_mem)
                    ssl_ptr.conf_owned = true

                    // Set hostname for SNI extension and certificate verification
                    tls::ssl_set_hostname(ssl_ptr, req_builder.url.host.data())

                    // Handshake over the already-connected proxy tunnel
                    tls::ssl_set_socket(ssl_ptr, s)
                    var ret = tls::ssl_handshake(ssl_ptr)
                    if(ret < 0) {
                        if(ca != null && !custom_ca) { tls::cert_chain_free(ca) }
                        tls::ssl_free(ssl_ptr)
                        unsafe { dealloc ssl_ptr }
                        return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS handshake failed"))
                    }

                    // The CA bundle is only needed for handshake-time certificate
                    // verification, which has completed — release it now (only if
                    // we loaded it ourselves; custom chains are caller-owned).
                    if(ca != null && !custom_ca) { tls::cert_chain_free(ca) }

                    // Verify the server's certificate matches the requested hostname
                    if(!tls_skip_verify && ssl_ptr.peer_cert != null) {
                        var hostname_nul = std::string(req_builder.url.host.data(), req_builder.url.host.size())
                        hostname_nul.append('\0')
                        var hret = tls::x509_verify_hostname(ssl_ptr.peer_cert,
                                                              hostname_nul.data())
                        if(hret != 0) {
                            tls::ssl_free(ssl_ptr)
                            unsafe { dealloc ssl_ptr }
                            return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS hostname mismatch"))
                        }
                    }
                    tls_ctx = ssl_ptr
                }
            } else if(is_https) {
                // For HTTPS: heap-allocate TLS context using malloc + ssl_init.
                // The context is freed by the Body destructor (ssl_free + dealloc).
                var ssl_ptr = malloc(sizeof(tls::SSLContext)) as *mut tls::SSLContext
                tls::ssl_init(ssl_ptr)

                // Create config with auto-loaded CA (or the caller's custom
                // chain). The config is heap-allocated so it outlives this
                // function (the Body's TLS context keeps pointing at it);
                // ssl_free releases it via conf_owned.
                var config = tls::ssl_config_init(tls::SSL_IS_CLIENT)
                var custom_ca = ca_chain != null
                var ca: *mut tls::X509Cert = null
                if(custom_ca) {
                    ca = ca_chain
                } else {
                    ca = tls::load_system_ca_bundle()
                }
                if(ca != null) {
                    tls::ssl_set_ca_chain(&raw mut config, ca)
                }
                if(tls_skip_verify) {
                    config.authmode = tls::SSL_VERIFY_NONE
                }
                var config_mem = malloc(sizeof(tls::SSLConfig)) as *mut tls::SSLConfig
                if(config_mem == null) {
                    if(ca != null) { tls::cert_chain_free(ca) }
                    tls::ssl_free(ssl_ptr)
                    unsafe { dealloc ssl_ptr }
                    return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS config alloc failed"))
                }
                *config_mem = config
                tls::ssl_set_config(ssl_ptr, config_mem)
                ssl_ptr.conf_owned = true

                // Set hostname for SNI extension and certificate verification
                tls::ssl_set_hostname(ssl_ptr, req_builder.url.host.data())

                // Connect and handshake
                var ret = tls::tls_connect(ssl_ptr,
                                            req_builder.url.host.data(),
                                            req_builder.url.port)
                if(ret < 0) {
                    if(ca != null && !custom_ca) { tls::cert_chain_free(ca) }
                    tls::ssl_free(ssl_ptr)
                    unsafe { dealloc ssl_ptr }
                    return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS handshake failed"))
                }

                // The CA bundle is only needed for handshake-time certificate
                // verification, which has completed — release it now (only if
                // we loaded it ourselves; custom chains are caller-owned).
                if(ca != null && !custom_ca) { tls::cert_chain_free(ca) }

                // Verify the server's certificate matches the requested hostname
                if(!tls_skip_verify && ssl_ptr.peer_cert != null) {
                    var hostname_nul = std::string(req_builder.url.host.data(), req_builder.url.host.size())
                    hostname_nul.append('\0')
                    var hret = tls::x509_verify_hostname(ssl_ptr.peer_cert,
                                                          hostname_nul.data())
                    if(hret != 0) {
                        tls::ssl_free(ssl_ptr)
                        unsafe { dealloc ssl_ptr }
                        return std::Result.Err<Response, std::string>(std::string::make_no_len("TLS hostname mismatch"))
                    }
                }
                tls_ctx = ssl_ptr
            } else {
                s = net::dial(req_builder.url.host.data(), req_builder.url.port)
                if(s == 0u || (s as longlong) < 0) {
                    return std::Result.Err<Response, std::string>(std::string::make_no_len("failed to connect"))
                }
            }

            // Send request (absolute-form target when routing plain HTTP through a proxy)
            var req_data = req_builder.build(use_proxy && !is_https)
            if(is_https) {
                tls::ssl_write(tls_ctx, req_data.data() as *u8, req_data.size() as i32)
            } else {
                net::send_all(s, req_data.data(), req_data.size() as int)
            }

            // Read response headers (Body.tls_ctx is set inside read_response_incremental)
            var buf_ptr = new net::Buffer()
            var is_head = req_builder.method.equals_with_len("HEAD", 4)
            var res_opt = read_response_incremental(s, &mut *buf_ptr, req_builder.timeout_secs,
                                                      self.max_response_header_bytes,
                                                      tls_ctx,
                                                      self.max_body_len,
                                                      is_head)
            if(res_opt is std::Option.None) {
                if(is_https) {
                    // tls_ctx is heap-allocated via malloc; free with ssl_free + dealloc
                    tls::ssl_free(tls_ctx)
                    unsafe { dealloc tls_ctx }
                } else {
                    net::close_socket(s)
                }
                delete buf_ptr
                return std::Result.Err<Response, std::string>(std::string::make_no_len("failed to read response"))
            }

            var res = res_opt.take()

            // For HTTPS: Body.tls_ctx is set, and Body destructor will free the TLS context
            // For plain HTTP: mark buf as owned by Body so it gets freed
            res.body.owns_buf = true
            return std::Result.Ok<Response, std::string>(res)
        }

        public func get(&self, url_str: &std::string_view) : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("GET", std::replace(&mut u, URL()));
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }

        public func post(&self, url_str: &std::string_view, body: &std::string_view, content_type: *char = "text/plain") : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("POST", std::replace(&mut u, URL()));
            rb.set_body(body, content_type);
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }

        public func put(&self, url_str: &std::string_view, body: &std::string_view, content_type: *char = "text/plain") : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("PUT", std::replace(&mut u, URL()));
            rb.set_body(body, content_type);
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }

        public func patch(&self, url_str: &std::string_view, body: &std::string_view, content_type: *char = "text/plain") : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("PATCH", std::replace(&mut u, URL()));
            rb.set_body(body, content_type);
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }

        public func delete(&self, url_str: &std::string_view) : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("DELETE", std::replace(&mut u, URL()));
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }

        public func head(&self, url_str: &std::string_view) : std::Result<Response, std::string> {
            var u_opt = URL::parse(url_str);
            if(u_opt is std::Option.None) return std::Result.Err<Response, std::string>(std::string::make_no_len("invalid URL"));
            var Some(u) = u_opt else unreachable;
            var rb = RequestBuilder("HEAD", std::replace(&mut u, URL()));
            rb.timeout(self.default_timeout_secs);
            return self.request(&rb);
        }
    }

}
