#universal CondStateDetail(props) {
    state detailUser = null;
    return <div>{detailUser ? <span>{detailUser.name}</span> : <span>No user</span>}</div>
}

@test
public func universal_cond_state_detail_ssr_no_user(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetail /> }
    // State initializers are SSR-evaluated (null → falsy), so the else branch
    // renders — matching the client's first render.
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}"><div><span>No user</span></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_cond_state_detail_js_no_user(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetail /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_CondStateDetail(props) { const detailUser = $_us(null); return $_ur.createElement("div", {}, $_ucs(() => (detailUser.value ? $_ur.createElement("span", {}, $_ucs(() => detailUser.value.name)) : $_ur.createElement("span", {}, ${"`No user`"})))); }\nwindow.$__uni_dispatch('universal_lib_test_CondStateDetail', document.getElementById('u${page.getComponentId(0)}'), {});\n`)
    view_equals(env, page.getJs(), str.to_view())
}

#universal CondStateDetailWithProp(props) {
    state detail = null;
    return <div>{detail ? <span>{props.prefix}{detail.name}</span> : <span>Empty</span>}</div>
}

// Mixed-precedence condition: `a !== undefined || b !== undefined` must parse
// as ||(!==(a,undefined), !==(b,undefined)) — the precedence-climbing parser
// fix (previously the tree was !==(||(...), undefined) and SSR dropped the
// whole branch because the mis-nested comparison couldn't convert).
#universal PrecedenceOrCond(props) {
    if(props.a !== undefined || props.b !== undefined) {
        return <div>Has</div>
    }
    return <div>None</div>
}

@test
public func universal_precedence_or_cond_ssr_has(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PrecedenceOrCond a="x" /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}"><div>Has</div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_precedence_or_cond_ssr_none(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PrecedenceOrCond /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}"><div>None</div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_precedence_or_cond_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PrecedenceOrCond a="x" /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_PrecedenceOrCond(props) { if(window.$__uni_value(props.a) !== undefined || window.$__uni_value(props.b) !== undefined) { return $_ur.createElement("div", {}, ${"`Has`"}); } return $_ur.createElement("div", {}, ${"`None`"}); }\nwindow.$__uni_dispatch('universal_lib_test_PrecedenceOrCond', document.getElementById('u${page.getComponentId(0)}'), ${"{\"a\":\"x\"}"});\n`)
    view_equals(env, page.getJs(), str.to_view())
}

// Arithmetic precedence: `1 + 2 * 3` evaluates as 1 + (2*3) = 7 (JS rules),
// not (1+2)*3 = 9.
#universal PrecedenceArith(props) {
    return <div>{1 + 2 * 3}</div>
}

@test
public func universal_precedence_arith_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PrecedenceArith /> }
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}"><div>7</div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_cond_state_detail_prop_ssr_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetailWithProp prefix="Name: " /> }
    // State initializers are SSR-evaluated (null → falsy), so the else branch
    // renders — matching the client's first render.
    var html = std::string()
    html.append_expr(`<span id="u${page.getComponentId(0)}"><div><span>Empty</span></div></span>`)
    view_equals(env, page.getHtml(), html.to_view())
}
