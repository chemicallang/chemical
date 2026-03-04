// ===== HTTP utilities and parser =====
public namespace http {

    public const DEFAULT_READ_BUF = 8192u;

    public struct HeaderMap {

        var headers: std::vector<std::pair<std::string,std::string>>;

        @make
        func make() {
            return HeaderMap {
                headers : std::vector<std::pair<std::string,std::string>>()
            }
        }

        func empty(&self) : bool {
            return headers.empty()
        }

        func print(&self) {
            var start = headers.data()
            const end = start + headers.size()
            printf("headers:\n");
            while(start != end) {
                printf("\theader '%s' : '%s'\n", start.first.data(), start.second.data());
            }
        }

        func insert(&mut self, k: std::string, v: std::string) {
            headers.push_back(std::pair<std::string,std::string>{ first : k, second : v })
        }

        func insert_view(&mut self, k: &std::string_view, v: &std::string_view) {
            headers.push_back(std::pair<std::string,std::string>{ first : std::string(k.data(), k.size()), second : std::string(v.data(), v.size()) })
        }

        func get(&self, name: *char) : std::Option<std::string> {
            var i = 0u;
            while(i < headers.size()) {
                var p = headers.get_ptr(i);
                // case-insensitive compare using manual loop
                if(strcasecmp(p.first.data(), name) == 0) { return std::Option.Some<std::string>(p.second.copy()) }
                i = i + 1u;
            }
            return std::Option.None<std::string>();
        }

    }

    // helper: simple ASCII case-insensitive strcmp
    public func strcasecmp(a:*char, b:*char) : int {
        var i = 0;
        loop {
            var ca = a[i]; var cb = b[i];
            if(ca == '\0' && cb == '\0') { return 0 }
            var la = ca; var lb = cb;
            if(la >= 'A' && la <= 'Z') { la = (la + 32) as char }
            if(lb >= 'A' && lb <= 'Z') { lb = (lb + 32) as char }
            if(la != lb) { return (la as int) - (lb as int) }
            i = i + 1;
        }
    }

    // Body: lazy reader for request body. Does NOT close the socket by itself.
    // It borrows a pointer to an io::Buffer containing already-read bytes (headers + maybe some body).
    @direct_init
    public struct Body {

        var sock: net::Socket;               // underlying socket (do not close here)
        var buf: *mut io::Buffer;            // pointer to buffer that holds bytes already read
        var remaining: isize;                // -1 meaning unknown (no Content-Length), otherwise bytes left
        var chunked: bool;                   // transfer-encoding: chunked
        var timeout_secs: long;              // timeout for subsequent recv calls
        var max_body: usize;                 // configured max body size (for limits)
        var closed: bool;                    // has the handler closed this Body?
        // chunked state
        var cur_chunk_left: usize;           // bytes left in current chunk when chunked==true
        var seen_total: usize;               // how many body bytes have been delivered to user (enforce max_body)

        @make
        func empty_make() {
            return Body {
                sock = 0
                buf = null
                remaining = -1
                chunked = false
                timeout_secs = 0
                max_body = 0
                closed = false
                cur_chunk_left = 0
                seen_total = 0
            }
        }

        // construct body from request metadata (call from handle_conn after parsing headers)
        public func make_body(
            sock: net::Socket,
            buf_ptr: *mut io::Buffer,
            content_len: isize,    // -1 if unknown
            chunked: bool,
            timeout_secs: long,
            max_body: usize
        ) : Body {
            return Body{
                sock: sock,
                buf: buf_ptr,
                remaining: content_len,
                chunked: chunked,
                timeout_secs: timeout_secs,
                max_body: max_body,
                closed: false,
                cur_chunk_left: 0u,
                seen_total: 0u
            };
        }

    }

    // low-level recv helper that honors timeout and returns <=0 on error/timeout
    func body_recv(b:*mut Body, dst:*mut u8, cap: usize) : int {
        // set timeout on socket
        net::set_recv_timeout(b.sock, b.timeout_secs, 0);
        return net::recv_all(b.sock, dst, cap);
    }

    // Helper: copy up to `want` bytes from the already-read buffer into dst, returning copied count.
    func copy_from_buffer(b:*mut Body, dst:*mut u8, want: usize) : usize {
        if(b.buf == null) { return 0u }
        var bufref = b.buf;
        var avail = bufref.len();
        var take = if(avail < want) avail else want;
        if(take == 0u) { return 0u }
        // copy bytes
        var i = 0u;
        var srcp = bufref.as_ptr();
        while(i < take) {
            dst[i] = srcp[i];
            i = i + 1u;
        }
        bufref.consume(take);
        return take;
    }

