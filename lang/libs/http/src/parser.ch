// ===== HTTP message parsing =====
public namespace http {

using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;

// Default maximum response body size read into memory (100MB).
public comptime const DEFAULT_MAX_BODY_LEN : usize = 100u * 1024u * 1024u

// URL decode (percent-decoding)
public func url_decode(in_s : &string_view) : string {
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

// ---------------------------------------------------------------------------
// Buffered reader helpers
// ---------------------------------------------------------------------------

// Scan for the end of a head block (CRLFCRLF). Returns the offset just past it,
// or NPOS when the block is not complete yet.
func find_header_end(buf : &net::Buffer) : size_t {
    var i = 0u
    while(i + 3u < buf.len()) {
        if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i+1u) == '\n' as u8 &&
           buf.get_byte(i+2u) == '\r' as u8 && buf.get_byte(i+3u) == '\n' as u8) {
            return i + 4u
        }
        i += 1u
    }
    return std::NPOS
}

// Pull one more chunk from the stream into `buf`. Returns bytes read (0 = EOF).
//
// `WouldBlock`, `TimedOut` and `Interrupted` mean "no data yet" (a non-blocking
// descriptor, an `SO_RCVTIMEO` expiry, a signal); those are retried until the
// caller's deadline. Every other error is a real failure of the connection and
// is propagated immediately, so a reset peer is not mistaken for a slow one.
func fill(stream : &mut Stream, buf : &mut net::Buffer, budget_ms : long) : Result<size_t, HttpError> {
    var tmp : [DEFAULT_READ_BUF]u8
    var view = unsafe(std::mut_span<u8>(&raw mut tmp[0], DEFAULT_READ_BUF as size_t))
    var started = std::chrono::Instant::now()
    loop {
        var r = stream.read_bytes(view)
        if(r is Result.Ok) {
            var Ok(n) = r else unsafe { unreachable }
            if(n > 0u) { buf.append(unsafe(std::span<u8>(&raw mut tmp[0], n))) }
            return Result.Ok(n)
        }
        var Err(e) = r else unsafe { unreachable }
        if(!is_transient(e)) {
            return Result.Err(HttpError.Io(e))
        }
        if((started.elapsed().as_millis() as long) >= budget_ms) {
            return Result.Err(HttpError.Timeout())
        }
        std.concurrent.sleep_ms(1u)
    }
    return Result.Err(HttpError.Timeout())
}

/// Whether a socket error means "try again" rather than "this connection failed".
func is_transient(e : std::io::IoError) : bool {
    var kind = e.kind()
    if(kind is std::io::IoErrorKind.WouldBlock) { return true }
    if(kind is std::io::IoErrorKind.TimedOut) { return true }
    if(kind is std::io::IoErrorKind.Interrupted) { return true }
    return false
}

// Read one CRLF-terminated line (without the CRLF). `None` on EOF before the
// line is complete. Bounded by `limit`.
func take_line(stream : &mut Stream, buf : &mut net::Buffer, limit : usize, budget_ms : long) : Result<Option<string>, HttpError> {
    loop {
        var i = 0u
        var n = buf.len()
        while(i + 1u < n) {
            if(buf.get_byte(i) == '\r' as u8 && buf.get_byte(i + 1u) == '\n' as u8) {
                var line = http_ctor(buf.as_ptr() as *char, i as size_t)
                buf.consume(i + 2u)
                return Result.Ok(Option.Some<string>(line))
            }
            i += 1u
        }
        if(n > limit) { return Result.Err(HttpError.HeaderTooLarge()) }
        var filled = fill(stream, buf, budget_ms)
        if(filled is Result.Err) {
            var Err(fe) = filled else unsafe { unreachable }
            return Result.Err(fe)
        }
        var Ok(got) = filled else unsafe { unreachable }
        if(got == 0u) { return Result.Ok(Option.None<string>()) }
    }
}

