// Shared between the server (native) and the client (JavaScript) job. Every
// function here runs identically in both, which is what makes `shared` worth
// having: one spelling of the validation/formatting rules.
//
// Cross-module symbols are referenced unqualified (module imports flatten names
// into scope, like `lang/tests/submod3`), so the names are prefixed to stay
// unambiguous.

public func shared_add(a : int, b : int) : int {
    return a + b
}

// Heap string work in shared code: it has to allocate in both jobs (the natively
// compiled server and the JS client), which is exactly the kind of code the
// "shared is pure logic" rule is meant to allow.
public func shared_label(text : std::string_view) : std::string {
    var out = std::string("value=")
    out.append_view(&text)
    return out
}
