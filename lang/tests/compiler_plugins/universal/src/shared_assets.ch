// =============================================================================
// Shared JS/CSS bundles across a set of pages (SharedAssets).
//
// A page opts in with `page.attach_shared(shared)`. While attached,
// page-independent appends (the hydration runtime, component definitions, and
// component classes) go into the shared sink and are de-duplicated across every
// attached page, while dispatch statements stay on the page. These tests pin
// that contract.
// =============================================================================

func shared_test_style(page : &mut HtmlPage) : *char {
    return #css {
        color: rgb(7, 8, 9);
    }
}

func shared_build_greeting(page : &mut HtmlPage) {
    #html { <Greeting /> }
}

// Counts non-overlapping occurrences of `needle` in `hay`.
func shared_count(hay : &std::string_view, needle : &std::string_view) : int {
    if(needle.size() == 0 || hay.size() < needle.size()) { return 0 }
    var count = 0
    var pos : size_t = 0
    while(pos + needle.size() <= hay.size()) {
        const found = hay.subview(pos, hay.size()).find(needle)
        if(found == std::NPOS) { break }
        count = count + 1
        pos = pos + found + needle.size()
    }
    return count
}

@test
public func shared_assets_bundle_has_no_dispatch_statements(env : &mut TestEnv) {
    var shared = shared_assets("test")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <Greeting /> }

    var bundle = std::string()
    bundle.append_view(shared.js())
    const dispatch_stmt = std::string_view("window.$__uni_dispatch('")
    if(bundle.contains(&dispatch_stmt)) {
        env.error("shared bundle must not contain page-specific dispatch statements")
        return
    }
    env.success("shared bundle contains no dispatch statements")
}

@test
public func shared_assets_local_js_has_dispatch(env : &mut TestEnv) {
    var shared = shared_assets("test")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <Greeting /> }

    var local = page.local_js()
    const dispatch_stmt = std::string_view("window.$__uni_dispatch('")
    if(!local.contains(&dispatch_stmt)) {
        env.error("page local js must contain its dispatch statement")
        return
    }
    if(local.contains("function universal_lib_test_Greeting")) {
        env.error("page local js must not contain component definitions")
        return
    }
    env.success("page local js has its dispatch and no definition")
}

@test
public func shared_assets_dedupes_definitions_across_pages(env : &mut TestEnv) {
    var shared = shared_assets("test")

    var p1 = HtmlPage()
    p1.attach_shared(shared)
    p1.defaultUniversalSetup()
    shared_build_greeting(&mut p1)

    var p2 = HtmlPage()
    p2.attach_shared(shared)
    p2.defaultUniversalSetup()
    shared_build_greeting(&mut p2)

    var bundle = std::string()
    bundle.append_view(shared.js())
    var bundle_view = bundle.to_view()
    const sig = std::string_view("universal_lib_test_Greeting(props)")
    const n = shared_count(&bundle_view, &sig)
    if(n != 1) {
        env.error("shared bundle must define a shared component exactly once")
        env.info(bundle.data())
        return
    }
    env.success("shared component definition de-duplicated across pages")
}

@test
public func shared_assets_dedupes_css_across_pages(env : &mut TestEnv) {
    var shared = shared_assets("test")

    var p1 = HtmlPage()
    p1.attach_shared(shared)
    shared_test_style(&mut p1)

    var p2 = HtmlPage()
    p2.attach_shared(shared)
    shared_test_style(&mut p2)

    var css = std::string()
    css.append_view(shared.css())
    var css_view = css.to_view()
    const marker = std::string_view("rgb(7 8 9)")
    const n = shared_count(&css_view, &marker)
    if(n != 1) {
        env.error("shared CSS must contain a repeated class exactly once")
        env.info(css.data())
        return
    }
    env.success("shared component class de-duplicated across pages")
}

@test
public func unattached_page_still_owns_its_js(env : &mut TestEnv) {
    // A page that never attaches a shared sink keeps today's behaviour:
    // definitions and the dispatch both live on the page.
    var page = HtmlPage()
    #html { <Greeting /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(!js.contains("function universal_lib_test_Greeting")) {
        env.error("unattached page must contain its component definition")
        return
    }
    if(!js.contains("window.$__uni_dispatch('")) {
        env.error("unattached page must contain its dispatch")
        return
    }
    env.success("unattached page owns its definitions and dispatch")
}

@test
public func shared_assets_runtime_emitted_once(env : &mut TestEnv) {
    var shared = shared_assets("test")
    var p1 = HtmlPage()
    p1.attach_shared(shared)
    p1.defaultUniversalSetup()
    var p2 = HtmlPage()
    p2.attach_shared(shared)
    p2.defaultUniversalSetup()

    var bundle = std::string()
    bundle.append_view(shared.js())
    var bundle_view = bundle.to_view()
    const marker = std::string_view("$__uni_hydration_queue = []")
    if(shared_count(&bundle_view, &marker) != 1) {
        env.error("the hydration runtime must be emitted exactly once into the shared sink")
        return
    }
    env.success("shared runtime emitted once across pages")
}