// Decimal parse of a header value, saturating at `usize` max. Returns NPOS-like
// `std::NPOS` on malformed input.
func parse_decimal(v : string_view) : u64 {
    var val : u64 = 0
    var seen = false
    var i = 0u
    while(i < v.size()) {
        var c = v.at(i)
        if(c == ' ' || c == '\t') { i += 1u; continue }
        if(c < '0' || c > '9') { return 0xFFFFFFFFFFFFFFFFu64 }
        val = val * 10u64 + ((c as u64) - ('0' as u64))
        seen = true
        i += 1u
    }
    if(!seen) { return 0xFFFFFFFFFFFFFFFFu64 }
    return val
}

// Copy `want` bytes from `buf` (consuming) plus whatever the stream supplies.
func take_bytes(stream : &mut Stream, buf : &mut net::Buffer, body : *mut RequestBody, want : usize, budget_ms : long) : Result<Unit, HttpError> {
    var remaining = want
    var started = std::chrono::Instant::now()
    while(remaining > 0u) {
        var avail = buf.len()
        if(avail > 0u) {
            var take = avail
            if(remaining < take) { take = remaining }
            unsafe { body.append_raw(buf.as_ptr(), take) }
            buf.consume(take)
            remaining -= take
            continue
        }
        var tmp : [DEFAULT_READ_BUF]u8
        var view = unsafe(std::mut_span<u8>(&raw mut tmp[0], DEFAULT_READ_BUF as size_t))
        var r = stream.read_bytes(view)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            if(!is_transient(e)) { return Result.Err(HttpError.Io(e)) }
            if((started.elapsed().as_millis() as long) >= budget_ms) {
                return Result.Err(HttpError.Timeout())
            }
            std.concurrent.sleep_ms(1u)
            continue
        }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0u) { return Result.Err(HttpError.Protocol(http_make_string("truncated body"))) }
        if((n as usize) > remaining) {
            // Only part of this read belongs to the body; the rest is the next
            // pipelined message.
            unsafe { body.append_raw(&raw mut tmp[0], remaining) }
            buf.append(unsafe(std::span<u8>(&raw mut tmp[0], n)))
            buf.consume(remaining)
            remaining = 0u
            continue
        }
        unsafe { body.append_raw(&raw mut tmp[0], n) }
        remaining -= n
    }
    return Result.Ok(Unit { })
}

// Decode a chunked body into `body`, bounded by `max_body`.
func take_chunked(stream : &mut Stream, buf : &mut net::Buffer, body : *mut RequestBody, max_body : usize, budget_ms : long) : Result<Unit, HttpError> {
    loop {
        var line = take_line(stream, buf, 8192u, budget_ms)
        if(line is Result.Err) { return Result.Err(HttpError.Protocol(http_make_string("bad chunk line"))) }
        var Ok(opt) = line else unsafe { unreachable }
        if(opt is Option.None) { return Result.Err(HttpError.Protocol(http_make_string("truncated chunked body"))) }
        var Some(text) = opt else unsafe { unreachable }
        // chunk-size [;ext]
        var size_end = 0u
        while(size_end < text.size() && text.at(size_end) != ';') { size_end += 1u }
        var text_view = text.to_view()
        var size_str = text_view.subview(0u, size_end)
        var size = parse_decimal(size_str)
        if(size == 0xFFFFFFFFFFFFFFFFu64) { return Result.Err(HttpError.Protocol(http_make_string("bad chunk size"))) }
        if(size == 0u) {
            // trailer section: lines until an empty one
            loop {
                var trailer = take_line(stream, buf, 8192u, budget_ms)
                if(trailer is Result.Err) { return Result.Err(HttpError.Protocol(http_make_string("bad chunk trailer"))) }
                var Ok(topt) = trailer else unsafe { unreachable }
                if(topt is Option.None) { return Result.Ok(Unit { }) }
                var Some(tline) = topt else unsafe { unreachable }
                if(tline.empty()) { return Result.Ok(Unit { }) }
            }
        }
        if(size > max_body as u64) { return Result.Err(HttpError.BodyTooLarge()) }
        unsafe {
            if(body.size() + (size as usize) > max_body) { return Result.Err(HttpError.BodyTooLarge()) }
        }
        var br = take_bytes(stream, buf, body, size as usize, budget_ms)
        if(br is Result.Err) { return br }
        // trailing CRLF after the chunk data
        var crlf = take_line(stream, buf, 8u, budget_ms)
        if(crlf is Result.Err) { return Result.Err(HttpError.Protocol(http_make_string("bad chunk terminator"))) }
    }
}

