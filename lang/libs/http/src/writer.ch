// ===== HTTP response encoding =====
//
// The framework owns the connection: a handler returns a `Response` and this is
// the single place that turns one into wire bytes. Because the body is known
// before the head is written, `Content-Length` is always exact, a second write
// cannot desynchronise the framing, and keep-alive is decidable per message.
public namespace http {

using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;

public const DEFAULT_READ_BUF = 8192u;

/// Whether this status code may carry a body.
func status_allows_body(code : u16) : bool {
    if(code == 204u || code == 304u) { return false }
    if(code >= 100u && code < 200u) { return false }
    return true
}

// ---------------------------------------------------------------------------
// Wire safety of a response head
// ---------------------------------------------------------------------------
//
// The head is assembled from values the handler chose, and a handler usually gets
// some of them from the request: `web::redirect(next)` where `next` is a query
// parameter, a validation message that quotes what the user typed, a header
// synthesized from a form field. A CR or LF inside any of those ends our head
// early: the rest is read by the client as headers or as a second response
// (response splitting / cache poisoning), and with a body it is XSS.
//
// The check lives here, at the single place that turns a `Response` into bytes,
// because every other producer (a handler, `web`, `webserve`, a plugin) can be
// skipped but this one cannot.

/// A field value may hold visible characters, spaces, tabs and obs-text; control
/// characters (CR and LF above all) are refused.
func wire_safe_field_value(value : string_view) : bool {
    var i : usize = 0u
    while(i < value.size()) {
        var c = value.at(i)
        var byte = c as u8
        if(byte < 32u && byte != 9u) { return false }
        if(byte == 127u) { return false }
        i = i + 1u
    }
    return true
}

/// A field name must be an RFC 9110 token: `!#$%&'*+-.^_`|~` plus alphanumerics.
func wire_safe_field_name(value : string_view) : bool {
    if(value.size() == 0u) { return false }
    var i : usize = 0u
    while(i < value.size()) {
        var c = value.at(i)
        var ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
        if(!ok) {
            ok = c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\''
            ok = ok || c == '*' || c == '+' || c == '-' || c == '.' || c == '^'
            ok = ok || c == '_' || c == '`' || c == '|' || c == '~'
        }
        if(!ok) { return false }
        i = i + 1u
    }
    return true
}

/// Whether every byte this response's head would carry is wire-safe.
func response_head_is_wire_safe(res : &Response) : bool {
    if(!res.proto.empty() && !wire_safe_field_value(res.proto.to_view())) { return false }
    if(!res.status_text.empty() && !wire_safe_field_value(res.status_text.to_view())) { return false }
    var iter = res.headers.iter()
    while(iter.valid()) {
        if(!wire_safe_field_name(iter.name())) { return false }
        if(!wire_safe_field_value(iter.value())) { return false }
        iter.next()
    }
    return true
}

/// Refuse a head the framework cannot encode: `500`, no body, connection closed.
/// Nothing of the offending response has been written, so the client sees one
/// complete answer instead of a truncated or split one.
func write_unsafe_head_refusal(out : &mut Stream, date : std::string_view) : Result<Unit, HttpError> {
    var head = string()
    head.append_view("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\nConnection: close\r\n")
    if(!date.empty()) {
        head.append_view("Date: ")
        head.append_view(&date)
        head.append_view("\r\n")
    }
    head.append_view("\r\n")
    var _ = out.write_all_bytes(unsafe(std::span<u8>(head.data() as *u8, head.size())))
    return Result.Err(HttpError.Protocol("response head is not wire-safe"))
}

/// Write a response payload onto `out`.
public func (b: &mut ResponseBody) write_onto(out : &mut Stream) : Result<Unit, HttpError> {
    switch(b) {
        Bytes(d) => {
            var w = out.write_all_bytes(d.as_span())
            if(w is Result.Err) { return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe()))) }
            return Result.Ok(Unit { })
        }
        Text(t) => {
            var w = out.write_all_bytes(unsafe(std::span<u8>(t.data() as *u8, t.size())))
            if(w is Result.Err) { return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe()))) }
            return Result.Ok(Unit { })
        }
        View(v) => {
            var w = out.write_all_bytes(unsafe(std::span<u8>(v.data() as *u8, v.size())))
            if(w is Result.Err) { return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe()))) }
            return Result.Ok(Unit { })
        }
        File(f, len) => {
            var chunk : [65536]u8
            var remaining = len
            while(remaining > 0u) {
                var want : usize = 65536u
                if(remaining < 65536u) { want = remaining as usize }
                var view = unsafe(std::mut_span<u8>(&raw mut chunk[0], want))
                var r = unsafe(f.read(view))
                if(r is Result.Err) {
                    return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::Other())))
                }
                var Ok(n) = r else unsafe { unreachable }
                if(n == 0u) {
                    return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())))
                }
                var w = out.write_all_bytes(unsafe(std::span<u8>(&raw mut chunk[0], n)))
                if(w is Result.Err) {
                    return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe())))
                }
                remaining = remaining - (n as u64)
            }
            return Result.Ok(Unit { })
        }
        Empty() => { return Result.Ok(Unit { }) }
        default => { return Result.Ok(Unit { }) }
    }
}

