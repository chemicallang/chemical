using namespace std;

// ---------------------------------------------------------------------------
// Universal router emission tests (Phase 3, design §4.3 / §15.3).
//
// These assert the SSR wrappers, the client registry/stubs and the activation
// tail without needing a WebView: the generated server function is executed by
// the test binary and its page buffers inspected.
// ---------------------------------------------------------------------------

#universal RouterPane(props) {
    return <div class="pane">Pane</div>
}

#universal RouterApp(props) {
    router "main" {
        route default #"dashboard" { <RouterPane /> }
        route #"projects" { <RouterPane /> }
    }
}

@test
public func router_emits_ssr_wrappers(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterApp /> }
    var html = page.getHtml()
    const wrapper = std::string_view("class=\"chx-route\" id=\"r")
    if(html.find(&wrapper) == std::NPOS) {
        env.error("router wrapper element missing from SSR output")
        return
    }
    const dashRoute = std::string_view("data-uni-route=\"main#dashboard\"")
    if(html.find(&dashRoute) == std::NPOS) {
        env.error("default route wrapper missing")
        return
    }
    const projRoute = std::string_view("data-uni-route=\"main#projects\"")
    if(html.find(&projRoute) == std::NPOS) {
        env.error("second route wrapper missing")
        return
    }
    const boundary = std::string_view("data-chx-i")
    if(html.find(&boundary) == std::NPOS) {
        env.error("route boundary span missing")
        return
    }
    const active = std::string_view("data-uni-route-active=\"true\"")
    if(html.find(&active) == std::NPOS) {
        env.error("declared default route should render active")
    }
}

@test
public func router_emits_client_registry_and_stubs(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterApp /> }
    var js = page.getJs()
    const registry = std::string_view("window.$__uni_routers[\"main\"]")
    if(js.find(&registry) == std::NPOS) {
        env.error("router registry object missing from pageJs")
        return
    }
    const stub = std::string_view("window.$__uni_route_register(\"main\", \"dashboard\"")
    if(js.find(&stub) == std::NPOS) {
        env.error("route registration stub missing from pageJs")
        return
    }
    const compFn = std::string_view("function universal_lib_test_RouterPane(props)")
    if(js.find(&compFn) == std::NPOS) {
        env.error("route component client function missing")
        return
    }
    // The stub must reference a defined function: component definition first.
    if(js.find(&compFn) > js.find(&stub)) {
        env.error("route component function must be defined before its stub")
        return
    }
    // Route bodies must NOT enter the hydration queue (INV-7).
    const dispatch = std::string_view("$__uni_dispatch('universal_lib_test_RouterPane'")
    if(js.find(&dispatch) != std::NPOS) {
        env.error("route body must not be dispatched through the hydration queue")
    }
}

@test
public func router_emits_activation_tail(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterApp /> }
    var doc = page.toString()
    const tail = std::string_view("window.$__uni_activate_initial(\"main\", \"dashboard\"")
    if(doc.find(&tail) == std::NPOS) {
        env.error("initial activation tail missing")
    }
    const runtime = std::string_view("window.$__uni_router_version = 1;")
    if(doc.find(&runtime) == std::NPOS) {
        env.error("router runtime missing when a router is declared")
    }
}

@test
public func router_absent_without_declaration(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterPane /> }
    var doc = page.toString()
    const runtime = std::string_view("$__uni_router_version")
    if(doc.find(&runtime) != std::NPOS) {
        env.error("a page without a router must not ship router bytes")
    }
}

#universal RouterPreloadApp(props) {
    router "main" {
        route #"home" { <RouterPane /> }
        route #"admin" preload { <RouterPane /> }
    }
}

@test
public func router_preload_mode_hydrates_at_load(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterPreloadApp /> }
    var js = page.getJs()
    const preload = std::string_view("window.$__uni_preload(\"main\", \"admin\");")
    if(js.find(&preload) == std::NPOS) {
        env.error("a `preload` route should emit a bootstrap preload call")
    }
    const noPreload = std::string_view("window.$__uni_preload(\"main\", \"home\");")
    if(js.find(&noPreload) != std::NPOS) {
        env.error("a lazy route must not be preloaded")
    }
}

#universal RouterRemoteApp(props) {
    router "main" {
        route #"home" { <RouterPane /> }
        route "/reports" remote { <RouterPane /> }
    }
}

