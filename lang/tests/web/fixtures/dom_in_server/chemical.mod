application dom_in_server

// A server-only module reaching for the typed DOM: `dom` is a JavaScript host
// API, so the server pass must refuse it with a compile diagnostic instead of
// emitting a call that only fails at link time.

source "src" if server

import cstd
import std
import dom
import "./server" if server
import "./client" if client
