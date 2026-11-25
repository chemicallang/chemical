// Server entry point. It renders the shared component through `#html` into a
// static artifact, so the gate can assert the server really resolved and emitted
// the component's markup (not just that it compiled).
public func main() : int {
    var site = StaticSite.make("output", "app", "assets")
    var page = HtmlPage()
    site.add(&mut page, "index.html")
    page.defaultUniversalSetup()
    render_shared_badge(&mut page)

    if(!site.finish()) {
        unsafe { printf("shared component fixture: %s\n", site.error_message().data()) }
        return 1
    }
    unsafe {
        printf("shared component fixture: artifacts=%llu\n", site.artifact_count())
    }
    return 0
}
