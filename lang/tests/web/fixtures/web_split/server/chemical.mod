module server

// Routes, handlers and SSR assembly. Native only: everything here may use the
// platform, and none of it is compiled by the client job.
source "."

import cstd
import std
import "../shared"
