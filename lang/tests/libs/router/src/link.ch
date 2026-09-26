using namespace std;

// ---------------------------------------------------------------------------
// `<Link>` / `<NavLink>` emission tests (Phase 4, design §6.3/§6.6).
// ---------------------------------------------------------------------------

#universal LinkHost(props) {
    return <div><Link href="/projects">Projects</Link></div>
}

#universal NavLinkHost(props) {
    return <div><NavLink href="/projects" routeId="projects">Projects</NavLink></div>
}

@test
public func test_router_link_emits_anchor(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LinkHost /> }
    var html = page.getHtml()
    const href = std::string_view("href=\"/projects\"")
    if(html.find(&href) == std::NPOS) {
        env.error("<Link> should render its href")
        return
    }
    var js = page.getJs()
    // `router(...)` must lower to the runtime accessor, never a bare `router(`.
    const accessor = std::string_view("window.$__uni_router(")
    if(js.find(&accessor) == std::NPOS) {
        env.error("router(...) should lower to window.$__uni_router(...)")
        return
    }
    const intercept = std::string_view("$__uni_should_intercept")
    if(js.find(&intercept) == std::NPOS) {
        env.error("<Link> should use the shared click predicate")
    }
}

@test
public func test_router_navlink_active_state(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NavLinkHost /> }
    var js = page.getJs()
    const signal = std::string_view("$current.value")
    if(js.find(&signal) == std::NPOS) {
        env.error("<NavLink> should read the $current signal for active state")
    }
    const aria = std::string_view("aria-current")
    if(page.getJs().find(&aria) == std::NPOS) {
        env.error("<NavLink> should emit aria-current (reactive, client-set)")
    }
}
