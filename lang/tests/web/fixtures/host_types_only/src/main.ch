// Server entry point. It *names* the DOM type in a signature (`dom::Document`)
// but never calls a wrapper. The server job must compile this with `dom` as a
// declarations-only module: no `chx_dom_*` and no `dom_*` wrapper definitions
// in the server binary, no link error.
public func count_nodes(root : *dom::Document) : int {
    if(root == null) {
        return 0
    }
    return 1
}

public func main() : int {
    // A null handle only exercises the type; no host call is made.
    return count_nodes(null)
}
