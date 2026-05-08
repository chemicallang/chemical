public namespace http {

    public struct Response {
        var status: uint;
        var status_text: std::string;
        var proto: std::string;
        var headers: HeaderMap;
        var body_len: usize;
        var body: Body;

        @constructor func constructor() {
            return Response {
                status = 200u;
                status_text = std::string();
                proto = std::string();
                headers = HeaderMap();
                body_len = 0u;
                body : Body()
            }
        }
    }

    public func read_response_incremental(s: net::Socket, buf: &mut io::Buffer, timeout_secs: long, max_header_bytes: usize) : std::Option<Response> {
        net::set_recv_timeout(s, timeout_secs, 0);
        loop {
            var i = 0u; var found = false; var crlfpos = 0u;
            while(i + 3 < buf.len()) {
                if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i+1u) == '\n' as u8 &&
                   buf.get_byte(i+2u) == '\r' as u8 && buf.get_byte(i+3u) == '\n' as u8) {
                    found = true;
                    crlfpos = i + 4u;
                    break;
                }
                i++;
            }
            if(found) {
                var ptr = buf.as_ptr();
                var res_opt = parse_response_from_bytes(ptr, crlfpos, s);
                if(res_opt is std::Option.Some) {
                    buf.consume(crlfpos);
                    var Some(res) = res_opt else unreachable;
                    // attach body source
                    var body_len: isize = -1;
                    var chunked = false;
                    if(res.body_len > 0u) { body_len = res.body_len as isize; }
                    var te_opt = res.headers.get("Transfer-Encoding");
                    if(te_opt is std::Option.Some) {
                        var Some(te) = te_opt else unreachable;
                        if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
                    }
                    res.body = http.Body.make_body(s, buf as *mut io::Buffer, body_len, chunked, timeout_secs * 4, 100u * 1024u * 1024u);
                    return std::Option.Some<Response>(std::replace(res, Response()));
                } else { return std::Option.None<Response>() }
            }
            if(buf.len() > max_header_bytes) { return std::Option.None<Response>() }
            var tmp : [DEFAULT_READ_BUF]u8;
            var n = net::recv_all(s, &mut tmp[0], DEFAULT_READ_BUF);
            if(n <= 0) { return std::Option.None<Response>() }
            buf.append_bytes(&mut tmp[0], n as usize);
        }
    }

    func parse_response_from_bytes(buf:*u8, n: usize, s: net::Socket) : std::Option<Response> {
        var crlf_pos = -1;
        var j = 0u;
        while(j + 1 < n) {
            if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
            j++;
        }
        if(crlf_pos == -1) { return std::Option.None<Response>() }

        // proto SP status SP status_text
        var sp1 = -1; var sp2 = -1; var k = 0u;
        while(k < (crlf_pos as usize)) {
            if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
            else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
            k++
        }
        if(sp1 == -1 || sp2 == -1) { return std::Option.None<Response>() }

        var res = Response();
        res.proto = std::string::constructor((buf + 0) as *char, (sp1 as usize) as size_t);
        var status_str = std::string::constructor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);
        
        var status_val = 0u;
        for(var i=0u; i<status_str.size(); i++) {
            var c = status_str.get(i);
            if(c >= '0' && c <= '9') { status_val = status_val * 10u + (c as uint - '0' as uint) }
        }
        res.status = status_val;
        res.status_text = std::string::constructor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

        var line_start = (crlf_pos as usize) + 2;
        while(line_start + 1 < n) {
            var line_end = line_start;
            while(line_end + 1 < n) {
                if(buf[line_end] == '\r' as u8 && buf[line_end+1] == '\n' as u8) { break }
                line_end = line_end + 1
            }
            if(line_end >= n) { break }
            if(line_end == line_start) { break }

            var colon = -1; var p = line_start;
            while(p < line_end) {
                if(buf[p] == ':' as u8) { colon = p as int; break }
                p = p + 1
            }
            if(colon != -1) {
                var name = std::string::constructor((buf + line_start) as *char, ((colon as usize) - line_start) as size_t);
                var vstart = (colon as usize) + 1;
                while(vstart < line_end && buf[vstart] == ' ' as u8) { vstart = vstart + 1 }
                var value = std::string::constructor((buf + vstart) as *char, (line_end - vstart) as size_t);
                res.headers.insert(name, value);
            }
            line_start = line_end + 2;
        }

        var cl_opt = res.headers.get("Content-Length");
        if(cl_opt is std::Option.Some) {
            var Some(v) = cl_opt else unreachable;
            var val = 0u; var ii = 0u;
            while(ii < v.size()) { var c = v.get(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
            res.body_len = val;
        }
        return std::Option.Some<Response>(res);
    }
}

