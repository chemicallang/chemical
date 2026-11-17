// NEGATIVE fixture server code: valid on the native side, so the build failure
// the gate expects can only come from the client pass.

public func routes_ready() : bool {
    return shared_double(21) == 42
}
