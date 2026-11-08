// P2.5 — compile-only coverage for the safe server façade.
//
// `serve_static` / `serve_static_async` bind a port and block (or run until
// shutdown), so this function is intentionally not a `@test`. Being compiled
// (but never called) proves the public surface type-checks with `&str`
// arguments and without any raw pointer in caller code.

func p25_safe_server_api_typechecks() {
    var _rc = server::safe::serve_static(".", 0u, "127.0.0.1")
    var _future = server::safe::serve_static_async(".", 0u, "127.0.0.1")
}
