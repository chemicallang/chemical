using namespace std;

func univ_view_equals(env : &mut TestEnv, got : &std::string_view, expected : &std::string_view) {
    if(got.equals(expected)) {
        return;
    }
    env.error("universal parse mismatch");
    var expected_s = std::string("expected:\"");
    expected_s.append_view(expected)
    expected_s.append('"');
    env.info(expected_s.data())

    var got_s = std::string("got     :\"");
    got_s.append_view(got)
    got_s.append('"');
    env.info(got_s.data())
}

@test
public func test_universal_parse_basic_jsx(env : &mut TestEnv) {
    const view = std::string_view("<div>hello</div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div>hello</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_attributes(env : &mut TestEnv) {
    const view = std::string_view("<div className=\"box\" onClick={() => count += 1}>Hi {name}</div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div className=\"box\" onClick={() => count += 1}>Hi {name}</div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_fragment(env : &mut TestEnv) {
    const view = std::string_view("<>fragment</>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<>fragment</>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_self_closing_component(env : &mut TestEnv) {
    const view = std::string_view("<MyComp prop={value} />")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<MyComp prop={value} />;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_component_body(env : &mut TestEnv) {
    const view = std::string_view("var count = 0; function inc() { count += 1 } return <button onClick={inc}>{count}</button>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("var count = 0;function inc(){count += 1;}return <button onClick={inc}>{count}</button>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_nested_jsx(env : &mut TestEnv) {
    const view = std::string_view("<div><span>a</span><span>b</span></div>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<div><span>a</span><span>b</span></div>;")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_universal_parse_expression_container(env : &mut TestEnv) {
    const view = std::string_view("<p>{items.map(x => x.name)}</p>")
    var out = universal::parse_universal(view)
    var expected = std::string_view("<p>{items.map((x) => x.name)}</p>;")
    univ_view_equals(env, out.to_view(), &expected)
}

// ── Universal router declarations (Phase 0) ─────────────────────────────────

@test
public func test_router_parse_id_route(env : &mut TestEnv) {
    const view = std::string_view("router \"m\" { route #\"a\" { <A/> } }")
    var out = universal::router_decl_summary(view)
    var expected = std::string_view("router:m;route:#a|id=a|url=0|def=0|fb=0|mode=|title=|hooks=0")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_router_parse_default_url_fallback(env : &mut TestEnv) {
    const view = std::string_view("router \"m\" { route default #\"dash\" { <D/> } route \"/p/{id}\" preload title \"P\" { <P/> } route * { <N/> } }")
    var out = universal::router_decl_summary(view)
    var expected = std::string_view("router:m;route:#dash|id=dash|url=0|def=1|fb=0|mode=|title=|hooks=0;route:/p/{id}|id=/p/{id}|url=1|def=0|fb=0|mode=preload|title=P|hooks=0;route:*|id=*|url=0|def=0|fb=1|mode=|title=|hooks=0")
    univ_view_equals(env, out.to_view(), &expected)
}

@test
public func test_router_parse_hooks(env : &mut TestEnv) {
    const view = std::string_view("router \"m\" { route #\"a\" { onActivate(() => { x += 1 }) onDeactivate(() => { y -= 1 }) <A/> } }")
    var out = universal::router_decl_summary(view)
    var expected = std::string_view("router:m;route:#a|id=a|url=0|def=0|fb=0|mode=|title=|hooks=1")
    univ_view_equals(env, out.to_view(), &expected)
}

