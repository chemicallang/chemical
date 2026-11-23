application dom_async

// A `--web` app whose client entry point *awaits* a host `fetch` (the
// promise-based async path, `js-backend-abi.md` §17). The server pass produces a
// trivial executable: the point of the fixture is the emitted client JavaScript.
//
// `scripts/web-dom-test.sh` builds the two jobs and runs the bundle under
// `scripts/js_backend/dom_harness.mjs`, whose `fetch` is a real host promise.

source "src" if server

import cstd
import std
import "./client" if client