public namespace net {

    public struct Client {
        var timeout_secs: long;

        @constructor func constructor() {
            return Client { timeout_secs : 10 }
        }

        func request(&self, method: &std::string_view, url: &std::string_view, body: &std::string_view = "", content_type: &std::string_view = "") : std::Result<http::Response, std::string> {
            // parse simple URL: http://host:port/path
            var host = std::string();
            var port = 80u;
            var path = std::string();

            var u = *url;
            if(u.starts_with("http://")) { u = u.skip(7u) }
            
            var slash = u.find("/");
            var host_port = std::string();
            if(slash == std::NPOS) {
                host_port = std::string::view_make(u);
                path.append('/');
            } else {
                host_port = std::string::view_make(u.subview(0u, slash));
                path = std::string::view_make(u.subview(slash, u.size()));
            }

            var colon = host_port.find(":");
            if(colon == -1u) {
                host = host_port;
            } else {
                host = host_port.substring(0u, colon);
                var pstr = host_port.substring(colon + 1u, host_port.size());
                var pval = 0u;
                for(var i=0u; i<pstr.size(); i++) {
                    var c = pstr.get(i);
                    if(c >= '0' && c <= '9') { pval = pval * 10u + (c as uint - '0' as uint) }
                }
                if(pval > 0u) { port = pval }
            }

            var s = net::dial(host.data(), port);
            if(s == 0u || (s as longlong) < 0) { return std::Result.Err<http::Response, std::string>(std::string::make_no_len("failed to connect")) }

            var req_str = std::string();
            req_str.append_view(method);
            req_str.append(' ');
            req_str.append_string(path);
            req_str.append_view(" HTTP/1.1\r\n");
            req_str.append_view("Host: ");
            req_str.append_string(host);
            req_str.append_view("\r\n");
            
            if(body.size() > 0u) {
                req_str.append_view("Content-Length: ");
                req_str.append_uinteger(body.size() as ubigint);
                req_str.append_view("\r\n");
                if(content_type.size() > 0u) {
                    req_str.append_view("Content-Type: ");
                    req_str.append_view(content_type);
                    req_str.append_view("\r\n");
                }
            }
            req_str.append_view("Connection: close\r\n\r\n");
            if(body.size() > 0u) {
                req_str.append_view(body);
            }

            net::send_all(s, req_str.data(), req_str.size() as int);

            var buf = io::Buffer();
            var res_opt = http::read_response_incremental(s, buf, self.timeout_secs, 64u * 1024u);
            if(res_opt is std::Option.None) {
                net::close_socket(s);
                return std::Result.Err<http::Response, std::string>(std::string::make_no_len("failed to read response"))
            }

            return std::Result.Ok<http::Response, std::string>(res_opt.take());
        }

        public func get(&self, url: &std::string_view) : std::Result<http::Response, std::string> {
            return request("GET", url);
        }

        public func post(&self, url: &std::string_view, body: &std::string_view, content_type: &std::string_view) : std::Result<http::Response, std::string> {
            return request("POST", url, body, content_type);
        }
    }

}
