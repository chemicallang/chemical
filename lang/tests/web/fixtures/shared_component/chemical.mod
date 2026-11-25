application shared_component

// H31 fixture: a `#universal` component in a module compiled by BOTH `--web`
// passes, rendered by the server `src` with `#html`. The Phase 2 checkpoint kept
// its component client-only; this fixture is the real shape (shared component
// imported by both sides) and must build both sides.
//
// `shared` imports the ambient provider on both sides (the Phase 4b shape: a
// server job includes a declarations-only provider and still runs its
// IndexModule hook), so both jobs index it and both body-check the component.
// `scripts/web-two-pass-test.sh` also builds a client-only-provider copy to
// prove the server skips the check when no provider is present.
source "src" if server

import std
import page
import html_cbi
import css_cbi
import universal_cbi
import "./shared"
import "./client" if client
