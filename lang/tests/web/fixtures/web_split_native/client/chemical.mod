module client

// The client entry point. It calls the shared helper, which is where the
// native symbol lives — so the failure is attributed to shared code that the
// client job also compiles, which is the point of the fixture.

source "."

import std
import "../shared"
