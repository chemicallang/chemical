application host_types_only

// Phase 4b: a native/server job imports `dom` (whose `chemical.mod` declares
// `option host = "js"`). The module is included for its declarations only:
// the server may name `dom::Document` in a signature, but the module emits no
// object and no `chx_dom_*` symbols. The client pass compiles `dom` normally.
//
// `dom_in_server` is the negative half: an actual call into a `dom` wrapper is
// refused at the caller (scripts/web-types-only-test.sh).
source "src" if server

import std
import dom
import "./client" if client