// ---------------------------------------------------------------------------
// Requests
// ---------------------------------------------------------------------------

// Read and frame one request from `stream`. Any bytes belonging to the body or
// the next pipelined request stay in `buf`.
func read_request_head(stream : &mut Stream, buf : &mut net::Buffer,
                       header_timeout_secs : long, max_header_bytes : usize,
                       max_headers : uint) : Result<Request, HttpError> {
    var _ = stream.set_read_timeout((header_timeout_secs * 1000) as u64)
    loop {
        var end = find_header_end(buf)
        if(end != std::NPOS) {
            var parsed = parse_request_from_bytes(buf.as_ptr(), end, max_headers)
            if(parsed is Option.Some) {
                var Some(req) = parsed else unsafe { unreachable }
                buf.consume(end)
                return Result.Ok(std::replace(&mut req, Request()))
            }
            return Result.Err(HttpError.Protocol(http_lit("malformed request")))
        }
        if(buf.len() > max_header_bytes) {
            return Result.Err(HttpError.HeaderTooLarge())
        }
        var filled = fill(stream, buf, header_timeout_secs * 1000)
        if(filled is Result.Err) {
            var Err(fe) = filled else unsafe { unreachable }
            return Result.Err(fe)
        }
        var Ok(n) = filled else unsafe { unreachable }
        if(n == 0u) { return Result.Err(HttpError.Protocol(http_lit("connection closed"))) }
    }
}

/// Read the request body according to its framing (chunked, Content-Length, or
/// none) into `req.body`, bounded by `max_body`.
public func read_request_body(stream : &mut Stream, buf : &mut net::Buffer, req : &mut Request,
                              max_body : usize, timeout_ms : u64) : Result<Unit, HttpError> {
    var _ = stream.set_read_timeout(timeout_ms)
    var chunked = false
    var te = req.headers.get("Transfer-Encoding")
    if(te is Option.Some) {
        var Some(tv) = te else unsafe { unreachable }
        if(text_equals(tv, string_view("chunked"))) { chunked = true }
    }
    if(chunked) {
        return take_chunked(stream, buf, &raw mut req.body, max_body, timeout_ms as long)
    }
    var cl = req.headers.get("Content-Length")
    if(cl is Option.None) { return Result.Ok(Unit { }) }
    var Some(cv) = cl else unsafe { unreachable }
    var declared = parse_decimal(cv)
    if(declared == 0xFFFFFFFFFFFFFFFFu64) {
        return Result.Err(HttpError.Protocol(http_make_string("bad content-length")))
    }
    if(declared > max_body as u64) { return Result.Err(HttpError.BodyTooLarge()) }
    req.body.reserve_hint(declared as usize)
    return take_bytes(stream, buf, &raw mut req.body, declared as usize, timeout_ms as long)
}

// ---------------------------------------------------------------------------
// Responses
// ---------------------------------------------------------------------------

// Read and frame one response head from `stream`.
func read_response_head(stream : &mut Stream, buf : &mut net::Buffer,
                        timeout_secs : long, max_header_bytes : usize) : Result<Response, HttpError> {
    var _ = stream.set_read_timeout((timeout_secs * 1000) as u64)
    var interim_1xx_count = 0u
    loop {
        var end = find_header_end(buf)
        if(end != std::NPOS) {
            var parsed = parse_response_from_bytes(buf.as_ptr(), end)
            if(parsed is Option.Some) {
                var Some(res) = parsed else unsafe { unreachable }
                buf.consume(end)
                if(res.status >= 100u && res.status <= 199u && res.status != 101u) {
                    interim_1xx_count = interim_1xx_count + 1u
                    if(interim_1xx_count > 16u) {
                        return Result.Err(HttpError.Protocol(http_make_string("too many interim responses")))
                    }
                    continue
                }
                return Result.Ok(std::replace(&mut res, Response()))
            }
            return Result.Err(HttpError.Protocol(http_make_string("malformed response")))
        }
        if(buf.len() >= max_header_bytes) {
            return Result.Err(HttpError.HeaderTooLarge())
        }
        var filled = fill(stream, buf, timeout_secs * 1000)
        if(filled is Result.Err) {
            var Err(fe) = filled else unsafe { unreachable }
            return Result.Err(fe)
        }
        var Ok(n) = filled else unsafe { unreachable }
        if(n == 0u) {
            return Result.Err(HttpError.Protocol(http_make_string("connection closed before response")))
        }
    }
}

