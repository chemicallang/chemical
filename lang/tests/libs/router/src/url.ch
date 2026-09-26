using namespace std;

// ---------------------------------------------------------------------------
// Server-side URL matching / deep-link selection tests (Phase 5, §6.1/§6.4).
// ---------------------------------------------------------------------------

#universal UrlDeepPane(props) {
    return <div class="pane">P</div>
}

#universal UrlDeepApp(props) {
    router "main" {
        route #"home" { <UrlDeepPane /> }
        route "/projects/{id}" { <UrlDeepPane /> }
        route * { <UrlDeepPane /> }
    }
}

#universal UrlNoFallbackApp(props) {
    router "main" {
        route #"home" { <UrlDeepPane /> }
        route "/projects/{id}" { <UrlDeepPane /> }
    }
}

@test
public func test_router_server_deeplink_selects_route(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42", "")
    #html { <UrlDeepApp /> }
    var html = page.getHtml()
    const active = std::string_view("data-uni-route=\"main#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&active) == std::NPOS) {
        env.error("deep link should select the URL route server-side")
        env.info(html.data())
        return
    }
    // The matched param is stored for the SSR props / activation tail.
    const id = page.get_parameter("id")
    if(!id.equals(std::string_view("42"))) {
        env.error("deep link should store the matched {id} param")
    }
}

@test
public func test_router_server_base_path_stripping(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/app/projects/7", "/app")
    #html { <UrlDeepApp /> }
    var html = page.getHtml()
    const active = std::string_view("data-uni-route=\"main#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&active) == std::NPOS) {
        env.error("base-prefixed URL should strip the base and match")
        return
    }
    const id = page.get_parameter("id")
    if(!id.equals(std::string_view("7"))) {
        env.error("base-prefixed deep link should store the matched param")
    }
}

@test
public func test_router_server_miss_fallback(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/nope", "")
    #html { <UrlDeepApp /> }
    if(page.route_missing()) {
        env.error("a miss with a fallback route should not mark the page missing")
        return
    }
    var html = page.getHtml()
    const active = std::string_view("data-uni-route=\"main#*\" data-uni-route-active=\"true\"")
    if(html.find(&active) == std::NPOS) {
        env.error("a miss should activate the fallback route")
    }
}

@test
public func test_router_server_miss_no_fallback_marks_missing(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/nope", "")
    #html { <UrlNoFallbackApp /> }
    if(!page.route_missing()) {
        env.error("a miss without a fallback should mark the page route-missing")
    }
}

#universal UrlTitleApp(props) {
    router "main" {
        route default #"home" title "Home Page" { <UrlDeepPane /> }
        route "/projects/{id}" title "Project" { <UrlDeepPane /> }
    }
}

@test
public func test_router_server_title_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UrlTitleApp /> }
    var head = page.getHead()
    const t = std::string_view("<title>Home Page</title>")
    if(head.find(&t) == std::NPOS) {
        env.error("the default route's title should be emitted into the head")
    }
}

@test
public func test_router_server_title_deeplink(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/9", "")
    #html { <UrlTitleApp /> }
    var head = page.getHead()
    const t = std::string_view("<title>Project</title>")
    if(head.find(&t) == std::NPOS) {
        env.error("a deep-linked route's title should be emitted into the head")
    }
}

@test
public func test_router_server_noindex_on_miss(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/nope", "")
    #html { <UrlNoFallbackApp /> }
    if(page.getHead().find(&std::string_view("noindex")) == std::NPOS) {
        env.error("a route miss should emit a robots noindex meta")
    }
}

@test
public func test_router_server_percent_decoding(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/a%20b", "")
    #html { <UrlDeepApp /> }
    const id = page.get_parameter("id")
    if(!id.equals(std::string_view("a b"))) {
        env.error("a deep-linked percent-encoded param should be decoded")
        env.info(id.data())
    }
}

@test
public func test_router_server_param_plus_not_space(env : &mut TestEnv) {
    // Server and client must agree: `+` is a literal in a path segment, not a
    // space (decodeURIComponent semantics).
    var page = HtmlPage()
    page.set_route_url("/projects/a+b", "")
    #html { <UrlDeepApp /> }
    const id = page.get_parameter("id")
    if(!id.equals(std::string_view("a+b"))) {
        env.error("a `+` in a path param must stay a literal plus")
        env.info(id.data())
    }
}

@test
public func test_router_server_table_base_written(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/app/projects/7", "/app")
    #html { <UrlDeepApp /> }
    var doc = page.toString()
    const base = std::string_view("window.$__uni_set_table_base(\"main\", \"/app\")")
    if(doc.find(&base) == std::NPOS) {
        env.error("a mount base must be written into the client match table")
    }
}