    // PUBLIC: read up to `cap` bytes into dst; returns number of bytes read or <=0 on error.
    // This function supports Content-Length and chunked transfer (basic).
    func (b: &mut Body) read(dst:*mut u8, cap: usize) : int {
        if(b.closed) { return 0 } // EOF for closed
        // Enforce max_body
        if(b.max_body > 0u && b.seen_total >= b.max_body) { return -1 } // too large
        // If chunked mode -> decode chunk frames
        if(b.chunked) {
            // If current chunk has data, use it first
            if(b.cur_chunk_left > 0u) {
                var want = if(cap < b.cur_chunk_left) cap else b.cur_chunk_left;
                // first copy from internal already-read buffer
                var copied = copy_from_buffer(&mut b, dst, want);
                var total_read = copied;
                // if need more and socket provides, recv more
                while(total_read < want) {
                    var n = body_recv(&mut b, &mut dst[total_read], want - total_read);
                    if(n <= 0) { return -1 }
                    total_read = total_read + (n as usize);
                }
                b.cur_chunk_left = b.cur_chunk_left - want;
                b.seen_total = b.seen_total + want;
                if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
                // if chunk finished, consume trailing CRLF from stream/buffer
                if(b.cur_chunk_left == 0u) {
                    // consume trailing \r\n after chunk data
                    var tmp : [2]u8;
                    // try to copy from buffer first
                    var c = copy_from_buffer(&mut b, &mut tmp[0], 2);
                    if(c < 2) {
                        var rem = 2 - c;
                        var n = body_recv(&mut b, &mut tmp[c], rem);
                        if(n <= 0) { return -1 }
                    }
                    // ignore values, just discard
                }
                return (want as int);
            }
            // Need to read next chunk size line: it's ASCII hex then CRLF.
            // We'll read bytes from buffer and socket until we find CRLF.
            var linebuf = std::string::empty_str();
            // first consume from buffer if present
            if(b.buf != null && b.buf.len() > 0u) {
                // attempt to read until CRLF appears in buffer
                var i = 0u;
                while(i + 1 < b.buf.len()) {
                    var a = b.buf.get_byte(i);
                    var bb = b.buf.get_byte(i + 1u);
                    if(a == '\r' as u8 && bb == '\n' as u8) {
                        // read the line from buffer
                        var nline = i; // number of bytes before CRLF
                        var ptr = b.buf.as_ptr();
                        var tmpline = std::string::constructor(ptr as *char, nline as size_t);
                        b.buf.consume(nline + 2u); // remove line + CRLF
                        linebuf = tmpline;
                        break;
                    }
                    i = i + 1u;
                }
            }
            // if linebuf still empty, read bytes from socket until CRLF
            while(linebuf.size() == 0u) {
                var tmp : [64]u8;
                var n = body_recv(&mut b, &mut tmp[0], 64);
                if(n <= 0) { return -1 }
                // append into a small temporary string searching for CRLF
                var i = 0;
                while(i < n) {
                    var ch = tmp[i];
                    linebuf.append(ch as char);
                    var L = linebuf.size();
                    if(L >= 2 && linebuf.get(L-2u) == '\r' && linebuf.get(L-1u) == '\n') {
                        // strip trailing CRLF
                        linebuf = linebuf.substring(0u, L-2u);
                        break;
                    }
                    i = i + 1;
                }
            }
            // parse hex chunk size
            var hex_ok = true;
            var hex_val : usize = 0u;
            var ii = 0u;
            while(ii < linebuf.size()) {
                var c = linebuf.get(ii);
                var v = hex_value(c);
                if(v < 0) { hex_ok = false; break }
                hex_val = (hex_val * 16u) + (v as usize);
                ii = ii + 1u;
            }
            if(!hex_ok) { return -1 }
            if(hex_val == 0u) {
                // final chunk: consume optional trailer until blank line
                // for simplicity, consume bytes until we see "\r\n\r\n"
                var found = false;
                while(!found) {
                    // check buffer first
                    if(b.buf != null) {
                        var L = b.buf.len();
                        if(L >= 4u) {
                            var i = 0u;
                            while(i + 3u < L) {
                                if(b.buf.get_byte(i) == '\r' as u8 && b.buf.get_byte(i+1u) == '\n' as u8 &&
                                   b.buf.get_byte(i+2u) == '\r' as u8 && b.buf.get_byte(i+3u) == '\n' as u8) {
                                    b.buf.consume(i + 4u);
                                    found = true;
                                    break;
                                }
                                i = i + 1u;
                            }
                        }
                    }
                    if(!found) {
                        var tmp : [DEFAULT_READ_BUF]u8;
                        var n = body_recv(&mut b, &mut tmp[0], DEFAULT_READ_BUF);
                        if(n <= 0) { return -1 }
                        // append to buffer for next iteration
                        if(b.buf != null) { b.buf.append_bytes(&mut tmp[0], n as usize) }
                    }
                }
                // EOF for chunked body
                b.closed = true;
                return 0;
            }
            // set current chunk left and loop to read from it (recursive call)
            b.cur_chunk_left = hex_val;
            // recursive to read from cur_chunk_left portion
            return b.read(dst, cap);
        } // chunked end

        // Non-chunked simple: Content-Length known or unknown.
        if(b.remaining >= 0) {
            // known length: read up to min(cap, remaining)
            var want = if((b.remaining as usize) < cap) (b.remaining as usize) else cap;
            // copy from buffer first
            var copied = copy_from_buffer(&mut b, dst, want);
            var total = copied;
            while(total < want) {
                var n = body_recv(&mut b, &mut dst[total], want - total);
                if(n <= 0) { return -1 }
                total = total + (n as usize);
            }
            b.remaining = b.remaining - (total as isize);
            b.seen_total = b.seen_total + total;
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            return (total as int);
        } else {
            // unknown length: read until peer closes (connection: close) — treat as stream
            // copy from buffer first
            var copied = copy_from_buffer(&mut b, dst, cap);
            var total = copied;
            if(total > 0u) {
                b.seen_total = b.seen_total + total;
                if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
                return (total as int);
            }
            // otherwise, read from socket
            var n = body_recv(&mut b, dst, cap);
            if(n <= 0) { return (0) } // EOF or error -> return 0 for EOF
            b.seen_total = b.seen_total + (n as usize);
            if(b.max_body > 0u && b.seen_total > b.max_body) { return -1 }
            return n;
        }
    }

