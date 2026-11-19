application client_result

// Client code must consume a `std::Result` (lang/docs/web-stack-plan.md §Phase 3
// item 6). The same source is legal on the server, so this tree proves the rule
// is scoped to the client job: the client's `main` discards a Result and the
// client pass must reject the build, while `src/main.ch` (server only) discards
// the very same call and is never asked to.

source "src" if server
import cstd
import std
import "./shared"
import "./client" if client
