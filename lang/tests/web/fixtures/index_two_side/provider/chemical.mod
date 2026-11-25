module index_provider

// Phase 2 provider: a marked module that supplies the ambient root `document`.
// It is imported by the app only on the client side, so the server job has no
// provider and never builds an index.
source "."

import std

index_with "universal"
