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
// Route installation is deliberately synchronous: the handler owns a shared
// `Arc<FileServer>` and is constructed before the coroutine starts. Keeping the
// capturing closure out of the async frame also keeps the frame witness simple
// and avoids two live aliases of the file server in the coroutine lowering.
func install_static_routes(srv : *mut Server, root : *char) {
    var fs_server = std::arc(http::create_file_server(root))
    var fs_server_root = fs_server.clone()
    unsafe {
        srv.router.add("GET", "/:path*", |fs_server|(req, res) => {
            fs_server.get().serve_http(req, res)
        })
        srv.router.add("GET", "/", |fs_server_root|(req, res) => {
            fs_server_root.get().serve_http(req, res)
        })
    }
}

public async func serve_files_async(srv : *mut Server, root : *char, port : uint = 8080u) : int {
    install_static_routes(srv, root)
    return await serve_coro(srv, port)
}

}
