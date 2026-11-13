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
public func (res: &mut Response) write_to(out : &mut Stream, keep_alive : bool, head_only : bool) : Result<Unit, HttpError> {
    var head = string()
    head.reserve(256)

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
