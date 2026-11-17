application web_split_native

// Negative fixture for `--web`: `shared` reaches a native symbol (`getenv`), and
// `client` reaches that shared function. The client pass must fail with the JS
// backend's own diagnostic (symbol + function + source location) instead of
// emitting a bundle that traps in the browser.
//
// The same tree also proves the module sets: the server pass compiles
// shared + server, the client pass compiles shared + client — the failure is in
// the client pass, and it names `shared/calc.ch`.

source "src" if server

import cstd
import std
import "./shared"
import "./server" if server
import "./client" if client
