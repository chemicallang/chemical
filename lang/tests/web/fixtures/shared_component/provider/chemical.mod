module shared_component_provider

// Client-only provider: supplies the ambient `document` root the shared
// component reads inside its click handler.
source "."

import std

index_with "universal"
