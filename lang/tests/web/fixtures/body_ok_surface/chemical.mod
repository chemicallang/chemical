application body_ok_surface

// Phase 3b fixture for scripts/web-body-check-test.sh.
// The component is client-only so only the client job compiles it; the
// server job is a trivial native entry and must not run the body checker.
source "src" if server

import std
import html_cbi
import css_cbi
import universal_cbi
import dom if client
import "./client" if client
