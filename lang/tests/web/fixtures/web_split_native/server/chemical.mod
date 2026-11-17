module server

// Server-only code for the negative fixture: it compiles fine (native `getenv`
// is allowed here), which proves the failure in the client pass is about the
// target, not about the code being invalid.

source "."

import cstd
import std
import "../shared"
