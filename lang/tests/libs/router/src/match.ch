using namespace std;

// ---------------------------------------------------------------------------
// Universal router URL matching tests (Phase 5, §15.4)
// ---------------------------------------------------------------------------

func make_pattern(pattern : std::string_view, id : std::string_view, is_fallback : bool = false) : RoutePattern {
    return RoutePattern {
        segments : pattern_segments(pattern),
        id : id,
        is_fallback : is_fallback,
        prefix : false,
        chain : std::vector<RouteChainStep>()
    }
}

@test
public func test_router_match_literal(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/settings", "settings"))
    const m = match_route(&mut patterns, "/settings", "")
    if(!m.matched) { env.error("literal route should match"); return }
    if(!m.id.equals(std::string_view("settings"))) { env.error("wrong id") }
    if(m.params.size() != 0) { env.error("literal route should have no params") }
}

@test
public func test_router_match_params(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    const m = match_route(&mut patterns, "/projects/42", "")
    if(!m.matched) { env.error("param route should match"); return }
    if(!m.id.equals(std::string_view("projects"))) { env.error("wrong id"); return }
    if(m.params.size() != 1) { env.error("expected one param"); return }
    const p = m.params.get_ptr(0)
    var Param(name, value) = *p else unreachable
    if(!name.equals(std::string_view("id"))) { env.error("param name") }
    if(!value.equals(std::string_view("42"))) { env.error("param value") }
}

@test
public func test_router_match_miss_fallback(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/settings", "settings"))
    patterns.push(make_pattern("/", "not-found", true))
    var m = match_route(&mut patterns, "/nope", "")
    if(!m.matched) { env.error("fallback should match a miss"); return }
    if(!m.is_fallback) { env.error("fallback flag") }
    if(!m.id.equals(std::string_view("not-found"))) { env.error("fallback id") }
}

@test
public func test_router_match_miss_no_fallback(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/settings", "settings"))
    var m = match_route(&mut patterns, "/nope", "")
    if(m.matched) { env.error("no fallback: should not match") }
}

@test
public func test_router_match_first_wins(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/a/{x}", "first"))
    patterns.push(make_pattern("/a/{y}", "second"))
    const m = match_route(&mut patterns, "/a/1", "")
    if(!m.matched) { env.error("should match") ; return }
    if(!m.id.equals(std::string_view("first"))) { env.error("first-match scan should win") }
}

@test
public func test_router_match_base_and_trailing_slash(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects", "projects"))
    var m = match_route(&mut patterns, "/app/projects/", "/app")
    if(!m.matched) { env.error("base + trailing slash should match"); return }
    if(!m.id.equals(std::string_view("projects"))) { env.error("wrong id") }
}

@test
public func test_router_match_ignores_query(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/search", "search"))
    const m = match_route(&mut patterns, "/search?q=1", "")
    if(!m.matched) { env.error("query should be ignored while matching") }
}

@test
public func test_router_build_path_roundtrip(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    var params = std::unordered_map<std::string_view, std::string_view>()
    params.insert(std::string_view("id"), std::string_view("a b/c"))
    var path = build_path(&mut patterns, "projects", &mut params)
    var expected = std::string_view("/projects/a%20b%2Fc")
    if(!path.to_view().equals(&expected)) {
        env.error("build_path should percent-encode params")
        env.info(path.data())
    }
}

@test
public func test_router_build_path_missing_param(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    var params = std::unordered_map<std::string_view, std::string_view>()
    var path = build_path(&mut patterns, "projects", &mut params)
    var expected = std::string_view("/projects/")
    if(!path.to_view().equals(&expected)) {
        env.error("missing param should build an empty segment")
        env.info(path.data())
    }
}

@test
public func test_router_normalize_path(env : &mut TestEnv) {
    var a = normalize_path("/projects/", "")
    var expected = std::string_view("/projects")
    if(!a.to_view().equals(&expected)) { env.error("trailing slash should be stripped") }
    var b = normalize_path("/app/projects", "/app")
    var e2 = std::string_view("/projects")
    if(!b.to_view().equals(&e2)) { env.error("base should be stripped") }
    var c = normalize_path("/", "")
    var e3 = std::string_view("/")
    if(!c.to_view().equals(&e3)) { env.error("root should stay root") }
}

