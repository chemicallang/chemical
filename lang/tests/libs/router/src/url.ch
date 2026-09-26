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

#universal UrlParamPane(props) {
    return <div class="proj">{props.id}</div>
}

#universal UrlParamApp(props) {
    router "main" {
        route default #"home" { <div>Home</div> }
        route "/projects/{id}" { <UrlParamPane /> }
    }
}

@test
public func test_router_server_param_reaches_ssr_props(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42", "")
    #html { <UrlParamApp /> }
    var html = page.getHtml()
    const expected = std::string_view("<div class=\"proj\">42</div>")
    if(html.find(&expected) == std::NPOS) {
        env.error("a deep-linked {id} must reach the route component's SSR props")
        env.info(html.data())
    }
}

// ── Phase 6: nested routes / <Outlet /> ─────────────────────────────────────

#universal NestedChildA(props) {
    return <div class="child-a">A</div>
}

#universal NestedChildB(props) {
    return <div class="child-b">B</div>
}

#universal NestedApp(props) {
    router "main" {
        route default #"home" { <div>Home</div> }
        route #"projects" {
            route default #"list" { <NestedChildA /> }
            route #"detail" { <NestedChildB /> }
            <div class="layout"><main><Outlet /></main></div>
        }
    }
}

@test
public func test_router_nested_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_parameter("main", "projects")
    #html { <NestedApp /> }
    var html = page.getHtml()
    const layout = std::string_view("class=\"layout\"")
    if(html.find(&layout) == std::NPOS) {
        env.error("nested layout markup missing")
        return
    }
    // The nested default (`list`) is active; `detail` ships hidden (eager SSR).
    const listActive = std::string_view("data-uni-route=\"main#projects#list\" data-uni-route-active=\"true\"")
    if(html.find(&listActive) == std::NPOS) {
        env.error("nested default route should render active")
        env.info(html.data())
        return
    }
    const detailInactive = std::string_view("data-uni-route=\"main#projects#detail\" data-uni-route-active=\"false\"")
    if(html.find(&detailInactive) == std::NPOS) {
        env.error("inactive nested route should render hidden")
    }
    // The nested wrappers are registered under the derived router name.
    const nestedRoute = std::string_view("data-uni-route=\"main#projects#list\"")
    if(html.find(&nestedRoute) == std::NPOS) {
        env.error("nested wrapper missing its derived router name")
    }
}

@test
public func test_router_nested_registry_and_activation(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_parameter("main", "projects")
    #html { <NestedApp /> }
    var doc = page.toString()
    const registry = std::string_view("window.$__uni_routers[\"main#projects\"]")
    if(doc.find(&registry) == std::NPOS) {
        env.error("nested router registry missing")
        return
    }
    // The outer route registers the nested router, so activation cascades.
    const nested = std::string_view("nested: \"main#projects\"")
    if(doc.find(&nested) == std::NPOS) {
        env.error("outer route should register its nested router")
    }
    const nestedDefault = std::string_view("nestedDefault: \"list\"")
    if(doc.find(&nestedDefault) == std::NPOS) {
        env.error("outer route should register the nested default")
    }
    const nestedStub = std::string_view("window.$__uni_route_register(\"main#projects\", \"list\"")
    if(doc.find(&nestedStub) == std::NPOS) {
        env.error("nested router should emit its own registration stub")
    }
}

// ── Phase 6: nested routes inherit the outer URL route's params ─────────────

#universal NestedParamChild(props) {
    return <div class="np">{props.id}</div>
}

#universal NestedParamApp(props) {
    router "np" {
        route default #"home" { <div>Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <NestedParamChild /> }
            <div class="npl"><Outlet /></div>
        }
    }
}

@test
public func test_router_nested_param_reaches_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42", "")
    #html { <NestedParamApp /> }
    var html = page.getHtml()
    const expected = std::string_view("class=\"np\">42</div>")
    if(html.find(&expected) == std::NPOS) {
        env.error("a nested child under /projects/{id} should receive the {id} param at SSR")
        env.info(html.data())
    }
}

