using namespace std;

// ---------------------------------------------------------------------------
// Server-side redirects — `redirect_to` / `get_route_redirect` (§6.7-F).
// ---------------------------------------------------------------------------

#universal RedirectPane(props) {
    return <div class="pane">R</div>
}

#universal RedirectApp(props) {
    router "main" {
        route #"home" { <RedirectPane /> }
        route "/projects/{id}" { <RedirectPane /> }
        route "/dashboard" { <RedirectPane /> }
        route * { <RedirectPane /> }
    }
}

@test
public func test_router_redirect_to_selects_target(env : &mut TestEnv) {
    var page = HtmlPage()
    page.redirect_to("main", "home")
    if(!page.route_selected("main", "home", "")) {
        env.error("redirect_to should select the target route")
    }
    if(page.route_selected("main", "dashboard", "")) {
        env.error("redirect_to should not select a non-target route")
    }
}

@test
public func test_router_redirect_to_records_path(env : &mut TestEnv) {
    var page = HtmlPage()
    page.redirect_to("main", "dashboard", "/dashboard")
    if(!page.get_route_redirect().equals(std::string_view("/dashboard"))) {
        env.error("get_route_redirect should return the target path")
        env.info(page.get_route_redirect().data())
    }
    // The path also drives the matcher, so an in-place render follows it.
    if(!page.get_route_url().equals(std::string_view("/dashboard"))) {
        env.error("a redirect path should be stored as the route url")
    }
}

@test
public func test_router_redirect_without_path_has_no_redirect(env : &mut TestEnv) {
    var page = HtmlPage()
    page.redirect_to("main", "home")
    if(page.get_route_redirect().size() != 0) {
        env.error("a redirect without a path should record no path")
    }
}

@test
public func test_router_redirect_overrides_url_match(env : &mut TestEnv) {
    // Request URL matches `/projects/{id}`, but the handler redirects to
    // `/dashboard` (with a path). The rendered page must show `dashboard`, not
    // the route the request URL matched.
    var page = HtmlPage()
    page.set_route_url("/projects/42", "")
    page.redirect_to("main", "dashboard", "/dashboard")
    #html { <RedirectApp /> }
    var html = page.getHtml()
    const dashActive = std::string_view("data-uni-route=\"main#/dashboard\" data-uni-route-active=\"true\"")
    if(html.find(&dashActive) == std::NPOS) {
        env.error("a redirect should win over the request URL match")
        env.info(html.data())
        return
    }
    const projActive = std::string_view("data-uni-route=\"main#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&projActive) != std::NPOS) {
        env.error("the URL match must not stay active after a redirect")
    }
}
