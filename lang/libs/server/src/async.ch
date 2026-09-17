// ---------------------------------------------------------------------------
// Async file server (Tier 6).
//
// `server` is the runtime file-server library. Its synchronous path
// (`Server::serve`) blocks the calling thread in an accept loop
// (`serve_non_iocp`) unless IOCP is used on Windows. This file adds the
// additive coroutine path: the same router + `FileServer`, driven by the Tier 3
// `http::server::serve_coro` accept loop, so the accept wait and every accepted
// connection are awaited on the async runtime instead of pinning a thread.
//
// Additive only: `serve` and `main` are unchanged; `serve_files_async` is a new
// entry point used by `main --async` and available to any embedder.
// ---------------------------------------------------------------------------

public namespace server {

using std::string;

// Add the static-file routes to `srv` and serve `root` until `srv.shutdown()`
// is called. Resolves with 0 on a clean shutdown.
//
// Drive it with `async::block_on<int>(serve_files_async(...))`, or submit it
// with `async::spawn<int>(...)` and stop it by calling `srv.shutdown()` (the
// accept loop observes `srv.run`). The caller owns `srv`; the same pattern as
// the synchronous `Server::S.serve`.
public async func serve_files_async(srv : *mut Server, root : *char, port : uint = 8080u) : int {
    var fs_server = http::create_file_server(root)
    srv.router.add("GET", "/:path*", |&fs_server|(req, res) => {
        fs_server.serve_http(req, res)
    })
    srv.router.add("GET", "/", |&fs_server|(req, res) => {
        fs_server.serve_http(req, res)
    })
    return await serve_coro(srv, port)
}

}
