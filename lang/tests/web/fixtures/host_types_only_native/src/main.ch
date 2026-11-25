// Plain native entry: only the type crosses the boundary, never a host call.
public func describe(root : *dom::Document) : int {
    if(root == null) {
        return 0
    }
    return 1
}

public func main() : int {
    return describe(null)
}
