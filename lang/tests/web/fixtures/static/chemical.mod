application static_fixture

// Phase 0 fixture: a page that uses only `#html` — no components, no `#js`, no
// universal setup. Its whole point is the promise in §3.20 that a page using
// none of the new features ships **0 bytes of JavaScript**, so the gate asserts
// exactly that on the artifact tree.

source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
