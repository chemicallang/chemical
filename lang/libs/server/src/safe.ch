// safe — additive pointer-free static-server façade.
//
// The runtime entry points stay where they are: `server::main` (CLI) and
// `serve_files_async(srv : Server, root : &str)` in `async.ch`. This file adds
// the normal-use surface for embedders:
//
//   * `server::serve_static(root, addr)` binds, installs the static-file
//     routes and blocks in `Server::serve`;
//   * `server::serve_static_async(root, addr)` is the same on a background
//     thread, returning its `ShutdownHandle` and thread;
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
    var full_addr = string()
    full_addr.append_string(&addr_text)
    full_addr.append(':')
    full_addr.append_uinteger(port)

    var cfg = ServerConfig()
    cfg.addr = full_addr

    var bound = Server.bind(cfg)
    if(bound is std.Result.Err) {
        return 1
    }
    var Ok(bound_server) = bound else unsafe { unreachable }
    var srv : Server = bound_server
    var root_view = std::str.make_view(root_text.to_view())
    install_static_routes(&mut srv, root_view)
    srv.serve()
    return 0
}

// Background static-file server. Take the returned handle to stop it.
public func serve_static_async(root : &std::str, port : uint = 8080u,
                               addr : &std::str = "0.0.0.0") : std.concurrent.Thread {
    var root_text = string()
    root_text.append_view(root.to_view())
    var addr_text = string()
    addr_text.append_view(addr.to_view())
    var full_addr = string()
    full_addr.append_string(&addr_text)
    full_addr.append(':')
    full_addr.append_uinteger(port)

    var cfg = ServerConfig()
    cfg.addr = full_addr

    var bound = Server.bind(cfg)
    if(bound is std.Result.Err) {
        return std.thread.spawn_void(noop_thread, 0)
    }
    var Ok(bound_server) = bound else unsafe { unreachable }
    var srv : Server = bound_server
    var root_view = std::str.make_view(root_text.to_view())
    return serve_files_async(srv, root_view)
}

func noop_thread(ignored : int) { }

} // namespace server
