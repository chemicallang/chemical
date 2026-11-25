application body_ok_deep_chains

// Phase 5b fixture for scripts/web-body-check-test.sh: each component contains
// only a fully typed ambient chain, so its untyped count must be 0. A broken
// Option unwrap, container element model or dom member lookup turns the edge
// into WEB-BODY-005 (or 002) and the gate fails.
source "src" if server

import std
import html_cbi
import css_cbi
import universal_cbi
import dom if client
import "./client" if client
