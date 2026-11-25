// Client entry point: the same `dom` package, compiled normally on this side.
// No WebView/Node run is needed; the gate checks that the client bundle carries
// the mapped host calls and the `dom` module.
public func main() : int {
    var doc = dom.document()
    var found = doc.getElementById("title")
    if(found is std::Option.Some) {
        return 1
    }
    return 0
}
