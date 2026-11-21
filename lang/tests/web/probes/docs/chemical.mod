application docs_probe

// Probe 5 of the web-stack survey: a documentation site — the shape that is
// *almost* the blog but has a sectioned layout, active navigation, breadcrumbs,
// prev/next links, per-page canonical metadata, and a search index.
//
// It is the cheapest possible output (no JS, no hydration) and the hardest to
// write with the current API, because layout composition is per-site code and the
// build can only export HTML routes (there is nowhere to put `search-index.json`
// or an RSS feed).
source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
import universal_cbi
import web
import md
