// Negative tests for the universal router diagnostics (design §14.8, G-3/G-4).
//
// Every router misuse must be a compiler diagnostic with a source location —
// never an assertion failure or a silent drop.

@test
public func neg_router_orphan_route(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    route #\"x\" { <A /> }\n    return <div><A /></div>\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_orphan_route", ch,
        "'route' declaration is only valid inside a router block", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_duplicate_id(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { <A /> }\n        route #\"a\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_duplicate_id", ch,
        "route '#a' is declared twice in router \"m\"", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_duplicate_name(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { <A /> }\n    }\n    router \"m\" {\n        route #\"b\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_duplicate_name", ch,
        "router \"m\" is declared twice", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_fallback_not_last(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route * { <A /> }\n        route #\"a\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_fallback_not_last", ch,
        "fallback route must be the last route", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_body_no_root(env : &mut TestEnv) {
    var ch = "#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_body_no_root", ch,
        "route body must render exactly one root element", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_unsupported_pattern(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route \"/a/*\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_unsupported_pattern", ch,
        "unsupported route pattern '/a/*'", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_runtime_internals(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { <div onClick={() => { window.$__uni_route_visible(1) }}>x</div> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_runtime_internals", ch,
        "route bodies cannot call runtime internals", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_unknown_activate_id(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" {\n            onActivate(() => { router(\"m\").activateRoute(\"typo\") })\n            <A />\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_unknown_activate_id", ch,
        "no route 'typo' in router \"m\"", NEG_MOD_UNIVERSAL)
}

// Nested routers (a route body's nested `route` children) are validated with the
// same rules under their derived name `parent#routeId`.

@test
public func neg_router_nested_duplicate_id(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" {\n            route #\"x\" { <A /> }\n            route #\"x\" { <A /> }\n            <div>x</div>\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_nested_duplicate_id", ch,
        "route '#x' is declared twice in router \"m#a\"", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_nested_body_no_root(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" {\n            route #\"x\" { }\n            <div>x</div>\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_nested_body_no_root", ch,
        "route body must render exactly one root element", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_nested_fallback_not_last(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" {\n            route * { <A /> }\n            route #\"x\" { <A /> }\n            <div>x</div>\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_nested_fallback_not_last", ch,
        "fallback route must be the last route", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_nested_unknown_activate_id(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" {\n            route #\"x\" {\n                onActivate(() => { router(\"m#a\").activateRoute(\"typo\") })\n                <A />\n            }\n            <div>x</div>\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_nested_unknown_activate_id", ch,
        "no route 'typo' in router \"m#a\"", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_ambiguous_patterns(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route \"/a/{x}\" { <A /> }\n        route \"/a/{y}\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_ambiguous_patterns", ch,
        "route patterns '/a/{x}' and '/a/{y}' are ambiguous", NEG_MOD_UNIVERSAL)
}

@test
public func neg_router_ambiguous_nested_patterns(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route \"/a/{x}\" {\n            route \"/b/{p}\" { <A /> }\n            route \"/b/{q}\" { <A /> }\n            <div>x</div>\n        }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "router_ambiguous_nested_patterns", ch,
        "are ambiguous", NEG_MOD_UNIVERSAL)
}

@test
public func warn_router_no_default(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { <A /> }\n        route #\"b\" { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_output_contains(env, "router_no_default", ch,
        "router \"m\" has no default route; the page renders inert without a server parameter", NEG_MOD_UNIVERSAL)
}

@test
public func warn_router_no_default_absent_when_fallback(env : &mut TestEnv) {
    var ch = "#universal A(props) {\n    return <span>x</span>\n}\n#universal Host(props) {\n    router \"m\" {\n        route #\"a\" { <A /> }\n        route * { <A /> }\n    }\n}\npublic func main() : int {\n    return 0\n}\n"
    // The compiler still succeeds, and the R10 warning must not be present.
    expect_compile_output_not_contains(env, "router_no_default_absent", ch,
        "has no default route", NEG_MOD_UNIVERSAL)
}