@test
public func test_router_match_carries_nested_chain(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    var chain = std::vector<RouteChainStep>()
    chain.push(RouteChainStep { reg : std::string_view("main#/a/{x}"), id : std::string_view("/settings") })
    patterns.push(RoutePattern {
        segments : pattern_segments("/a/{x}/settings"),
        id : std::string_view("/a/{x}"),
        is_fallback : false,
        prefix : false,
        chain : chain
    })
    const m = match_route(&mut patterns, "/a/1/settings", "")
    if(!m.matched) { env.error("nested full pattern should match"); return }
    if(!m.id.equals(std::string_view("/a/{x}"))) { env.error("the entry id is the root route id") }
    if(m.params.size() != 1) { env.error("a full nested pattern captures ancestor params") }
    if(m.chain.size() != 1) { env.error("the chain should carry one nested step"); return }
    const s = m.chain.get_ptr(0)
    if(!s.reg.equals(std::string_view("main#/a/{x}"))) { env.error("chain registry") }
    if(!s.id.equals(std::string_view("/settings"))) { env.error("chain route id") }
    // A top-level entry carries no chain.
    var top = std::vector<RoutePattern>()
    top.push(make_pattern("/a/{x}", "/a/{x}"))
    const mt = match_route(&mut top, "/a/1", "")
    if(mt.chain.size() != 0) { env.error("a top-level match must have an empty chain") }
}

@test
public func test_router_match_rejects_dot_segments(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    var m = match_route(&mut patterns, "/projects/..", "")
    if(m.matched) { env.error("a `..` segment must not be captured by a param") }
    var m2 = match_route(&mut patterns, "/projects/.", "")
    if(m2.matched) { env.error("a `.` segment must not be captured by a param") }
    // Literal matching is unaffected (no regression).
    var lit = std::vector<RoutePattern>()
    lit.push(make_pattern("/projects/list", "list"))
    var m3 = match_route(&mut lit, "/projects/list", "")
    if(!m3.matched) { env.error("literal matching must be unaffected") }
}

@test
public func test_router_match_rejects_percent_encoded_dot_segments(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    var m = match_route(&mut patterns, "/projects/%2E%2E", "")
    if(m.matched) { env.error("percent-encoded `..` must not be captured by a param") }
    var m2 = match_route(&mut patterns, "/projects/%2e", "")
    if(m2.matched) { env.error("percent-encoded `.` must not be captured by a param") }
    var m3 = match_route(&mut patterns, "/projects/%2E.", "")
    if(m3.matched) { env.error("a mixed encoded/literal `..` must not be captured") }
    // A normal percent-encoded param is still captured (no regression).
    var m4 = match_route(&mut patterns, "/projects/a%2Eb", "")
    if(!m4.matched) { env.error("a normal percent-encoded param must still match") }
}

@test
public func test_router_build_path_fallback_returns_root(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/projects/{id}", "projects"))
    patterns.push(make_pattern("/", "*", true))
    var params = std::unordered_map<std::string_view, std::string_view>()
    var path = build_path(&mut patterns, "*", &mut params)
    var expected = std::string_view("/")
    if(!path.to_view().equals(&expected)) {
        // Documented client/server split: the client `buildPath` skips fallback
        // entries (returns `null`); the server reverses the fallback to root.
        env.error("server build_path for the fallback id should return '/'")
        env.info(path.data())
    }
}

@test
public func test_router_match_prefix_fallback(env : &mut TestEnv) {
    var patterns = std::vector<RoutePattern>()
    patterns.push(make_pattern("/a/{x}", "/a/{x}"))
    var chain = std::vector<RouteChainStep>()
    chain.push(RouteChainStep { reg : std::string_view("main#/a/{x}"), id : std::string_view("*") })
    patterns.push(RoutePattern {
        segments : pattern_segments("/a/{x}"),
        id : std::string_view("/a/{x}"),
        is_fallback : false,
        prefix : true,
        chain : chain
    })
    // An exact entry on the same pattern wins for the bare path.
    const me = match_route(&mut patterns, "/a/1", "")
    if(!me.matched || me.chain.size() != 0) {
        env.error("an exact match must win over a prefix entry")
    }
    // The prefix entry catches a longer, otherwise-unmatched path.
    const mp = match_route(&mut patterns, "/a/1/unknown", "")
    if(!mp.matched) { env.error("a prefix entry should catch a longer path"); return }
    if(!mp.id.equals(std::string_view("/a/{x}"))) { env.error("prefix entry id") }
    if(mp.params.size() != 1) { env.error("prefix entry should capture the prefix params") }
    if(mp.chain.size() != 1) { env.error("the prefix entry should carry its chain") }
}
