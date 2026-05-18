#universal CondStateDetail(props) {
    state detailUser = null;
    return <div>{detailUser ? <span>{detailUser.name}</span> : <span>No user</span>}</div>
}

@test
public func universal_cond_state_detail_ssr_no_user(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetail /> }
    // SSR cannot evaluate state variable conditions; both branches are skipped
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_cond_state_detail_js_no_user(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetail /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_CondStateDetail(props) { const detailUser = $_us(null); return $_ur.createElement("div", {}, $_ucs(() => (detailUser.value ? $_ur.createElement("span", {}, $_ucs(() => detailUser.value.name)) : $_ur.createElement("span", {}, ${"` No user `"})))); }\nwindow.$__uni_dispatch('universal_lib_test_CondStateDetail', document.getElementById('u${page.getComponentId(0)}'), {});\n`)
    view_equals(env, page.getJs(), str.to_view())
}

#universal CondStateDetailWithProp(props) {
    state detail = null;
    return <div>{detail ? <span>{props.prefix}{detail.name}</span> : <span>Empty</span>}</div>
}

@test
public func universal_cond_state_detail_prop_ssr_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondStateDetailWithProp prefix="Name: " /> }
    // SSR cannot evaluate state variable conditions; both branches are skipped
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}
