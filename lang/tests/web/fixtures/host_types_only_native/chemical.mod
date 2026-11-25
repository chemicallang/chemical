application host_types_only_native

// Phase 4b: the plain native case (no `--web`). A native job imports `dom`,
// whose `chemical.mod` declares `option host = "js"`, and names
// `*dom::Document` in a signature without calling a wrapper. The module must be
// included for its declarations only, emit no object, and link cleanly.
source "src"

import std
import dom