// Regression: the original bug was a "shared" bundle that still contained the
// page-specific `window.$__uni_dispatch('Comp', getElementById('u<loc>'), ...)`
// statements. With a shared sink, those statements stay on the page and the
// bundle is page-independent.
@test
public func feature_external_bundle_has_no_page_specific_dispatch(env : &mut TestEnv) {
    var shared = shared_assets("test")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <PosTrackingComp x={42} /> }

    var bundle = std::string()
    bundle.append_view(shared.js())
    const dispatch_stmt = std::string_view("window.$__uni_dispatch('")
    if(bundle.contains(&dispatch_stmt)) {
        env.error("the shared bundle still contains a page-specific hydration dispatch")
        return
    }
    var local = page.local_js()
    if(!local.contains(&dispatch_stmt)) {
        env.error("the page must keep its own dispatch statement")
        return
    }
    env.success("external bundle is free of page-specific hydration dispatches")
}

// =============================================================================
// Umbrella warm page + frozen sink (the request-time workflow)
// =============================================================================
//
// Build the shared bundle once, before serving, from ONE umbrella component
// that transitively uses the rest. At request time a page attaches the already
// complete sink, so the render adds nothing to it — that "sink did not grow"
// check is the assert an app should run per request (and in tests).

#universal SharedChild(props) {
    return <span class="shared-child">child</span>
}

#universal SharedUmbrella(props) {
    return <div class="shared-umbrella"><SharedChild /></div>
}

func shared_build_umbrella(page : &mut HtmlPage) {
    #html { <SharedUmbrella /> }
}

func shared_build_child(page : &mut HtmlPage) {
    #html { <SharedChild /> }
}

@test
public func shared_assets_umbrella_pulls_transitive_definitions(env : &mut TestEnv) {
    var shared = shared_assets("umbrella")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <SharedUmbrella /> }

    var bundle = std::string()
    bundle.append_view(shared.js())
    var bundle_view = bundle.to_view()
    if(!bundle_view.contains("SharedUmbrella(props)")) {
        env.error("the bundle must define the umbrella component")
        return
    }
    if(!bundle_view.contains("SharedChild(props)")) {
        env.error("rendering the umbrella must pull in the components it uses")
        return
    }
    const dispatch_stmt = std::string_view("window.$__uni_dispatch('")
    if(bundle_view.contains(&dispatch_stmt)) {
        env.error("the bundle must stay dispatch-free")
        return
    }
    env.success("umbrella warm brings in transitive component definitions")
}

@test
public func shared_assets_request_render_does_not_grow_the_sink(env : &mut TestEnv) {
    // Warm once from the umbrella ...
    var shared = shared_assets("frozen")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <SharedUmbrella /> }

    // ... snapshot, then render a real page attached (as a request would).
    const js_before = shared.js_size()
    const css_before = shared.css_size()

    var req = HtmlPage()
    req.attach_shared(shared)
    req.defaultUniversalSetup()
    shared_build_child(&mut req)

    if(shared.js_size() != js_before) {
        env.error("a request render must not add js to the already-warmed sink")
        return
    }
    if(shared.css_size() != css_before) {
        env.error("a request render must not add css to the already-warmed sink")
        return
    }
    const dispatch_stmt = std::string_view("window.$__uni_dispatch('")
    var local = req.local_js()
    if(!local.contains(&dispatch_stmt)) {
        env.error("the page must still carry its own dispatch statement")
        return
    }
    env.success("request render leaves the warmed sink unchanged")
}

// A component missing from the warm set is NOT silently fine: its definition is
// written into the sink by the request render, so the sink grows — and the page
// has no local copy, so it would have no definition to hydrate from. This pins
// the failure mode that the "sink did not grow" assert exists to catch.
@test
public func shared_assets_missing_component_grows_the_sink(env : &mut TestEnv) {
    var shared = shared_assets("missing")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    #html { <SharedChild /> }

    const js_before = shared.js_size()

    var req = HtmlPage()
    req.attach_shared(shared)
    req.defaultUniversalSetup()
    shared_build_umbrella(&mut req)

    if(shared.js_size() == js_before) {
        env.error("a missing component should have grown the sink (updating this expectation means the routing changed)")
        return
    }
    if(req.local_js().contains("SharedUmbrella(props)")) {
        env.error("the missing definition must not be on the page today")
        return
    }
    env.success("a missing component grows the sink — the assert catches it")
}
