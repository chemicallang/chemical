module client

// Browser side of the fixture. It owns the universal component and the page
// renderer that uses it, so the client job compiles (and checks) both. The
// server entry point is a separate trivial module and never declares a
// component.
source "."

import std
import page