@test
public func router_remote_mode_ships_no_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterRemoteApp /> }
    var js = page.getJs()
    const remoteFlag = std::string_view("remote: true")
    if(js.find(&remoteFlag) == std::NPOS) {
        env.error("a `remote` route should register with remote: true")
    }
    // The remote route's body must not be server-rendered: only the home route
    // contributes the pane markup.
    var html = page.getHtml()
    const pane = std::string_view("class=\"pane\"")
    const first = html.find(&pane)
    if(first == std::NPOS) {
        env.error("home route markup missing")
        return
    }
    const second = html.subview(first + 1, html.size()).find(&pane)
    if(second != std::NPOS) {
        env.error("remote route must not server-render its body")
    }
}

#universal RouterHookApp(props) {
    router "main" {
        route #"home" {
            onActivate(() => { hookA += 1 })
            onDeactivate(() => { hookB += 1 })
            onBeforeActivate(() => { hookC = true })
            <RouterPane />
        }
    }
}

@test
public func router_hooks_registered(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterHookApp /> }
    var js = page.getJs()
    const activate = std::string_view("onActivate: ")
    if(js.find(&activate) == std::NPOS) {
        env.error("route stub should carry the onActivate hook")
        return
    }
    const before = std::string_view("beforeActivate: ")
    if(js.find(&before) == std::NPOS) {
        env.error("route stub should carry the onBeforeActivate hook")
        return
    }
    const fired = std::string_view("hookA")
    if(js.find(&fired) == std::NPOS) {
        env.error("hook function body was not emitted")
    }
    const guard = std::string_view("hookC")
    if(js.find(&guard) == std::NPOS) {
        env.error("guard hook function body was not emitted")
    }
}


#universal RouterUrlApp(props) {
    router "main" {
        route #"home" { <RouterPane /> }
        route "/projects/{id}" { <RouterPane /> }
        route * { <RouterPane /> }
    }
}

@test
public func router_url_table_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterUrlApp /> }
    var js = page.getJs()
    const table = std::string_view("table = { base: \"\", routes: [")
    if(js.find(&table) == std::NPOS) {
        env.error("URL router should emit a client match table")
        return
    }
    const entry = std::string_view("{ pattern: [\"projects\", \"{id}\"], id: \"/projects/{id}\" }")
    if(js.find(&entry) == std::NPOS) {
        env.error("match table should contain the URL pattern")
        return
    }
    const fallback = std::string_view("{ fallback: true, id: \"*\" }")
    if(js.find(&fallback) == std::NPOS) {
        env.error("match table should contain the fallback entry")
        return
    }
    const placeholder = std::string_view("baseProps: {\"id\": null}")
    if(js.find(&placeholder) == std::NPOS) {
        env.error("URL route baseProps should carry a null placeholder per param")
    }
    const isUrl = std::string_view("isUrl: true")
    if(js.find(&isUrl) == std::NPOS) {
        env.error("URL route stub should set isUrl: true")
    }
}

#universal RouterOrderApp(props) {
    router "main" {
        route "/a/{x}" { <RouterPane /> }
        route "/a/b" { <RouterPane /> }
        route * { <RouterPane /> }
    }
}

@test
public func router_url_precedence_order(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterOrderApp /> }
    var js = page.getJs()
    const literalEntry = std::string_view("{ pattern: [\"a\", \"b\"], id: \"/a/b\" }")
    const paramEntry = std::string_view("{ pattern: [\"a\", \"{x}\"], id: \"/a/{x}\" }")
    const li = js.find(&literalEntry)
    const pi = js.find(&paramEntry)
    if(li == std::NPOS || pi == std::NPOS) {
        env.error("both URL patterns should be in the match table")
        return
    }
    if(li > pi) {
        env.error("the more-literal pattern must precede the param pattern (D-6.2)")
    }
    // fallback stays last
    const fb = std::string_view("{ fallback: true, id: \"*\" }")
    if(js.find(&fb) < pi) {
        env.error("the fallback entry must be emitted last")
    }
}

#universal RouterNoScrollApp(props) {
    router "main" {
        route #"a" noscroll { <RouterPane /> }
    }
}

@test
public func router_noscroll_flag(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterNoScrollApp /> }
    var js = page.getJs()
    const flag = std::string_view("noscroll: true")
    if(js.find(&flag) == std::NPOS) {
        env.error("a `noscroll` route should register with noscroll: true")
    }
}

@test
public func router_url_sync_installed_for_url_routers(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterUrlApp /> }
    var doc = page.toString()
    const sync = std::string_view("window.$__uni_sync_url(\"main\")")
    if(doc.find(&sync) == std::NPOS) {
        env.error("a URL router should install the popstate sync")
    }
}

@test
public func router_id_router_has_no_popstate_sync(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RouterApp /> }
    var doc = page.toString()
    const sync = std::string_view("$__uni_sync_url(\"main\")")
    if(doc.find(&sync) != std::NPOS) {
        env.error("an id-only router must not install popstate sync")
    }
}
