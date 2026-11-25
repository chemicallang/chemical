application shared_component

// H31 fixture: a `#universal` component in a module compiled by BOTH `--web`
// passes, rendered by the server `src` with `#html`. The Phase 2 checkpoint kept
// its component client-only; this fixture is the real shape (shared component
// imported by both sides) and must build both sides.
//
// `shared` imports the ambient provider conditionally on the client side (like
// the real `components` library's `import dom if client`), so only the client
// job indexes it and the server job resolves/renders the component without an
// index.
source "src" if server

import std
import page
import html_cbi
import css_cbi
import universal_cbi
import "./shared"
import "./client" if client
