// Server-only code. Two things a client module can never have:
//   1. a native libc call (`getenv`, from cstd) — the JS backend refuses such a
//      symbol by name when it appears in a module the client job compiles;
//   2. this module's own answers (`route_count`), which the client never sees.
public func route_count() : int {
    return shared_add(2, 3)
}

public func home_dir() : *char {
    return unsafe(getenv("HOME"))
}
