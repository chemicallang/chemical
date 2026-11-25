// Client entry point. It renders the shared component too, so the component is
// reachable from the client root and its hydration JavaScript lands in the
// bundle (the gate asserts it does). The component is the same declaration the
// server pass compiled and rendered.
public func main() : int {
    var page = HtmlPage()
    render_shared_badge(&mut page)
    return 0
}
