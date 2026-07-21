// ===== HTTP request/response parsers and URL utilities =====
public namespace http {

    // URL decode (percent-decoding)
    public func url_decode(in_s: &std::string_view) : std::string {
        var out = std::string();
        var i = 0u;
        while(i < in_s.size()) {
            var c = in_s.get(i);
            if(c == '%') {
                if(i + 2 < in_s.size()) {
                    var hi = in_s.get(i+1u); var lo = in_s.get(i+2u);
                    var hv = hex_value(hi); var lv = hex_value(lo);
                    if(hv >= 0 && lv >= 0) {
                        var val = (hv * 16 + lv) as int;
                        out.append(val as char);
                        i = i + 3u;
                        continue;
                    }
                }
                out.append('%'); i++;
            } else if(c == '+') { out.append(' '); i++ } else { out.append(c); i++ }
        }
        return out;
    }

    public func hex_value(c: char) : int {
        if(c >= '0' && c <= '9') return (c - '0') as int;
        if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int;
        if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int;
        return -1
    }

    // Parse query string into simple vector of pairs
    public func parse_query(qs: &std::string_view) : std::vector<std::pair<std::string,std::string>> {
        var out = std::vector<std::pair<std::string,std::string>>();
        var i = 0u; var start = 0u;
        while(i <= qs.size()) {
            if(i == qs.size() || qs.get(i) == '&') {
                var eq = start;
                var found = false;
                while(eq < i) { if(qs.get(eq) == '=') { found = true; break } eq++ }
                if(found) {
                    var k = qs.subview(start, eq);
                    var v = qs.subview(eq + 1u, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(&k), second : url_decode(&v) });
                } else if(i > start) {
                    var k2 = qs.subview(start, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(&k2), second : std::string() });
                }
                start = i + 1u;
            }
            i++;
        }
        return out;
    }

    // Incremental request reader
    public func read_request_incremental(s: net::Socket, buf: &mut net::Buffer, header_timeout_secs: long, max_header_bytes: usize, max_headers: uint) : std::Option<Request> {
        net::set_recv_timeout(s, header_timeout_secs, 0);
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
                var req_opt = parse_request_from_bytes(ptr, crlfpos, s);
                if(req_opt is std::Option.Some) {
                    buf.consume(crlfpos);
                    return std::Option.Some<Request>(req_opt.take());
                } else { return std::Option.None<Request>() }
            }
            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
            var tmp : [DEFAULT_READ_BUF]u8;
            var n = net::recv_all(s, &raw mut tmp[0], DEFAULT_READ_BUF);
            if(n <= 0) { return std::Option.None<Request>() }
            buf.append_bytes(&raw mut tmp[0], n as usize);
            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
        }
    }

    // Parse request from raw bytes
    public func parse_request_from_bytes(buf:*u8, n: usize, s: net::Socket) : std::Option<Request> {
        var crlf_pos = -1;
        var j = 0u;
        while(j + 1 < n) {
            if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
            j++;
        }
        if(crlf_pos == -1) { return std::Option.None<Request>() }

        var sp1 = -1; var sp2 = -1; var k = 0u;
        while(k < (crlf_pos as usize)) {
            if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
            else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
            k++
        }
        if(sp1 == -1 || sp2 == -1) { return std::Option.None<Request>() }

        var req = Request();
        req.method = std::string::constructor((buf + 0) as *char, (sp1 as usize) as size_t);
        var full_path = std::string::constructor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);

        var q_pos = -1;
        var qi = 0u;
        while(qi < full_path.size()) {
            if(full_path.get(qi) == '?') { q_pos = qi as int; break }
            qi++;
        }

        if(q_pos != -1) {
            req.path = full_path.substring(0u, q_pos as usize);
            var qs = full_path.substring((q_pos as usize) + 1u, full_path.size());
            req.query = QueryMap.make_from_qs(qs.to_view());
        } else {
            req.path = full_path;
            req.query = QueryMap();
        }

        req.proto = std::string::constructor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

        var line_start = (crlf_pos as usize) + 2;
        var header_count = 0u;
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
                if(vstart < line_end && buf[vstart] == ' ' as u8) { vstart = vstart + 1 }
                var value = std::string::constructor((buf + vstart) as *char, (line_end - vstart) as size_t);
                req.headers.insert(name, value);
                header_count = header_count + 1u;
            }
            line_start = line_end + 2;
            if(header_count > 512u) { return std::Option.None<Request>() }
        }

        var cl_opt = req.headers.get("Content-Length");
        if(cl_opt is std::Option.Some) {
            var Some(v) = cl_opt else unreachable;
            var val = 0u; var ii = 0u;
            while(ii < v.size()) { var c = v.get(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
            req.body_len = val;
        }

        req.remote = std::string::make_no_len("127.0.0.1" as *char);
        return std::Option.Some<Request>(req);
    }

    // Helper to recv from socket or TLS context
    func http_recv(s: net::Socket, tls_ctx: *mut tls::SSLContext, buf: *mut u8, cap: usize) : int {
        if(tls_ctx != null) {
            return tls::ssl_read(tls_ctx, buf, cap as i32)
        }
        return net::recv_all(s, buf, cap)
    }

    // Incremental response reader (with optional TLS support)
    public func read_response_incremental(s: net::Socket, buf: &mut net::Buffer, timeout_secs: long, max_header_bytes: usize, tls_ctx: *mut tls::SSLContext = null) : std::Option<Response> {
        if(s != 0) {
            net::set_recv_timeout(s, timeout_secs, 0);
        }
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
                    var body_len: isize = -1;
                    var chunked = false;
                    if(res.body_len > 0u) { body_len = res.body_len as isize; }
                    var te_opt = res.headers.get("Transfer-Encoding");
                    if(te_opt is std::Option.Some) {
                        var Some(te) = te_opt else unreachable;
                        if(te.equals_with_len("chunked", 7)) { chunked = true; body_len = -1; }
                    }
                    res.body = Body.make_body(s, buf as *mut net::Buffer, body_len, chunked, timeout_secs * 4, 100u * 1024u * 1024u);
                    if(tls_ctx != null) {
                        res.body.tls_ctx = tls_ctx
                    }
                    return std::Option.Some<Response>(std::replace(&mut res, Response()));
                } else { return std::Option.None<Response>() }
            }
            if(buf.len() > max_header_bytes) { return std::Option.None<Response>() }
            var tmp : [DEFAULT_READ_BUF]u8;
            var n = http_recv(s, tls_ctx, &raw mut tmp[0], DEFAULT_READ_BUF);
            if(n <= 0) { return std::Option.None<Response>() }
            buf.append_bytes(&raw mut tmp[0], n as usize);
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
