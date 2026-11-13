// ---------------------------------------------------------------------------
// Async file server.
//
// `server` is the runtime file-server library. Both drivers share one request
// pipeline (`http::server::Server::serve_ready`), so the async entry point only
// differs in where the accept wait happens: `serve_async` runs the blocking
// accept loop on its own thread, and the coroutine driver will await accepted
// connections once `AsyncRead`/`AsyncWrite` land.
// ---------------------------------------------------------------------------

public namespace server {

using std::string;

/// Install the static-file routes on `srv`. The handler owns a shared
/// `Arc<FileServer>` so it satisfies the router's `Send + Sync + 'static`
/// capture requirement.
public func install_static_routes(srv : &mut Server, root : std::str) {
    var fs_server = std::arc(http.create_file_server(root))
    var fs_server_root = fs_server.clone()
    srv.router.add("GET", "/:path*", |fs_server|(req) => {
        return fs_server.get().serve_http(&req)
    })
    srv.router.add("GET", "/", |fs_server_root|(req) => {
        return fs_server_root.get().serve_http(&req)
    })
}

/// Serve `root` over `srv` on a background thread. Take a `ShutdownHandle`
/// first (`srv.handle()`), because the server is consumed here.
public func serve_files_async(srv : Server, root : std::str) : std::concurrent.Thread {
    var s = srv
    install_static_routes(&mut s, root)
    return s.serve_async()
}

}
