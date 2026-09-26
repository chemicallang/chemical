using namespace std;

// ---------------------------------------------------------------------------
// Universal router client-runtime emission tests (Phase 2)
// ---------------------------------------------------------------------------

@test
public func test_router_runtime_emitted_once(env : &mut TestEnv) {
    var page = HtmlPage()
    page.ensure_router_runtime()
    const first = page.getJs().size()
    page.ensure_router_runtime()
    const second = page.getJs().size()
    if(first == 0) {
        env.error("ensure_router_runtime should emit the runtime")
        return
    }
    if(first != second) {
        env.error("ensure_router_runtime should emit exactly once")
    }
}

@test
public func test_router_runtime_has_key_symbols(env : &mut TestEnv) {
    var page = HtmlPage()
    page.ensure_router_runtime()
    var js = page.getJs()
    const version = std::string_view("window.$__uni_router_version = 1;")
    const activate = std::string_view("window.$__uni_activate_now")
    if(js.find(&version) == std::NPOS) {
        env.error("runtime should define the version marker")
    }
    if(js.find(&activate) == std::NPOS) {
        env.error("runtime should define $__uni_activate_now")
    }
    var css = page.getCss()
    const rule = std::string_view(".chx-route[data-uni-route-active=")
    if(css.find(&rule) == std::NPOS) {
        env.error("runtime should append the route hide rule")
    }
}

@test
public func test_router_runtime_absent_by_default(env : &mut TestEnv) {
    var page = HtmlPage()
    var js = page.getJs()
    const marker = std::string_view("$__uni_router_version")
    if(js.find(&marker) != std::NPOS) {
        env.error("a page that never calls ensure_router_runtime must not ship the runtime")
    }
}