// ── Phase 6: full URL nesting (segment ownership + activation chain) ────────

#universal NestedUrlPane(props) {
    return <div class="nup">{props.id}</div>
}

#universal NestedUrlApp(props) {
    router "nurl" {
        route default #"home" { <div>Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <NestedUrlPane /> }
            route "/settings" title "Settings" { <NestedUrlPane /> }
            <div class="lay"><Outlet /></div>
        }
    }
}

@test
public func test_router_nested_url_deeplink_selects_child(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42/settings", "")
    #html { <NestedUrlApp /> }
    var html = page.getHtml()
    // The full nested pattern selects the child; the parent layout is visible too.
    const childActive = std::string_view("data-uni-route=\"nurl#/projects/{id}#/settings\" data-uni-route-active=\"true\"")
    if(html.find(&childActive) == std::NPOS) {
        env.error("a deep link to a nested URL route should select the child server-side")
        env.info(html.data())
        return
    }
    const parentActive = std::string_view("data-uni-route=\"nurl#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&parentActive) == std::NPOS) {
        env.error("the outer layout should stay visible for a nested deep link")
    }
    const defaultInactive = std::string_view("data-uni-route=\"nurl#/projects/{id}#overview\" data-uni-route-active=\"false\"")
    if(html.find(&defaultInactive) == std::NPOS) {
        env.error("the nested default should be hidden when a sibling is selected")
    }
    // The full pattern captures the ancestor param and the nested child reads it.
    const id = page.get_parameter("id")
    if(!id.equals(std::string_view("42"))) {
        env.error("the ancestor {id} param should be captured by the full nested pattern")
    }
    const rendered = std::string_view("class=\"nup\">42</div>")
    if(html.find(&rendered) == std::NPOS) {
        env.error("the selected nested child should render the inherited {id}")
        env.info(html.data())
    }
    // The nested route's title is emitted for a nested deep link.
    const title = std::string_view("<title>Settings</title>")
    if(page.toString().find(&title) == std::NPOS) {
        env.error("a nested URL route's title should be emitted server-side")
    }
}

@test
public func test_router_nested_url_parent_selects_default(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/7", "")
    #html { <NestedUrlApp /> }
    var html = page.getHtml()
    const parentActive = std::string_view("data-uni-route=\"nurl#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&parentActive) == std::NPOS) {
        env.error("the parent pattern should match exactly")
        return
    }
    const defaultActive = std::string_view("data-uni-route=\"nurl#/projects/{id}#overview\" data-uni-route-active=\"true\"")
    if(html.find(&defaultActive) == std::NPOS) {
        env.error("the nested default should be visible for the parent URL")
    }
    const childInactive = std::string_view("data-uni-route=\"nurl#/projects/{id}#/settings\" data-uni-route-active=\"false\"")
    if(html.find(&childInactive) == std::NPOS) {
        env.error("the nested child should be hidden for the parent URL")
    }
}

@test
public func test_router_nested_url_unknown_remainder_marks_missing(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42/unknown", "")
    #html { <NestedUrlApp /> }
    page.getHtml()
    if(!page.route_missing()) {
        env.error("an unmatched nested remainder must mark the page route-missing")
    }
}

// A nested URL child under a top-level *id* layout (no URL ancestor): the entry
// id is the id route, so the URL still resolves through the activation chain.
#universal IdRootNestedUrlApp(props) {
    router "idroot" {
        route default #"shell" {
            route "/reports/{id}" { <NestedUrlPane /> }
            <div class="idlay"><Outlet /></div>
        }
    }
}

@test
public func test_router_nested_url_under_id_layout(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/reports/weekly", "")
    #html { <IdRootNestedUrlApp /> }
    var html = page.getHtml()
    const shellActive = std::string_view("data-uni-route=\"idroot#shell\" data-uni-route-active=\"true\"")
    if(html.find(&shellActive) == std::NPOS) {
        env.error("an id layout should activate for a nested URL child")
        env.info(html.data())
        return
    }
    const childActive = std::string_view("data-uni-route=\"idroot#shell#/reports/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&childActive) == std::NPOS) {
        env.error("the nested URL child under an id layout should be selected")
    }
    const rendered = std::string_view("class=\"nup\">weekly</div>")
    if(html.find(&rendered) == std::NPOS) {
        env.error("the nested child should render the captured param")
    }
}

