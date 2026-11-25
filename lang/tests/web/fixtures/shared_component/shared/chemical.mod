module shared_component_shared

// Compiled by both `--web` passes. The component and its render site live here,
// so the server pass must resolve the component symbol and the client pass must
// also check its body.
//
// The ambient provider is a client-side dependency of this module, exactly like
// `import dom if client` in the real components library: the server pass has no
// provider/index, the client pass indexes it before this module's body check.
source "."

import std
import page
import "../provider" if client
