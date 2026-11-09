// ===== HTTP request/response parsers and URL utilities =====
public namespace http {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

// Default maximum response body size read into memory (100MB).
public comptime const DEFAULT_MAX_BODY_LEN : usize = 100u * 1024u * 1024u

// URL decode (percent-decoding)
public func url_decode(in_s: &string_view) : string {
    var out = string();
    var i = 0u;
    while(i < in_s.size()) {
        var c = in_s.at(i);
        if(c == '%') {
            if(i + 2 < in_s.size()) {
                var hi = in_s.at(i+1u); var lo = in_s.at(i+2u);
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
public func parse_query(qs: &string_view) : vector<std::pair<string, string>> {
    var out = vector<std::pair<string, string>>();
    var i = 0u; var start = 0u;
    while(i <= qs.size()) {
        if(i == qs.size() || qs.at(i) == '&') {
            var eq = start;
            var found = false;
            while(eq < i) { if(qs.at(eq) == '=') { found = true; break } eq++ }
            if(found) {
                var k = qs.subview(start, eq);
                var v = qs.subview(eq + 1u, i);
                out.push_back(std::pair<string, string>{ first : url_decode(&k), second : url_decode(&v) });
            } else if(i > start) {
                var k2 = qs.subview(start, i);
                out.push_back(std::pair<string, string>{ first : url_decode(&k2), second : string() });
            }
            start = i + 1u;
        }
        i++;
    }
    return out;
}

// Scan for the end of a header block. Returns NPOS when incomplete.
func find_header_end(buf : *net::Buffer) : size_t { unsafe {
    var i = 0u
    while(i + 3u < buf.len()) {
        if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i+1u) == '\n' as u8 &&
           buf.get_byte(i+2u) == '\r' as u8 && buf.get_byte(i+3u) == '\n' as u8) {
            return i + 4u
        }
        i += 1u
    }
    return std::NPOS
}}

// Incremental request head reader over an owning/borrowed Transport.
func read_request_head(transport : &mut Transport, buf : *mut net::Buffer,
                              header_timeout_secs : long, max_header_bytes : usize,
                              max_headers : uint) : Result<*mut Request, HttpError> { unsafe {
    var _ = transport.set_read_timeout((header_timeout_secs * 1000) as u64)
    loop {
        var end = find_header_end(buf)
        if(end != std::NPOS) {
            var ptr = buf.as_ptr()
            var req_opt = parse_request_from_bytes(ptr, end)
            if(req_opt is Option.Some) {
                var req_value = req_opt.take_unchecked()
                buf.consume(end)
                var box = malloc(sizeof(Request)) as *mut Request
                if(box == null) {
                    return Result.Err(HttpError.Other(http_lit("out of memory")))
                }
                unsafe { memcpy(box as *mut void, &raw mut req_value as *mut void, sizeof(Request)) }
                intrinsics::forget(req_value)
                return Result.Ok<*mut Request, HttpError>(box)
            }
            return Result.Err(HttpError.Protocol(http_lit("malformed request")))
        }
        if(buf.len() > max_header_bytes) {
            return Result.Err(HttpError.HeaderTooLarge())
        }
        var tmp : [DEFAULT_READ_BUF]u8;
        var r = transport.read_bytes(unsafe(std::mut_span<u8>(&raw mut tmp[0], DEFAULT_READ_BUF as size_t)))
        if(r is Result.Err) {
            return Result.Err(HttpError.Io(http_io_error()))
        }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0u) { return Result.Err(HttpError.Protocol(http_lit("connection closed"))) }
        buf.append_bytes(&raw mut tmp[0], n)
    }
}
}

// Parse request from raw bytes. The caller supplies the pointer/length, so
// safe callers go through `read_request_head`; this is an internal helper.
func parse_request_from_bytes(buf:*u8, n: usize) : Option<Request> {
    unsafe {
    var crlf_pos = -1;
    var j = 0u;
    while(j + 1 < n) {
        if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
        j++;
    }
    if(crlf_pos == -1) { return Option.None<Request>() }

    var sp1 = -1; var sp2 = -1; var k = 0u;
    while(k < (crlf_pos as usize)) {
        if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
        else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
        k++
    }
    if(sp1 == -1 || sp2 == -1) { return Option.None<Request>() }

    var req = Request();
    req.method = http_ctor((buf + 0) as *char, (sp1 as usize) as size_t);
    var full_path = http_ctor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);

    var q_pos = -1;
    var qi = 0u;
    while(qi < full_path.size()) {
        if(full_path.at(qi) == '?') { q_pos = qi as int; break }
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

    req.proto = http_ctor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

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
            var name = http_ctor((buf + line_start) as *char, ((colon as usize) - line_start) as size_t);
            var vstart = (colon as usize) + 1;
            if(vstart < line_end && buf[vstart] == ' ' as u8) { vstart = vstart + 1 }
            var value = http_ctor((buf + vstart) as *char, (line_end - vstart) as size_t);
            req.headers.insert(name, value);
            header_count = header_count + 1u;
        }
        line_start = line_end + 2;
        if(header_count > 512u) { return Option.None<Request>() }
    }

    var cl_opt = req.headers.get("Content-Length");
    if(cl_opt is Option.Some) {
        var Some(v) = cl_opt else unsafe { unreachable };
        var val = 0u; var ii = 0u;
        while(ii < v.size()) { var c = v.at(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
        req.body_len = val;
    }

    req.remote = http_lit("127.0.0.1" as *char);
    return Option.Some<Request>(req);
    }
}

// Incremental response head reader over an owning Transport.
func read_response_head(transport : &mut Transport, buf : *mut net::Buffer,
                               timeout_secs : long, max_header_bytes : usize) : Result<Response, HttpError> { unsafe {
    var _ = transport.set_read_timeout((timeout_secs * 1000) as u64)
    var interim_1xx_count = 0u;
    loop {
        var end = find_header_end(buf)
        if(end != std::NPOS) {
            var ptr = buf.as_ptr()
            var res_opt = parse_response_from_bytes(ptr, end)
            if(res_opt is Option.Some) {
                var Some(res) = res_opt else unsafe { unreachable };
                buf.consume(end);
                if(res.status >= 100u && res.status <= 199u && res.status != 101u) {
                    interim_1xx_count = interim_1xx_count + 1u;
                    if(interim_1xx_count > 16u) {
                        return Result.Err(HttpError.Protocol(http_make_string("too many interim responses")))
                    }
                    continue;
                }
                return Result.Ok<Response, HttpError>(std::replace(&mut res, Response()))
            }
            return Result.Err(HttpError.Protocol(http_make_string("malformed response")))
        }
        if(buf.len() >= max_header_bytes) {
            return Result.Err(HttpError.HeaderTooLarge())
        }
        var tmp : [DEFAULT_READ_BUF]u8;
        var r = transport.read_bytes(unsafe(std::mut_span<u8>(&raw mut tmp[0], DEFAULT_READ_BUF as size_t)))
        if(r is Result.Err) {
            return Result.Err(HttpError.Io(http_io_error()))
        }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0u) {
            return Result.Err(HttpError.Protocol(http_make_string("connection closed before response")))
        }
        buf.append_bytes(&raw mut tmp[0], n);
    }
}
}

func parse_response_from_bytes(buf:*u8, n: usize) : Option<Response> {
    unsafe {
    var crlf_pos = -1;
    var j = 0u;
    while(j + 1 < n) {
        if(buf[j] == '\r' as u8 && buf[j+1] == '\n' as u8) { crlf_pos = j as int; break }
        j++;
    }
    if(crlf_pos == -1) { return Option.None<Response>() }

    var sp1 = -1; var sp2 = -1; var k = 0u;
    while(k < (crlf_pos as usize)) {
        if(buf[k] == ' ' as u8 && sp1 == -1) { sp1 = k as int }
        else if(buf[k] == ' ' as u8 && sp2 == -1) { sp2 = k as int; break }
        k++
    }
    if(sp1 == -1 || sp2 == -1) { return Option.None<Response>() }

    var res = Response();
    res.proto = http_ctor((buf + 0) as *char, (sp1 as usize) as size_t);
    var status_str = http_ctor((buf + (sp1 as usize + 1)) as *char, ((sp2 as usize) - (sp1 as usize) - 1) as size_t);
    var status_val = 0u;
    for(var i=0u; i<status_str.size(); i++) {
        var c = status_str.at(i);
        if(c >= '0' && c <= '9') { status_val = status_val * 10u + (c as uint - '0' as uint) }
    }
    res.status = status_val;
    res.status_text = http_ctor((buf + (sp2 as usize + 1)) as *char, ((crlf_pos as usize) - (sp2 as usize) - 1) as size_t);

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
            var name = http_ctor((buf + line_start) as *char, ((colon as usize) - line_start) as size_t);
            var vstart = (colon as usize) + 1;
            while(vstart < line_end && buf[vstart] == ' ' as u8) { vstart = vstart + 1 }
            var value = http_ctor((buf + vstart) as *char, (line_end - vstart) as size_t);
            res.headers.insert(name, value);
        }
        line_start = line_end + 2;
    }

    var cl_opt = res.headers.get("Content-Length");
    if(cl_opt is Option.Some) {
        var Some(v) = cl_opt else unsafe { unreachable };
        var val = 0u; var ii = 0u;
        while(ii < v.size()) { var c = v.at(ii); if(c >= '0' && c <= '9') { val = val * 10u + (c as usize - '0' as usize) } ii = ii + 1u }
        res.body_len = val;
    }
    return Option.Some<Response>(res);
    }
}

} // namespace http
