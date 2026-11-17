application shell_fixture

// Phase 0 fixture: the baseline a slot shell must reproduce byte-identically.
//
// One layout (`AppShell`) renders the chrome once and takes the route body as
// `props.children`, so today a personalised page re-renders the whole tree —
// layout included. Both renders are golden snapshots: when `web::Shell` lands in
// Phase 4, `shell.emit(fills)` must produce these same bytes while doing no
// component work for the chrome (`shell.emit(empty)` vs full SSR is the §3.19
// gate).

source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
import universal_cbi
