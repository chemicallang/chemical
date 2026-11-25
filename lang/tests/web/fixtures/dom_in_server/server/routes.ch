// Server-only code that touches the DOM. `dom` declares `option host = "js"`, so
// this native job includes it for declarations only: naming `dom::Document` is
// fine, but calling a wrapper is refused at this caller with the client-only
// reason (not with an undefined `chx_dom_*` at link time).
public func route_title() : int {
    var doc = dom.document()
    var found = doc.getElementById("title")
    if(found is std::Option.Some) {
        return 1
    }
    return 0
}
