/** TODO:
// ----------------------
// HTTP client
// ----------------------
public namespace http_client {

    // --- small helpers and types ---
    public struct Response {
        var status: uint;
        var reason: std::string;
        var headers: http.HeaderMap;        // reuse your server HeaderMap
        var body: std::vector<u8>;
        @constructor func constructor() {
            status = 0u;
            reason = std::string::empty_str();
            headers = http.HeaderMap();
            body = std::vector<u8>();
        }
        func body_as_string(&self) : std::string {
            var s = std::string::empty_str();
            var i = 0u;
            while(i < self.body.size()) {
                s.append(self.body.get(i) as char);
                i = i + 1u;
            }
            return s;
        }
    }

    // Simple URL parse result (supports http only)
    public struct Url {
        var scheme: std::string;
        var host: std::string;
        var port: uint;
        var path: std::string;
        @constructor func constructor() { scheme = std::string::empty_str(); host = std::string::empty_str(); port = 80u; path = std::string::make_no_len("/"); }
    }

    // --- tiny URL parser (supports "http://host[:port]/path" and "/path" with host provided) ---
    public func parse_url(u: *char) : std::Option<Url> {
        // naive parse, enough for common cases
        if(u == null) { return std::Option.None<Url>() }
        var s = std::string::make_no_len(u);
        var out = Url();
        // scheme
        var i = 0u;
        while(i < s.size()) {
            if(s.get(i) == ':' ) { break }
            i = i + 1u;
        }
        if(i + 2u < s.size() && s.get(i) == ':' && s.get(i+1u) == '/' && s.get(i+2u) == '/') {
            out.scheme = s.substring(0, i);
            var host_start = i + 3u;
            // host[:port]
            var host_end = host_start;
            while(host_end < s.size() && s.get(host_end) != '/' ) { host_end = host_end + 1u }
            // find colon for port
            var colon = host_start;
            var found_colon = false;
            while(colon < host_end) {
                if(s.get(colon) == ':') { found_colon = true; break }
                colon = colon + 1u;
            }
            if(found_colon) {
                out.host = s.substring(host_start, colon);
                var port_s = s.substring(colon + 1u, host_end);
                // parse port
                var pv = 0u;
                var pi = 0u;
                while(pi < port_s.size()) {
                    var c = port_s.get(pi);
                    if(c >= '0' && c <= '9') { pv = pv * 10u + (c as uint - '0' as uint); }
                    pi = pi + 1u;
                }
                if(pv == 0u) { pv = 80u }
                out.port = pv;
            } else {
                out.host = s.substring(host_start, host_end);
                out.port = 80u;
            }
            // path
            if(host_end < s.size()) { out.path = s.substring(host_end, s.size()) } else { out.path = std::string::make_no_len("/") }
            return std::Option.Some<Url>(out);
        } else {
            // no scheme -> treat as path-only (caller must provide Host header)
            out.path = s;
            return std::Option.Some<Url>(out);
        }
    }

    // --- tiny connect helper ---
    // This tries inet_pton for an IPv4 numeric host first, otherwise falls back to getaddrinfo when available.
    // Requires these wrappers in net module:
    //   - sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)
    //   - (on POSIX/Windows) connect / sock_connect wrapper
    //   - inet_pton() (already in your net)
    //   - sock_getaddrinfo()/sock_freeaddrinfo() (optional, used if present)
    //
    // If your net module doesn't expose connect/getaddrinfo, add platform externs:
    // POSIX:
    //   @extern public func connect(s:int, addr:*char, addrlen:int): int;
    // Windows:
    //   @dllimport @stdcall @extern public func connect(s:uintptr_t, name:*char, namelen:int): int;
    //
    // Then in net provide:
    //   public func sock_connect(s: Socket, addr:*char, addrlen:int): int { return connect(s as int, addr, addrlen) }
    //
    public func connect_tcp(host: &std::string, port: uint, timeout_secs: long) : std::Option<net.Socket> {
        // numeric IPv4 attempt
        var s = net::sock_socket(net.AF_INET as int, net.SOCK_STREAM as int, net.IPPROTO_TCP as int);
        if(s == 0 as net::Socket) {
            return std::Option.None<net.Socket>()
        }

        // prepare sockaddr_in
        var addr = net.sockaddr_in{
            sin_family: (net.AF_INET as u16),
            sin_port: net.htons_port(port as u16),
            sin_addr: net.in_addr{ s_addr: 0u },
            sin_zero: ['\0','\0','\0','\0','\0','\0','\0','\0']
        };

        // try inet_pton numeric
        var ip_ok = false;
        var ret = inet_pton(net.AF_INET as int, host.data(), &addr.sin_addr.s_addr as *mut char);
        if(ret == 1) { ip_ok = true }

        // if it's not numeric, try getaddrinfo if available
        if(!ip_ok) {
            // attempt simple getaddrinfo approach if your net exposes sock_getaddrinfo
            // We expect: net.sock_getaddrinfo(node:*char, service:*char, hints:*mut char, res:*mut *mut char)
            // And that res is a list of addrinfo; to keep things simple we attempt one sockaddr_in style bind (like earlier in server)
            var port_s = std::string::empty_str(); port_s.append_uinteger(port as ubigint);
            // try to call net.sock_getaddrinfo (if it exists)
            var res_ptr : *mut char = null;
            var gi_r = net.sock_getaddrinfo(host.data(), port_s.data(), null, &mut res_ptr);
            if(gi_r == 0 && res_ptr != null) {
                // VERY naive: assume res_ptr points to an ai struct whose ai_addr points to sockaddr_in
                // For safety: copy the first sockaddr_in from res_ptr memory region.
                // This is brittle across ABIs; if this path fails for you, prefer using inet_pton or add robust addrinfo bindings to net.
                // We'll attempt to read an in-place sockaddr_in at offset 24 (best-effort guess) OR simply fallback to closing socket.
                // Safer: iterate until you find an ai with family AF_INET; but without typed addrinfo it's fragile.
                // So we'll simply close and return None — caller should provide numeric IP if getaddrinfo ABI is unknown.
                net.sock_freeaddrinfo(res_ptr);
                net.sock_close(s);
                return std::Option.None<net.Socket>();
            } else {
                // getaddrinfo not available or failed -> bail (caller should pass numeric IP)
                net.sock_close(s);
                return std::Option.None<net.Socket>();
            }
        }

        // now connect using the sockaddr_in we filled
        // we must call connect syscall — we expect net.sock_connect wrapper to exist
        var con_res = net.sock_connect(s, &addr as *char, sizeof(net.sockaddr_in) as int);
        if(con_res != 0) {
            net.sock_close(s);
            return std::Option.None<net.Socket>();
        }

        // optionally set recv timeout (header read/timeouts) if desired
        if(timeout_secs > 0) { net.set_recv_timeout(s, timeout_secs as long, 0) }

        return std::Option.Some<net.Socket>(s);
    }

    // --- recv helpers (reads until exactly len bytes or until EOF) ---
    public func recv_exact(s: net.Socket, out_buf: *mut u8, len: usize) : int {
        var got = 0;
        while(got < len) {
            var n = net.recv_all(s, out_buf + got, len - got);
            if(n <= 0) { return got } // EOF or error
            got = got + (n as int);
        }
        return got;
    }

    // read until CRLFCRLF found (returns bytes read into buffer vector)
    public func read_until_headers(s: net.Socket, buf: &mut std::vector<u8>, timeout_secs: long) : std::Option<usize> {
        var local_tmp : [8192]u8;
        while(true) {
            // scan for \r\n\r\n
            var i = 0u;
            while(i + 3u < buf.size()) {
                if(buf.get(i) == '\r' as u8 && buf.get(i+1u) == '\n' as u8 && buf.get(i+2u) == '\r' as u8 && buf.get(i+3u) == '\n' as u8) {
                    return std::Option.Some<usize>(i + 4usize);
                }
                i = i + 1u;
            }
            // read more
            var n = net.recv_all(s, &mut local_tmp[0], sizeof(local_tmp));
            if(n <= 0) { return std::Option.None<usize>() }
            var j = 0u;
            while(j < (n as usize)) { buf.push_back(local_tmp[j]); j = j + 1u }
        }
    }

    // parse response headers block (status line + headers) from bytes (0..hdr_len)
    public func parse_response_headers(bytes: *u8, hdr_len: usize) : std::Option<std::pair<uint, http.HeaderMap>> {
        // find first CRLF (end of status line)
        var i = 0u;
        var status_line_end = 0u;
        while(i + 1u < hdr_len) {
            if(bytes[i] == '\r' as u8 && bytes[i+1u] == '\n' as u8) { status_line_end = i; break }
            i = i + 1u;
        }
        if(status_line_end == 0u) { return std::Option.None<std::pair<uint, http.HeaderMap>>() }
        // parse status code from "HTTP/1.1 200 Reason"
        // find space after protocol
        var sp = 0u; i = 0u;
        while(i < status_line_end) {
            if(bytes[i] == ' ' as u8) { sp = i; break }
            i = i + 1u;
        }
        if(sp == 0u) { return std::Option.None<std::pair<uint, http.HeaderMap>>() }
        // next space after status code
        var sp2 = sp + 1u;
        while(sp2 < status_line_end && bytes[sp2] != ' ' as u8) { sp2 = sp2 + 1u }
        // parse code as integer
        var code = 0u;
        var p = sp + 1u;
        while(p < sp2) { code = code * 10u + ((bytes[p] - '0' as u8) as uint); p = p + 1u }
        // headers parsing
        var hm = http.HeaderMap();
        var line_start = status_line_end + 2u;
        while(line_start + 1u < hdr_len) {
            // find CRLF
            var line_end = line_start;
            while(line_end + 1u < hdr_len) {
                if(bytes[line_end] == '\r' as u8 && bytes[line_end+1u] == '\n' as u8) { break }
                line_end = line_end + 1u;
            }
            if(line_end == line_start) { break } // empty -> end
            // find ':'
            var colon = line_start;
            var found_colon = false;
            while(colon < line_end) { if(bytes[colon] == ':' as u8) { found_colon = true; break } colon = colon + 1u }
            if(found_colon) {
                var name = std::string::constructor((bytes + line_start) as *char, (colon - line_start) as size_t);
                var vstart = colon + 1u;
                if(vstart < line_end && bytes[vstart] == ' ' as u8) { vstart = vstart + 1u }
                var value = std::string::constructor((bytes + vstart) as *char, (line_end - vstart) as size_t);
                hm.insert(name, value);
            }
            line_start = line_end + 2u;
        }
        return std::Option.Some<std::pair<uint, http.HeaderMap>>(std::pair<uint,http.HeaderMap>{ first: code, second: hm });
    }

    // read body according to headers
    public func read_body_into(s: net.Socket, headers: &http.HeaderMap, initial_buf: &mut std::vector<u8>, initial_consumed: usize) : std::Option<std::vector<u8>> {
        // initial_buf contains header+maybe some body data; initial_consumed tells how many bytes are header bytes consumed (so leftover = initial_buf.size()-initial_consumed)
        var out_body = std::vector<u8>();
        // leftover
        var i = initial_consumed;
        while(i < initial_buf.size()) { out_body.push_back(initial_buf.get(i)); i = i + 1u }

        // check Transfer-Encoding: chunked
        var te_opt = headers.get("Transfer-Encoding");
        if(te_opt is std::Option.Some) {
            var Some(te) = te_opt else unreachable;
            // rough check for 'chunked' token presence
            var ti = 0u; var is_chunked = false;
            while(ti < te.size()) { if(te.get(ti) == 'c' || te.get(ti) == 'C') { // try to find substring "chunked"
                        // naive substring
                        var sub = 0u; var matched = true;
                        var j = 0u;
                        var pattern = std::string::make_no_len("chunked");
                        while(j < pattern.size()) {
                            if(ti + j >= te.size() || (te.get(ti + j) | 0x20) != (pattern.get(j) | 0x20)) { matched = false; break }
                            j = j + 1u;
                        }
                        if(matched) { is_chunked = true; break }
                    }
                    ti = ti + 1u; }
            if(is_chunked) {
                // parse chunks from socket (we may already have some bytes in out_body, but they are raw chunk-data only if we already consumed header bytes correctly)
                // we'll keep a small read buffer and parse line-by-line
                var tmp : [8192]u8;
                var carry = std::vector<u8>(); // carry buffer for parsing across recv boundaries
                // move current out_body into carry to start (it may include partial chunk lines)
                var ii = 0u;
                while(ii < out_body.size()) { carry.push_back(out_body.get(ii)); ii = ii + 1u }
                out_body.clear();

                // helper to read until CRLF in carry, reading from socket as needed
                func read_line_from_carry() : std::Option<std::string> {
                    while(true) {
                        // search
                        var k = 0u;
                        while(k + 1u < carry.size()) {
                            if(carry.get(k) == '\r' as u8 && carry.get(k+1u) == '\n' as u8) {
                                // form string for line without CRLF
                                var s = std::string::constructor(&carry.get_ptr(0)[0] as *char, k as size_t);
                                // drop consumed bytes
                                var newc = std::vector<u8>();
                                var p = k + 2u;
                                while(p < carry.size()) { newc.push_back(carry.get(p)); p = p + 1u }
                                carry = newc;
                                return std::Option.Some<std::string>(s);
                            }
                            k = k + 1u;
                        }
                        // need more data from socket
                        var n = net.recv_all(s, &mut tmp[0], sizeof(tmp));
                        if(n <= 0) { return std::Option.None<std::string>() }
                        var q = 0u;
                        while(q < (n as usize)) { carry.push_back(tmp[q]); q = q + 1u }
                    }
                }

                // chunk loop
                while(true) {
                    var line_opt = read_line_from_carry();
                    if(line_opt is std::Option.None) { return std::Option.None<std::vector<u8>>() }
                    var Some(line) = line_opt else unreachable;
                    // parse chunk size as hex (line may contain extensions; parse until ';' or end)
                    var sz = 0usize; var p2 = 0u;
                    while(p2 < line.size()) {
                        var c = line.get(p2);
                        if(c == ';') { break }
                        sz = sz * 16usize + (hex_digit_value(c) as usize);
                        p2 = p2 + 1u;
                    }
                    if(sz == 0usize) {
                        // consume trailer headers until empty line
                        while(true) {
                            var tl_opt = read_line_from_carry();
                            if(tl_opt is std::Option.None) { return std::Option.None<std::vector<u8>>() }
                            var Some(tl) = tl_opt else unreachable;
                            if(tl.size() == 0) { break }
                            // we could parse trailer headers, but we ignore them
                        }
                        break; // done
                    }
                    // ensure we have sz bytes in carry; read from socket until we do
                    while(carry.size() < sz + 2u) {
                        var n2 = net.recv_all(s, &mut tmp[0], sizeof(tmp));
                        if(n2 <= 0) { return std::Option.None<std::vector<u8>>() }
                        var qq = 0u;
                        while(qq < (n2 as usize)) { carry.push_back(tmp[qq]); qq = qq + 1u }
                    }
                    // append chunk data (first sz bytes)
                    var r = 0u;
                    while(r < sz) {
                        out_body.push_back(carry.get(r));
                        r = r + 1u;
                    }
                    // drop chunk + trailing CRLF
                    var newv = std::vector<u8>();
                    var start = sz + 2u;
                    while(start < carry.size()) { newv.push_back(carry.get(start)); start = start + 1u }
                    carry = newv;
                }

                return std::Option.Some<std::vector<u8>>(out_body);
            }
        }

        // check Content-Length header
        var cl_opt = headers.get("Content-Length");
        if(cl_opt is std::Option.Some) {
            var Some(clv) = cl_opt else unreachable;
            var total = 0usize;
            var ii2 = 0u;
            while(ii2 < clv.size()) {
                var c = clv.get(ii2);
                if(c >= '0' && c <= '9') { total = total * 10usize + (c as usize - '0' as usize) }
                ii2 = ii2 + 1u;
            }
            // we already have 'out_body.size()' bytes; read the remaining
            var need = total - out_body.size();
            if(need > 0) {
                // read exactly need bytes
                var tmpbuf = std::vector<u8>();
                tmpbuf.reserve(need);
                var tmp2 : [8192]u8;
                var got = 0usize;
                while(got < need) {
                    var toread = if(need - got < sizeof(tmp2) as usize) (need - got) else sizeof(tmp2) as usize;
                    var nr = net.recv_all(s, &mut tmp2[0], toread);
                    if(nr <= 0) { break }
                    var q = 0u;
                    while(q < (nr as usize)) { out_body.push_back(tmp2[q]); q = q + 1u }
                    got = got + (nr as usize);
                }
            }
            return std::Option.Some<std::vector<u8>>(out_body);
        }

        // fallback: read until connection close
        var tmp3 : [8192]u8;
        while(true) {
            var nr = net.recv_all(s, &mut tmp3[0], sizeof(tmp3));
            if(nr <= 0) { break }
            var q2 = 0u;
            while(q2 < (nr as usize)) { out_body.push_back(tmp3[q2]); q2 = q2 + 1u }
        }
        return std::Option.Some<std::vector<u8>>(out_body);
    }

    public func hex_digit_value(c: char) : int {
        if(c >= '0' && c <= '9') return (c - '0') as int;
        if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int;
        if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int;
        return 0;
    }

    // --- top-level request API ---

    // RequestBuilder: build and send
    public struct RequestBuilder {
        var method: std::string;
        var url: std::string;
        var headers: http.HeaderMap;
        var body: std::vector<u8>;
        var timeout_secs: long;
        @constructor func constructor(m: *char, url_s: *char) {
            method = std::string::make_no_len(m);
            url = std::string::make_no_len(url_s);
            headers = http.HeaderMap();
            body = std::vector<u8>();
            timeout_secs = 5;
        }
        func set_header(&mut self, k: *char, v: *char) {
            headers.insert(std::string::make_no_len(k), std::string::make_no_len(v));
        }
        func set_timeout(&mut self, secs: long) { timeout_secs = secs }
        func body_bytes(&mut self, data:*u8, n: usize) {
            var i = 0u;
            while(i < n) { body.push_back(data[i]); i = i + 1u }
        }
        func body_string(&mut self, s: *char) {
            var tmp = std::string::make_no_len(s);
            var i = 0u;
            while(i < tmp.size()) { body.push_back(tmp.get(i) as u8); i = i + 1u }
        }

        func send(&mut self) : std::Option<Response> {
            // parse URL
            var purl_opt = parse_url(self.url.data());
            if(purl_opt is std::Option.None) { return std::Option.None<Response>() }
            var Some(purl) = purl_opt else unreachable;

            // connect
            var cs_opt = connect_tcp(&purl.host, purl.port, timeout_secs);
            if(cs_opt is std::Option.None) { return std::Option.None<Response>() }
            var Some(cs) = cs_opt else unreachable;

            // build request bytes
            var out = std::string::empty_str();
            out.append_string(self.method);
            out.append_view(std::string_view(" "));
            out.append_string(purl.path);
            out.append_view(std::string_view(" HTTP/1.1\r\n"));

            // Host header
            var host_hdr = std::string::empty_str();
            host_hdr.append_string(purl.host);
            if(purl.port != 80u) {
                host_hdr.append_view(std::string_view(":"));
                var ps = std::string::empty_str(); ps.append_uinteger(purl.port as ubigint); host_hdr.append_string(ps);
            }
            out.append_view(std::string_view("Host: ")); out.append_string(host_hdr); out.append_view(std::string_view("\r\n"));

            // Default Connection: close for simplicity
            out.append_view(std::string_view("Connection: close\r\n"));

            // Other headers from builder
            var i = 0u;
            while(i < headers.headers.size()) {
                var p = *headers.headers.get_ptr(i);
                out.append_string(p.first);
                out.append_view(std::string_view(": "));
                out.append_string(p.second);
                out.append_view(std::string_view("\r\n"));
                i = i + 1u;
            }

            // Content-Length if body present
            if(body.size() > 0) {
                var cl = std::string::empty_str();
                cl.append_uinteger(body.size() as ubigint);
                out.append_view(std::string_view("Content-Length: "));
                out.append_string(cl);
                out.append_view(std::string_view("\r\n"));
            }
            // end headers
            out.append_view(std::string_view("\r\n"));

            // send headers + body (single send_all for headers then send_all for body vector content)
            net.send_all(cs, out.data(), out.size() as int);
            if(body.size() > 0) {
                // send body bytes
                var j = 0u;
                while(j < body.size()) {
                    // gather chunk into small stack buffer for send_all
                    var buf_sz = 4096u;
                    var buf_tmp : [4096]u8;
                    var chunk = 0u;
                    while(chunk < buf_sz && j < body.size()) {
                        buf_tmp[chunk] = body.get(j); chunk = chunk + 1u; j = j + 1u;
                    }
                    net.send_all(cs, &buf_tmp[0] as *char, chunk as int);
                }
            }

            // receive response headers
            var hdr_buf = std::vector<u8>();
            hdr_buf.reserve(4096usize);
            var hdr_len_opt = read_until_headers(cs, &mut hdr_buf, timeout_secs);
            if(hdr_len_opt is std::Option.None) { net.close_socket(cs); return std::Option.None<Response>() }
            var Some(hdr_len) = hdr_len_opt else unreachable;

            // parse headers and status
            // we need to provide a pointer to hdr_buf data (ensure contiguous)
            var hdr_ptr = if(hdr_buf.size() > 0) (&hdr_buf.get_ptr(0)[0]) else null;
            var parsed = parse_response_headers(hdr_ptr, hdr_len);
            if(parsed is std::Option.None) { net.close_socket(cs); return std::Option.None<Response>() }
            var Some(pairv) = parsed else unreachable;
            var status_code = pairv.first;
            var hmap = pairv.second;

            // read body (initial bytes = hdr_buf.size() - hdr_len)
            var body_opt = read_body_into(cs, &hmap, &mut hdr_buf, hdr_len);
            if(body_opt is std::Option.None) { net.close_socket(cs); return std::Option.None<Response>() }
            var Some(body_v) = body_opt else unreachable;

            // construct Response
            var resp = Response();
            resp.status = status_code;
            resp.headers = hmap;
            resp.body = body_v;

            net.close_socket(cs);
            return std::Option.Some<Response>(resp);
        }
    }

    // convenience short functions
    public func get(url: *char) : std::Option<Response> {
        var rb = RequestBuilder("GET", url);
        return rb.send();
    }
    public func post(url: *char, body: *char) : std::Option<Response> {
        var rb = RequestBuilder("POST", url);
        rb.body_string(body);
        rb.set_header("Content-Type", "text/plain");
        return rb.send();
    }

} // namespace http_client
**/