    // read all remaining body into a string (useful for small bodies)
    public func (b: &mut Body) read_to_string() : std::Option<std::string> {
        var out = std::string::empty_str();
        var tmp : [DEFAULT_READ_BUF]u8;
        while(true) {
            var n = b.read(&mut tmp[0], DEFAULT_READ_BUF);
            if(n < 0) { return std::Option.None<std::string>() } // error
            if(n == 0) { break } // EOF
            out.append_view(std::string_view(&tmp[0] as *char, n as size_t))
        }
        return std::Option.Some<std::string>(out);
    }

    // read exactly `n` bytes (or return None on error/EOF)
    public func (b: &mut Body) read_exact(n: usize) : std::Option<std::string> {
        var out = std::string::empty_str();
        out.reserve(n);
        var rem = n;
        var tmp : [DEFAULT_READ_BUF]u8;
        while(rem > 0u) {
            var want = if(rem < DEFAULT_READ_BUF) rem else DEFAULT_READ_BUF;
            var r = b.read(&mut tmp[0], want);
            if(r <= 0) { return std::Option.None<std::string>() }
            var part = std::string::constructor(&tmp[0] as *char, r as size_t);
            out.append_string(part);
            rem = rem - (r as usize);
        }
        return std::Option.Some<std::string>(out);
    }

    // drain remaining body to /dev/null (useful when you want to discard the body)
    public func (b: &mut Body) drain() : bool {
        var tmp : [DEFAULT_READ_BUF]u8;
        while(true) {
            var n = b.read(&mut tmp[0], DEFAULT_READ_BUF);
            if(n < 0) { return false }
            if(n == 0) { break }
        }
        return true;
    }

    // close returns resources (no-op here except mark closed)
    public func (b: &mut Body) close() {
        b.closed = true;
    }

    public struct Request {
        var method: std::string;
        var path: std::string;
        var proto: std::string;
        var headers: HeaderMap;
        var body_len: usize;
        var remote: std::string;
        var body : Body

        @constructor func constructor() {
            return Request {
                method = std::string::empty_str();
                path = std::string::empty_str();
                proto = std::string::empty_str();
                headers = HeaderMap();
                body_len = 0u;
                remote = std::string::empty_str();
                body : Body()
            }
        }

    }

    public struct ResponseWriter {
        var sock: net::Socket;
        var headers: HeaderMap;
        var status: uint;
        var sent_headers: bool;

