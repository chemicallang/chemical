module dom

source "src"

import cstd
import std
import core

// The browser surface is the first ambient provider for the universal component
// body checker: `document`, `window`, `localStorage` and `navigator` are its
// public zero-argument roots. Only the client job indexes it.
index_with "universal"

// The host this module's implementation is written for. A server/native job
// includes `dom` for its declarations (types and signatures) only: it may name
// `dom::Document` but calling a wrapper is refused at the caller.
option host = "js"
