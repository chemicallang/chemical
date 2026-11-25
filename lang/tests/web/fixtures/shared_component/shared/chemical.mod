module shared_component_shared

// Compiled by both `--web` passes. The component and its render site live here,
// so the server pass must resolve the component symbol and the client pass must
// also check its body.
//
// The ambient provider is a dependency of both passes (the Phase 4b shape: a
// type-only server import still runs its IndexModule hook), so both jobs index
// it and both run the body check. The older client-only shape (`if client`) is
// exercised by `web-two-side-test.sh` and still skips on the server.
source "."

import std
import page
import "../provider"
