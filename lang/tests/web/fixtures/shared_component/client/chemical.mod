module shared_component_client

// Browser side entry point. It imports the shared module, so the client job also
// compiles and body-checks the shared component.
source "."

import std
import page
import "../shared"
