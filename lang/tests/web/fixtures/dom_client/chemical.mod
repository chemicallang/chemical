application dom_client

// A `--web` app whose interaction is written in `client/*.ch` with the typed `dom`
// module (proposal 0008). The client pass compiles `shared` + `client` to JS, the
// server pass compiles `shared` + `server` + `src` to a native executable.
//
// `scripts/web-dom-test.sh` runs the emitted bundle under Node with a small DOM
// harness (`scripts/js_backend/dom_harness.mjs`), so the gate checks the real
// emitted JavaScript against a real host API — not the HTML the server would make.

source "src" if server

import cstd
import std
import "./shared"
import "./server" if server
import "./client" if client
