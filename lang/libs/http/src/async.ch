// Async HTTP (design §7 Tier 3).
//
// Additive: the synchronous client/server APIs are unchanged. The payloads are
// pointer/int-only, the simplest ABI (B23, a plain-struct future payload on
// LLVM, is now fixed; B24, struct-typed async parameters on 2c, still applies).
//
// Client: each request offloads the whole blocking exchange (dial → TLS →
// write → incremental read) to the runtime thread pool, so requests from many
// tasks overlap (bounded by pool size).
//
// Server: a coroutine accept loop over the Tier 1 `net::accept_async`, with a
// short per-accept timeout so it can observe `Server::run` for shutdown.
// Accepted connections are handed to the pool because the request handler is
// synchronous, so the loop itself never blocks on a connection.
//
// Body: `read`/`read_to_string` offload to the pool.
public namespace http {

// ---- client ----------------------------------------------------------------
//
// The response is copied into a flat, heap-allocated `HttpResult` and the future
// carries a pointer to it. Two reasons:
//   * a large `Result<Response, std::string>` returned through a `FutureHandle`
//     is corrupted by the LLVM backend (B26);
//   * it gives callers a non-moving accessor API (`ok`/`status`/`body_view`),
//     avoiding B18 moves out of a variant payload.
// The caller owns the box and must `delete` it.

public struct HttpResult {
    var ok : bool
    var status : uint
    var status_text : std::string
    var headers : HeaderMap
    var body : std::string
    var error : std::string

    public func is_ok(&self) : bool {
        return self.ok
    }

    public func status_code(&self) : uint {
        return self.status
    }

    public func body_view(&self) : std::string_view {
        return self.body.to_view()
    }

    public func error_view(&self) : std::string_view {
        return self.error.to_view()
    }
}

func http_box(client : *mut Client, r : std::Result<Response, std::string>) : *mut HttpResult {
    var box = malloc(sizeof(HttpResult)) as *mut HttpResult
    if(r is std::Result.Ok) {
        var Ok(resp) = r else unreachable
        var body = std::string()
        var bopt = resp.body.read_to_string()
        if(bopt is std::Option.Some) {
            var Some(s) = bopt else unreachable
            body = std::replace(&mut s, std::string())
        }
        new(box) HttpResult {
            ok : true,
            status : resp.status,
            status_text : std::replace(&mut resp.status_text, std::string()),
            headers : std::replace(&mut resp.headers, HeaderMap.make()),
            body : body,
            error : std::string()
        }
    } else {
        var Err(e) = r else unreachable
        new(box) HttpResult {
            ok : false,
            status : 0u,
            status_text : std::string(),
            headers : HeaderMap.make(),
            body : std::string(),
            error : std::replace(&mut e, std::string())
        }
    }
    return box
}

public async func request_async(client : *mut Client, req_builder : *mut RequestBuilder) : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, req_builder|() => {
        return http_box(client, client.request(&mut *req_builder))
    })
}

public async func get_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client, client.get(&mut *url_str))
    })
}

public async func post_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        return http_box(client, client.post(&mut *url_str, &mut *body, content_type))
    })
}

public async func put_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        return http_box(client, client.put(&mut *url_str, &mut *body, content_type))
    })
}

public async func patch_async(client : *mut Client, url_str : *std::string_view, body : *std::string_view, content_type : *char = "text/plain") : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str, body, content_type|() => {
        return http_box(client, client.patch(&mut *url_str, &mut *body, content_type))
    })
}

public async func delete_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client, client.delete(&mut *url_str))
    })
}

public async func head_async(client : *mut Client, url_str : *std::string_view) : *mut HttpResult {
    return await async::spawn_blocking<*mut HttpResult>(|client, url_str|() => {
        return http_box(client, client.head(&mut *url_str))
    })
}

// ---- body ------------------------------------------------------------------

// Await the next chunk of the response body (F12 streaming). Resolves with the
// number of bytes read, 0 at end of body, <0 on error.
public func read_chunk_async(b : *mut Body, dst : *mut u8, cap : usize) : core::async::FutureHandle<int> {
    return body_read_async(b, dst, cap)
}

public async func body_read_async(b : *mut Body, dst : *mut u8, cap : usize) : int {
    return await async::spawn_blocking<int>(|b, dst, cap|() => {
        return b.read(dst, cap)
    })
}

public async func read_to_string_async(b : *mut Body) : std::Option<std::string> {
    return await async::spawn_blocking<std::Option<std::string>>(|b|() => {
        return b.read_to_string()
    })
}

}

// ---- server ----------------------------------------------------------------
// `server` is a top-level namespace (see `server.ch`), not nested under `http`.

public namespace server {

// Coroutine accept loop (R1: `serve_async` is taken by the thread variant).
// Drive it with `async::block_on<int>(serve_coro(&raw mut srv, port))`, or
// submit it with `async::spawn<int>(...)` and call `srv.shutdown()` to stop.
public async func serve_coro(srv : *mut Server, port : uint = 8080u) : int {
    srv.start(port)
    var listener_fd = srv.listen_sock as int
    // Bounded blocking accept on the pool: the loop stays responsive to `run`
    // without nesting a combinator await inside the coroutine. (A spawned
    // coroutine that awaits `timeout_or`/`select` currently loses its `Context`
    // on LLVM — B25.)
    net::set_recv_timeout(listener_fd as net::Socket, 0, 200000)
    while(srv.run) {
        var accepted = await async::spawn_blocking<int>(|listener_fd|() => net::accept_socket(listener_fd as net::Socket) as int)
        if(accepted <= 0) {
            continue
        }
        var fd = accepted as net::Socket
        net::set_keep_alive(fd, true)
        var s = srv
        var h = async::spawn_blocking<core::async::Unit>(|s, fd|() => {
            s.handle_conn(fd)
            return core::async::Unit { }
        })
    }
    net::close_socket(listener_fd as net::Socket)
    srv.listen_sock = 0u
    srv.shutdown()
    return 0
}

}
