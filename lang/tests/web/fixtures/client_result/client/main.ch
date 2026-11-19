// The client entry point. `shared_parse` returns a `std::Result` and the value
// is dropped: in a browser that is a silently broken tab (the JS target cannot
// unwind), so the client pass must refuse to build this module.
//
// The gate rewrites the line below to `var _ = shared_parse("x")` to show the
// deliberate-discard escape hatch.
public func main() : int {
    shared_parse("x")
    return 0
}
