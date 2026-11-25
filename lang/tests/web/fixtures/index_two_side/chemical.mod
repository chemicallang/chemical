application index_two_side

// Phase 2 fixture for scripts/web-two-side-test.sh.
//
// The component is in the client-only module, so only the client job compiles
// and checks it; the server job never sees the component declaration. Its body
// reads `document` inside a lambda (so the render-position hydration check is
// exempt), and the client-only `provider` module supplies the `document` root.
//
// The client module also contains a page renderer using `HtmlPage`/`#html`, so
// the component sits in the same real context as a normal page.
source "src" if server

import std
import html_cbi
import css_cbi
import universal_cbi
import "./provider" if client
import "./client" if client
