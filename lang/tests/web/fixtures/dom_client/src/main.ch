// Server entry point: it exists only in the server pass.
public func main() : int {
    var page = route_index()
    var view = page.to_view()
    unsafe { printf("server: %zu bytes of HTML\n", view.size()) }
    return 0
}
