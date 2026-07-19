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

            if(u.starts_with("http://")) {
                res.scheme = std::string::make_no_len("http");
                u = u.skip(7u);
            } else if(u.starts_with("https://")) {
                res.scheme = std::string::make_no_len("https");
                res.port = 443u;
                u = u.skip(8u);
            }

            var slash = u.find("/");
            var host_port = std::string_view();
            if(slash == std::NPOS) {
                host_port = u;
                res.path = std::string::make_no_len("/");
            } else {
                host_port = u.subview(0u, slash);
                var full_path = u.subview(slash, u.size());
                var qmark = full_path.find("?");
                if(qmark == std::NPOS) {
                    res.path = std::string::view_make(&full_path);
                } else {
                    res.path = std::string::view_make(full_path.subview(0u, qmark));
                    res.query = std::string::view_make(full_path.subview(qmark + 1u, full_path.size()));
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

        public func build(&self) : std::string {
            var out = std::string();
            out.append_string(&method);
            out.append(' ');
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

        @constructor func constructor() {
            return Client {
                default_timeout_secs = 10,
                max_response_header_bytes = 64u * 1024u
            }
        }

        public func request(&self, req_builder: &RequestBuilder) : std::Result<Response, std::string> {
            var s = net::dial(req_builder.url.host.data(), req_builder.url.port);
            if(s == 0u || (s as longlong) < 0) {
                return std::Result.Err<Response, std::string>(std::string::make_no_len("failed to connect"));
            }

            var req_data = req_builder.build();
            net::send_all(s, req_data.data(), req_data.size() as int);

            var buf_ptr = new net::Buffer();
            var res_opt = read_response_incremental(s, &mut *buf_ptr, req_builder.timeout_secs, self.max_response_header_bytes);
            if(res_opt is std::Option.None) {
                delete buf_ptr;
                net::close_socket(s);
                return std::Result.Err<Response, std::string>(std::string::make_no_len("failed to read response"));
            }

            var res = res_opt.take();
            res.body.owns_buf = true;
            return std::Result.Ok<Response, std::string>(res);
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
