application router_fixture

// Phase 0 fixture: the router on two deep links, plus a miss with and without a
// `route *` fallback. The head is part of the golden output, so the router's
// cheap wins are pinned: a route's `<title>` reaches the head only when that
// route is selected, and a page that misses with no fallback says `noindex`.

source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
import universal_cbi

