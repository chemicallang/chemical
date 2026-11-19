// Server-only code that touches the DOM. There is no native implementation of
// `chx_dom_*`, so the shared per-symbol target table (`MIRTargetSupport`) makes
// this a compile diagnostic for the C/LLVM targets.
public func route_title() : int {
    var doc = dom.document()
    var found = doc.get_element_by_id("title")
    if(found is std::Option.Some) {
        return 1
    }
    return 0
}
