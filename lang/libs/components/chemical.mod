module components

source "src"

import page
import std
import universal_cbi
import css_cbi
// Unconditional: the server job includes `dom` for its declarations only
// (`option host = "js"`), so the ambient index is built on both sides and the
// component body checker runs over this corpus in both `--web` passes. Calls to
// the host wrappers are still refused on the server at the caller.
import dom