/// Encode a response onto `out`.
///
/// * `keep_alive` — the connection may be reused; the `Connection` header is
///   emitted accordingly (a handler can force `close` by setting it).
/// * `head_only` — a `HEAD` response: the head carries the real
///   `Content-Length`, the body is not written.
/// * `date` — the `Date` header value (RFC 9110 §6.6.1). The server passes its
///   per-second cached IMF-fixdate; empty means "do not send one" (a non-server
///   caller writing a message for a test).
public func (res: &mut Response) write_to(out : &mut Stream, keep_alive : bool, head_only : bool,
                                         date : std::string_view) : Result<Unit, HttpError> {
    if(!response_head_is_wire_safe(res)) {
        return write_unsafe_head_refusal(out, date)
    }
    var head = string()
    head.reserve(320)

    if(res.proto.empty()) {
        head.append_view("HTTP/1.1 ")
    } else {
        head.append_string(&res.proto)
        head.append(' ')
    }
    head.append_uinteger(res.status as ubigint)
    head.append(' ')
    if(res.status_text.empty()) {
        head.append_view(status_text(res.status))
    } else {
        head.append_string(&res.status_text)
    }
    head.append_view("\r\n")

    var body_len = res.body.size()
    var has_body = status_allows_body(res.status)

    // Our length wins over a handler-set one: a mismatch would desynchronise the
    // connection. Bodyless statuses must not advertise a length at all.
    res.headers.remove("Content-Length")
    if(has_body) {
        head.append_view("Content-Length: ")
        head.append_uinteger(body_len as ubigint)
        head.append_view("\r\n")
    }

    // A handler can request `close` explicitly; otherwise follow the caller.
    var conn = res.headers.get("Connection")
    if(conn is Option.Some) {
        var Some(cv) = conn else unsafe { unreachable }
        if(text_equals(cv, string_view("close"))) { keep_alive = false }
    }
    res.headers.remove("Connection")
    if(keep_alive) {
        head.append_view("Connection: keep-alive\r\n")
    } else {
        head.append_view("Connection: close\r\n")
    }

    if(!date.empty() && !res.headers.contains("Date")) {
        head.append_view("Date: ")
        head.append_view(&date)
        head.append_view("\r\n")
    }

    var iter = res.headers.iter()
    while(iter.valid()) {
        head.append_view(iter.name())
        head.append_view(": ")
        head.append_view(iter.value())
        head.append_view("\r\n")
        iter.next()
    }
    head.append_view("\r\n")

    var wrote = out.write_all_bytes(unsafe(std::span<u8>(head.data() as *u8, head.size())))
    if(wrote is Result.Err) {
        return Result.Err(HttpError.Io(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe())))
    }

    if(head_only || !has_body || body_len == 0u) {
        return Result.Ok(Unit { })
    }
    return res.body.write_onto(out)
}

} // namespace http
