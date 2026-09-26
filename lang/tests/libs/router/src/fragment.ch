using namespace std;

// ---------------------------------------------------------------------------
// Remote-route fragment responses (§6.7, D-6.7): `set_route_fragment` +
// `route_fragment_response` must return the route's markup alone, wrapped in a
// `data-chx-i` boundary, and nothing else (the §8.2 byte-exactness contract).
// ---------------------------------------------------------------------------

#universal FragPane(props) {
    return <div data-testid="frag-pane">Remote Body</div>
}

#universal FragApp(props) {
    router "main" {
        route default #"home" { <div data-testid="frag-home">Home</div> }
        route "/reports" remote { <FragPane /> }
    }
}

@test
public func test_router_remote_route_ships_no_body_normally(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_url("/reports", "")
    #html { <FragApp /> }
    var html = page.getHtml()
    if(html.find(std::string_view("frag-pane")) != std::NPOS) {
        env.error("a remote route must not server-render its body on a normal page")
        env.info(html.data())
    }
    // A page that is not a fragment request yields no fragment response.
    if(page.route_fragment_response("main", "/reports").size() != 0) {
        env.error("a normal page must not produce a fragment response")
    }
}

@test
public func test_router_fragment_response_returns_only_the_route(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_fragment("main", "/reports")
    #html { <FragApp /> }
    var frag = page.route_fragment_response("main", "/reports")
    if(frag.size() == 0) {
        env.error("a fragment request should produce a fragment response")
        return
    }
    // The route's markup is present and wrapped in a client-resolvable boundary.
    if(frag.find(std::string_view("frag-pane")) == std::NPOS) {
        env.error("the fragment should contain the remote route's markup")
        env.info(frag.data())
    }
    if(frag.find(std::string_view("data-chx-i")) == std::NPOS) {
        env.error("the fragment must carry a data-chx-i boundary for $__uni_mount_fragment")
    }
    // Byte-exactness: nothing from the other routes or the page shell leaks in.
    if(frag.find(std::string_view("frag-home")) != std::NPOS) {
        env.error("the fragment must not contain another route's markup")
        env.info(frag.data())
    }
    if(frag.find(std::string_view("<html")) != std::NPOS || frag.find(std::string_view("chx-route")) != std::NPOS) {
        env.error("the fragment must not contain the page shell or wrapper")
        env.info(frag.data())
    }
}

@test
public func test_router_fragment_requested_is_scoped_to_router_and_id(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_fragment("main", "/reports")
    if(!page.route_fragment_requested("main", "/reports")) {
        env.error("the fragment request should be scoped to its router and id")
    }
    if(page.route_fragment_requested("other", "/reports")) {
        env.error("a fragment request must not match a different router")
    }
    if(page.route_fragment_requested("main", "/elsewhere")) {
        env.error("a fragment request must not match a different id")
    }
    // The route is selected, so its wrapper renders active for the fragment.
    if(!page.route_selected("main", "/reports", "")) {
        env.error("set_route_fragment should select the target route")
    }
}

@test
public func test_router_fragment_response_for_unknown_route_is_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_route_fragment("main", "/not-a-route")
    #html { <FragApp /> }
    if(page.route_fragment_response("main", "/not-a-route").size() != 0) {
        env.error("a fragment request for an undeclared route should produce no response")
    }
}
