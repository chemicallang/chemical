module shared

// NEGATIVE fixture for `--web`: `shared` reaches a native symbol, and `client`
// calls that shared function. The shared module is compiled by BOTH jobs, so
// the client (JavaScript) pass must reject it — a bundle that calls a C
// function would only fail in the browser, long after the build.
//
// The same tree also proves the module sets: the server pass compiles
// `shared` + `server` and the client pass compiles `shared` + `client`, so the
// failure names `shared/calc.ch` and the symbol, not a missing module.

source "."

import cstd
import std