        @constructor func constructor(s: net::Socket) {
            return ResponseWriter {
                sock = s;
                headers = HeaderMap()
                status = 200u;
                sent_headers = false
            }
        }

        func set_header(&mut self, k: std::string, v: std::string) { headers.insert(k, v) }
        func set_header_view(&mut self, k : &std::string_view, v : &std::string_view) {
            headers.insert_view(k, v)
        }
        func send_headers(&mut self, content_len: usize) {
            if(sent_headers) { return }

            var out = std::string();
            // Status line (HTTP requires CRLF)
            out.append_view(std::string_view("HTTP/1.1 "));
            out.append_uinteger(status);
            out.append_view(std::string_view(" OK\r\n"));

            // Server header
            out.append_view(std::string_view("Server: chemical-http\r\n"));

            // Content-Length
            out.append_view(std::string_view("Content-Length: "));
            out.append_uinteger(content_len as ubigint);
            out.append_view(std::string_view("\r\n"));

            // Other headers
            var i = 0u;
            while(i < headers.headers.size()) {
                var p = headers.headers.get_ptr(i);
                out.append_string(p.first);
                out.append_view(std::string_view(": "));
                out.append_string(p.second);
                out.append_view(std::string_view("\r\n"));
                i = i + 1u;
            }

            // End of header section
            out.append_view(std::string_view("\r\n"));

            net::send_all(sock, out.data(), out.size() as int);
            sent_headers = true;
        }
        func write_string(&mut self, s: std::string) { send_headers(s.size()); net::send_all(sock, s.data(), s.size() as int) }
        func write_view(&mut self, v : &std::string_view) {
            send_headers(v.size()); net::send_all(sock, v.data(), v.size() as int)
        }

        // Adds a zero-copy send_file method to ResponseWriter.
        // Returns true on success; false means the caller should fallback to chunked streaming.
        func send_file(&mut self, path_view : &std::string_view, content_type : &std::string_view) : bool {
            // set content-type header if provided
            set_header_view(std::string_view("Content-Type"), content_type);
            const path = path_view.data()
            comptime if(def.windows) {
                // Windows: use CreateFileA + GetFileSizeEx + TransmitFile
                var fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, 0u, 0 as HANDLE);
                if (fh == 0u || fh == -1 as uintptr_t) {
                    return false;
                }
                var filesize: longlong = 0
                if (GetFileSizeEx(fh, (&mut filesize) as *mut LARGE_INTEGER) == 0) {
                    CloseHandle(fh);
                    return false;
                }
                // send headers with Content-Length
                self.send_headers(filesize as usize);

                // TransmitFile: pass 0 as nNumberOfBytesToWrite means send entire file
                var ok = net::TransmitFile(self.sock as uintptr_t, fh as uintptr_t, 0u, 0u, null, null, 0u);
                // Close handle regardless
                CloseHandle(fh);
                return ok != 0;
            } else {
                // POSIX / Linux: use open + lseek + sendfile
                var fd = open(path, O_RDONLY, 0);
                if (fd < 0) { return false; }
                var size_ll = net::lseek(fd, 0 as longlong, SEEK_END);
                if (size_ll < 0) { close(fd); return false; }
                // rewind
                if (net::lseek(fd, 0 as longlong, SEEK_SET) < 0) { close(fd); return false; }

                // send headers with Content-Length
                self.send_headers((size_ll as usize));

                var remaining = size_ll;
                var offset: longlong = 0;
                while (remaining > 0) {
                    // send up to remaining bytes
                    var to_send = if(remaining > (1 << 30)) (1 << 30) as size_t else remaining as size_t;
                    // on Linux sendfile returns number of bytes sent (ssize_t)
                    var sent = net::sendfile(self.sock as int, fd, &mut offset, to_send);
                    if (sent <= 0) {
                        // error or EOF
                        break;
                    }
                    // adjust remaining - note: sendfile may update offset; we'll recompute
                    remaining = size_ll - offset;
                }

                var success = (offset == size_ll);
                close(fd);
                return success;
            }
        }



