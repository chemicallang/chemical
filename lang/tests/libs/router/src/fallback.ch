using namespace std;

// ---------------------------------------------------------------------------
// `props.__path` on the fallback/404 route (§12.7).
// ---------------------------------------------------------------------------

#universal NotFoundPane(props) {
    return <div class="nf">{props.__path}</div>
}

#universal NotFoundApp(props) {
    router "main" {
        route "/home" { <div class="home">H</div> }
        route * { <NotFoundPane /> }
    }
}

@test
public func test_router_fallback_receives_path(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/does-not-exist")
    #html { <NotFoundApp /> }
    var html = page.getHtml()
    if(html.find(std::string_view("/does-not-exist")) == std::NPOS) {
        env.error("the fallback body should receive the requested path as props.__path")
        env.info(html.data())
    }
}

@test
public func test_router_fallback_path_is_escaped(env : &mut TestEnv) {
    // A hostile path must not break out of the fallback markup (§12.7).
    var page = HtmlPage()
    page.set_route_url("/<script>alert(1)</script>")
    #html { <NotFoundApp /> }
    var html = page.getHtml()
    if(html.find(std::string_view("<script>alert(1)</script>")) != std::NPOS) {
        env.error("a hostile path must be escaped in the 404 body")
        env.info(html.data())
        return
    }
    if(html.find(std::string_view("&lt;script&gt;")) == std::NPOS) {
        env.error("the escaped path should still be visible")
        env.info(html.data())
    }
}
