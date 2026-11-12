// safe — additive pointer-free static-server façade.
//
// The runtime entry points stay where they are: `server::main` (CLI) and the
// async `serve_files_async(srv : *mut Server, root : *char, port)` in
// `async.ch`. This file adds the normal-use surface for embedders:
//
//   * `server::serve_static(root : &str, …)` creates and owns the server,
//     installs the static-file routes, and blocks in `Server::serve`;
//   * `server::serve_static_async(root : &str, …)` is the coroutine form;
//   * both take `&str` paths/addresses and never expose a raw pointer or a
//     caller-owned buffer.

public namespace server {

using std::string;

// Blocking static-file server. `root` is served at `/` and `/:path*`.
// Returns 0 when the server shuts down normally.
public func serve_static(root : &std::str, port : uint = 8080u, addr : &std::str = "0.0.0.0") : int {
    var root_text = string()
    root_text.append_view(root.to_view())
    var addr_text = string()
    addr_text.append_view(addr.to_view())

    var cfg = ServerConfig()
    cfg.addr = addr_text

    var srv = Server(cfg)
    install_static_routes(&raw mut srv, root_text.data())
    srv.serve(port)
    return 0
}

// Coroutine static-file server. Drive it with `async::block_on<int>(...)` or
// submit it with `async::raw::spawn<int>(...)`; `srv.shutdown()` stops the accept
// loop. The server and the root string live in this function's async frame for
// the whole run.
public async func serve_static_async(root : &std::str, port : uint = 8080u,
                                     addr : &std::str = "0.0.0.0") : int {
    var root_text = string()
    root_text.append_view(root.to_view())
    var addr_text = string()
    addr_text.append_view(addr.to_view())

    var cfg = ServerConfig()
    cfg.addr = addr_text

    var srv = Server(cfg)
    return await serve_files_async(&raw mut srv, root_text.data(), port)
}

} // namespace server