        func finish(&mut self) {}
    }

    // URL decode (percent-decoding)
    public func url_decode(in_s: &std::string) : std::string {
        var out = std::string::empty_str();
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
                // fallthrough: append '%'
                out.append('%'); i++;
            } else if(c == '+') { out.append(' '); i++ } else { out.append(c); i++ }
        }
        return out;
    }
    public func hex_value(c: char) : int { if(c >= '0' && c <= '9') return (c - '0') as int; if(c >= 'A' && c <= 'F') return (c - 'A' + 10) as int; if(c >= 'a' && c <= 'f') return (c - 'a' + 10) as int; return -1 }

    // Parse query string into simple vector of pairs
    public func parse_query(qs: &std::string) : std::vector<std::pair<std::string,std::string>> {
        var out = std::vector<std::pair<std::string,std::string>>();
        var i = 0u; var start = 0u;
        while(i <= qs.size()) {
            if(i == qs.size() || qs.get(i) == '&') {
                // substring start..i
                var eq = start;
                var found = false;
                while(eq < i) { if(qs.get(eq) == '=') { found = true; break } eq++ }
                if(found) {
                    var k = qs.substring(start, eq);
                    var v = qs.substring(eq + 1u, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(k), second : url_decode(v) });
                } else if(i > start) {
                    var k2 = qs.substring(start, i);
                    out.push_back(std::pair<std::string,std::string>{ first : url_decode(k2), second : std::string::empty_str() });
                }
                start = i + 1u;
            }
            i++;
        }
        return out;
    }

    // Incremental request reader that reads headers with timeout and enforces limits
    public func read_request_incremental(s: net::Socket, buf: &mut io::Buffer, header_timeout_secs: long, max_header_bytes: usize, max_headers: uint) : std::Option<Request> {
        // set recv timeout
        net::set_recv_timeout(s, header_timeout_secs, 0);
        // read loop
        loop {
            // check for CRLFCRLF in buffer (i.e. "\r\n\r\n")
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
                // parse request-line and headers from buf.as_ptr()
                var ptr = buf.as_ptr();
                var req_opt = parse_request_from_bytes(ptr, crlfpos, s);
                if(req_opt is std::Option.Some) {
                    // consume header bytes
                    buf.consume(crlfpos);
                    return std::Option.Some<Request>(req_opt.take());
                } else { return std::Option.None<Request>() }
            }
            // no full header yet: read more
            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
            var tmp : [DEFAULT_READ_BUF]u8;
            var n = net::recv_all(s, &mut tmp[0], DEFAULT_READ_BUF);
            if(n <= 0) { return std::Option.None<Request>() }
            buf.append_bytes(&mut tmp[0], n as usize);

            if(buf.len() > max_header_bytes) { return std::Option.None<Request>() }
            // loop to check again
        }
    }

    // parse_request_from_bytes: same as earlier parse_request_from_buf but uses raw bytes and returns Request
    public func parse_request_from_bytes(buf:*u8, n: usize, s: net::Socket) : std::Option<Request> {
        // find the first CRLF (end of request-line)
        var crlf_pos = -1;
        var j = 0u;
        while(j + 1 < n) {
            if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
            j++;
        }
        if(crlf_pos == -1) { return std::Option.None<Request>() }

        // find spaces in request-line (method SP path SP proto)
        var sp1 = -1; var sp2 = -1; var k = 0u;
        while(k < (crlf_pos as usize)) {
            if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
            else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
            k++
        }
        if(sp1 == -1 || sp2 == -1) { return std::Option.None<Request>() }

        var req = Request();
        req.method = std::string::constructor((buf + 0) as *char, (sp1 as usize) as size_t);
        req.path = std::string::constructor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);
        req.proto = std::string::constructor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

        // headers parse: iterate lines separated by CRLF until empty line
        var line_start = (crlf_pos as usize) + 2; // skip the first CRLF
        var header_count = 0u;
        while(line_start + 1 < n) {
            // find end of this header line (CRLF)
            var line_end = line_start;
            while(line_end + 1 < n) {
                if(buf[line_end] == '\r' as u8 && buf[line_end+1] == '\n' as u8) { break }
                line_end = line_end + 1
            }
            if(line_end >= n) { break }
            // if empty line (i.e. CRLF right away) -> end of headers
            if(line_end == line_start) { break }

            // find ':' in header line
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

            line_start = line_end + 2; // advance past CRLF
            if(header_count > 512u) { return std::Option.None<Request>() }
        }

        // Content-Length if present
        var cl_opt = req.headers.get("Content-Length");
        if(cl_opt is std::Option.Some) {
            var Some(v) = cl_opt else unreachable;
            var val = 0u; var ii = 0u;
            while(ii < v.size()) { var c = v.get(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
            req.body_len = val;
        }

        // remote address placeholder
        req.remote = std::string::make_no_len("127.0.0.1" as *char);
        return std::Option.Some<Request>(req);
    }
}