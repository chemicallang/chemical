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