/// Read a whole response body into `res.body` (bounded by `max_body`). A
/// response without `Content-Length` or `Transfer-Encoding` is terminated by the
/// peer closing the connection.
public func read_response_body(stream : &mut Stream, buf : &mut net::Buffer, res : &mut Response,
                               max_body : usize, timeout_ms : u64) : Result<Unit, HttpError> {
    var _ = stream.set_read_timeout(timeout_ms)
    if(!status_allows_body(res.status)) {
        res.body = ResponseBody.Empty()
        return Result.Ok(Unit { })
    }
    var chunked = false
    var te = res.headers.get("Transfer-Encoding")
    if(te is Option.Some) {
        var Some(tv) = te else unsafe { unreachable }
        if(text_equals(tv, string_view("chunked"))) { chunked = true }
    }
    var body = RequestBody()
    var out : Result<Unit, HttpError> = Result.Ok(Unit { })
    if(chunked) {
        out = take_chunked(stream, buf, &raw mut body, max_body, timeout_ms as long)
    } else {
        var cl = res.headers.get("Content-Length")
        if(cl is Option.Some) {
            var Some(cv) = cl else unsafe { unreachable }
            var declared = parse_decimal(cv)
            if(declared == 0xFFFFFFFFFFFFFFFFu64) {
                return Result.Err(HttpError.Protocol(http_make_string("bad content-length")))
            }
            if(declared > max_body as u64) { return Result.Err(HttpError.BodyTooLarge()) }
            body.reserve_hint(declared as usize)
            out = take_bytes(stream, buf, &raw mut body, declared as usize, timeout_ms as long)
        } else {
            // terminated by connection close
            var avail = buf.len()
            if(avail > 0u) {
                if(avail > max_body) { return Result.Err(HttpError.BodyTooLarge()) }
                body.append_raw(buf.as_ptr(), avail)
                buf.consume(avail)
            }
            loop {
                var tmp : [DEFAULT_READ_BUF]u8
                var view = unsafe(std::mut_span<u8>(&raw mut tmp[0], DEFAULT_READ_BUF as size_t))
                var r = stream.read_bytes(view)
                if(r is Result.Err) { break }
                var Ok(n) = r else unsafe { unreachable }
                if(n == 0u) { break }
                if(body.size() + n > max_body) { return Result.Err(HttpError.BodyTooLarge()) }
                body.append_raw(&raw mut tmp[0], n)
            }
            out = Result.Ok(Unit { })
        }
    }
    if(out is Result.Err) { return out }
    res.body = ResponseBody.Bytes(body.take_all())
    return Result.Ok(Unit { })
}

// ---------------------------------------------------------------------------
// Bare parsers (no I/O)
// ---------------------------------------------------------------------------

func parse_request_from_bytes(buf:*u8, n: usize, max_headers : uint) : Option<Request> { unsafe {
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
            // Multi-value headers (Cookie, Accept, ...) keep every occurrence.
            req.headers.append(name, value);
            header_count = header_count + 1u;
        }
        line_start = line_end + 2;
        if(max_headers > 0u && header_count > max_headers) { return Option.None<Request>() }
    }

    req.remote = http_lit("127.0.0.1" as *char);
    return Option.Some<Request>(req);
    }
}

func parse_response_from_bytes(buf:*u8, n: usize) : Option<Response> { unsafe {
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
    var status_val : u16 = 0
    for(var i=0u; i<status_str.size(); i++) {
        var c = status_str.at(i);
        if(c >= '0' && c <= '9') { status_val = (status_val * 10u16) + ((c as u16) - ('0' as u16)) }
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
            res.headers.append(name, value);
        }
        line_start = line_end + 2;
    }
    return Option.Some<Response>(res);
    }
}

} // namespace http
