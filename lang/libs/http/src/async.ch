// http::raw — legacy raw-pointer HTTP surface.
//
// This is the ONLY declaration of `http::raw` in the module. It contains:
//   * the C-string header getter (safe callers use `HeaderMap::get`/`get_owned`);
//   * the async client entry points, which return a caller-owned
//     `*mut HttpResult` and must be `delete`d by the caller. Prefer the
//     synchronous `http::Client` surface where possible.
public namespace http {

public namespace raw {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

/// C-string header lookup. Prefer `HeaderMap::get`/`get_owned`.
public unsafe func header_get_cstr(map : &HeaderMap, name : *char) : Option<string> {
    var want = unsafe(string_view.make_no_len(name))
    var i = 0u
    while(i < map.headers.size()) {
        var p = unsafe(map.headers.get_ptr(i))
        if(header_name_equals(unsafe(p.first.to_view()), want)) {
            return Option.Some<string>(unsafe(p.second.copy()))
        }
        i += 1u
    }
    return Option.None<string>()
}

/// Compatibility raw body constructor for tests/migration. The socket handle
/// is ignored; transport reads fail closed until a real Transport is attached.
public unsafe func make_body(sock : net::raw::Socket,
                             buf_ptr : *mut net::Buffer,
                             content_len : isize,
                             chunked : bool,
                             timeout_secs : long,
                             max_body : usize,
                             owns : bool = false) : Body {
    var t = unsafe(malloc(sizeof(Transport))) as *mut Transport
    if(t != null) { unsafe { *t = Transport() } }
    return Body.make_owned(t, buf_ptr, content_len, chunked, timeout_secs, max_body)
}

/// Raw pointer read. Safe callers use `Body::read(&mut [u8])`.
public unsafe func body_read(b : *mut Body, dst : *mut u8, cap : usize) : int {
    return unsafe(b.read_raw(dst, cap))
}

// ---- async client ----------------------------------------------------------
//
// The result is a flat heap box: the caller owns it and must `delete` it.
// These return pointers because the original async ABI predates the safe
// `HttpResult` value; new code should call the synchronous client and move the
// result, or use `http::HttpResult` directly.

func http_box(r : Result<Response, HttpError>) : *mut HttpResult {
    var box = unsafe(malloc(sizeof(HttpResult))) as *mut HttpResult
    if(r is Result.Ok) {
        var Ok(resp) = r else unsafe { unreachable }
        var body = string()
        var bopt = resp.body.read_to_string_opt()
        if(bopt is Option.Some) {
            var Some(s) = bopt else unsafe { unreachable }
            body = std::replace(&mut s, string())
        }
        unsafe { new(box) HttpResult {
            ok : true,
            status : resp.status,
            status_text : std::replace(&mut resp.status_text, string()),
            headers : std::replace(&mut resp.headers, HeaderMap.make()),
            body : body,
            error : string()
        } }
    } else {
        var Err(e) = r else unsafe { unreachable }
        var message = string()
        var view = e.message()
        message.append_view(&view)
        unsafe { new(box) HttpResult {
            ok : false,
            status : 0u,
            status_text : string(),
            headers : HeaderMap.make(),
            body : string(),
            error : std::replace(&mut message, string())
        } } }
    return box
}

public async func request_async(client : *mut Client, req_builder : *mut RequestBuilder) : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, req_builder|() => {
        return http_box(client.request(unsafe(&mut *req_builder)))
    }))
}

public async func get_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client.get_view(unsafe(&mut *url_str)))
    }))
}

public async func post_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        var ct = http_lit(content_type)
        return http_box(client.post_view(unsafe(&mut *url_str), unsafe(&mut *body), ct.to_view()))
    }))
}

public async func put_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        var ct = http_lit(content_type)
        return http_box(client.put_view(unsafe(&mut *url_str), unsafe(&mut *body), ct.to_view()))
    }))
}

public async func patch_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        var ct = http_lit(content_type)
        return http_box(client.patch_view(unsafe(&mut *url_str), unsafe(&mut *body), ct.to_view()))
    }))
}

public async func delete_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client.delete_view(unsafe(&mut *url_str)))
    }))
}

public async func head_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await unsafe(async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client.head_view(unsafe(&mut *url_str)))
    }))
}

// Await the next chunk of the response body. Resolves with the number of bytes
// read, 0 at end of body, <0 on error. Raw destination pointer.
public func read_chunk_async(b : *mut Body, dst : *mut u8, cap : usize) : core::async::FutureHandle<'static, int> {
    return body_read_async(b, dst, cap)
}

public async func body_read_async(b : *mut Body, dst : *mut u8, cap : usize) : int {
    return await unsafe(async::spawn_blocking<int>(|b, dst, cap|() => {
        return unsafe(b.read_raw(dst, cap))
    }))
}

public async func read_to_string_async(b : *mut Body) : std::Option<string> {
    return await unsafe(async::spawn_blocking<std::Option<string>>(|b|() => {
        return b.read_to_string_opt()
    }))
}

} // namespace raw
} // namespace http

public namespace server {

using std::Result;

// Coroutine accept loop (R1: `serve_async` is taken by the thread variant).
// Drive it with `async::block_on<int>(serve_coro(&raw mut srv, port))`, or
// submit it with `async::raw::spawn<int>(...)` and call `srv.shutdown()` to stop.
public async func serve_coro(srv : *mut Server, port : uint = 8080u) : int { unsafe {
    srv.start(port)
    if(!srv.run) { return 1 }
    var listener_fd = net::raw::listener_fd(&srv.listener)
    net::raw::set_recv_timeout(listener_fd, 0, 200000)
    while(srv.run) {
        var accepted = await unsafe(async::spawn_blocking<int>(|listener_fd|() => net::raw::accept_socket(listener_fd) as int))
        if(accepted <= 0) {
            continue
        }
        var fd = accepted as net::raw::Socket
        var s = srv
        var h = unsafe(async::spawn_blocking<core::async::Unit>(|s, fd|() => {
            var conn = net::raw::stream_from_fd(fd)
            conn.set_keep_alive(true)
            s.handle_conn(conn)
            return core::async::Unit { }
        }))
    }
    net::raw::close_socket(listener_fd)
    srv.run = false
    return 0
}}

} // namespace server
