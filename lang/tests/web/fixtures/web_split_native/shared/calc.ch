// NEGATIVE fixture: pure-looking logic with one native call in it.
//
// On the server this is ordinary code. In the client job it is a build error:
// `getenv` has no JavaScript equivalent, so the JS backend must refuse it by
// name and point at this file — not emit a call that throws at runtime.

public func shared_platform_label() : std::string {
    var out = std::string("home=")
    out.append_char_ptr(unsafe(getenv("HOME")))
    return out
}

public func shared_double(value : int) : int {
    return value + value
}