// A layout without an inline `<Outlet />` still emits its nested wrappers (they
// are appended after the layout), and must not be snapshot-cached.
#universal NestedNoOutletApp(props) {
    router "nno" {
        route #"shell" {
            route default #"a" { <NestedUrlPane /> }
            <div class="nno">shell</div>
        }
    }
}

@test
public func test_router_nested_without_inline_outlet_still_renders_children(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_parameter("nno", "shell")
    #html { <NestedNoOutletApp /> }
    var html = page.getHtml()
    const layout = std::string_view("class=\"nno\">shell</div>")
    if(html.find(&layout) == std::NPOS) {
        env.error("the layout should render")
    }
    const child = std::string_view("data-uni-route=\"nno#shell#a\"")
    if(html.find(&child) == std::NPOS) {
        env.error("nested wrappers must still be emitted without an inline <Outlet/>")
        env.info(html.data())
    }
}

// ── nested `route *` fallback (prefix match under a URL layout) ─────────────

#universal NestedFbPane(props) {
    return <div class="nfb-pane">Missing {props.id}</div>
}

#universal NestedFallbackApp(props) {
    router "nfb" {
        route default #"home" { <div>Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <NestedUrlPane /> }
            route "/settings" { <div class="nfb-settings">Settings</div> }
            route * { <NestedFbPane /> }
            <div class="nfb-lay"><Outlet /></div>
        }
    }
}

@test
public func test_router_nested_fallback_catches_unknown_remainder(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42/unknown", "")
    #html { <NestedFallbackApp /> }
    var html = page.getHtml()
    if(page.route_missing()) {
        env.error("a nested `route *` should catch an unknown remainder")
    }
    const fbActive = std::string_view("data-uni-route=\"nfb#/projects/{id}#*\" data-uni-route-active=\"true\"")
    if(html.find(&fbActive) == std::NPOS) {
        env.error("the nested fallback wrapper should be active")
        env.info(html.data())
        return
    }
    const layoutActive = std::string_view("data-uni-route=\"nfb#/projects/{id}\" data-uni-route-active=\"true\"")
    if(html.find(&layoutActive) == std::NPOS) {
        env.error("the outer layout should stay active for a nested fallback")
    }
    const rendered = std::string_view("class=\"nfb-pane\">Missing 42</div>")
    if(html.find(&rendered) == std::NPOS) {
        env.error("the nested fallback should render the inherited {id}")
        env.info(html.data())
    }
}

@test
public func test_router_nested_exact_still_wins_over_fallback(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/42", "")
    #html { <NestedFallbackApp /> }
    var html = page.getHtml()
    const defaultActive = std::string_view("data-uni-route=\"nfb#/projects/{id}#overview\" data-uni-route-active=\"true\"")
    if(html.find(&defaultActive) == std::NPOS) {
        env.error("the nested default must win over the fallback for the bare layout URL")
    }
    const fbInactive = std::string_view("data-uni-route=\"nfb#/projects/{id}#*\" data-uni-route-active=\"false\"")
    if(html.find(&fbInactive) == std::NPOS) {
        env.error("the nested fallback must be hidden for an exact match")
    }
}

@test
public func test_router_nested_url_child_wins_over_fallback(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/projects/7/settings", "")
    #html { <NestedFallbackApp /> }
    var html = page.getHtml()
    const settingsActive = std::string_view("data-uni-route=\"nfb#/projects/{id}#/settings\" data-uni-route-active=\"true\"")
    if(html.find(&settingsActive) == std::NPOS) {
        env.error("the nested URL child must win over the fallback")
        env.info(html.data())
    }
}
