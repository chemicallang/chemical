module dom

source "src"

import cstd
import std
import core

// The browser surface is the first ambient provider for the universal component
// body checker: `document`, `window`, `localStorage` and `navigator` are its
// public zero-argument roots. Only the client job indexes it.
index_with "universal"
