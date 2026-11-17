// Server entry point (server pass only). The client pass fails first, so this
// is never linked — it exists so the tree is a complete two-sided app.
public func main() : int {
    if(!routes_ready()) { return 1 }
    return 0
}
