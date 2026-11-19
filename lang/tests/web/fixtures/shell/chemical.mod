application shell_fixture

// Phase 0 fixture: the baseline a slot shell must reproduce byte-identically.
//
// One layout (`AppShell`) renders the chrome once and takes the route body as
// `props.children`, so this fixture re-renders the whole tree per render — layout
// included. Both renders are golden snapshots of that full-SSR cost.
//
// The shell+slots mode landed 2026-11-19 (`web::Shell`, `app.shell`); its
// byte-identity and cost gate lives in `scripts/web-app-test.sh` (the `app`
// fixture compares `shell.emit` against a full-SSR twin), so this golden pair
// stays the full-SSR baseline rather than moving to `/dashboard`-style routes.

source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
import universal_cbi
