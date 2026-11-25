module host_types_only_client

// Client entry point. `dom` is a real dependency here (the client backend
// implements host "js"), so the module is compiled as usual and its calls map
// to the JS runtime (`rt.dom.*`).
source "."

import std
import dom
