// SSR attribute expression evaluation tests.
// Props-driven expressions in attributes (ternary, ||, &&, comparisons, concat)
// must be evaluated at SSR time so HTML matches the JS client output.

#universal SsrExprButton(props) {
    return <button class={props.variant === "primary" ? "chx-btn chx-btn-primary" : "chx-btn chx-btn-default"} type="button">{props.children}</button>
}

@test
public func universal_ssr_expr_ternary_primary(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="primary" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn chx-btn-primary" type="button"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_ternary_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="outline" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn chx-btn-default" type="button"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprOrButton(props) {
    return <button class={props.primary ? "chx-btn-primary" : "chx-btn-default"}>{props.children}</button>
}

@test
public func universal_ssr_expr_or_truthy(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprOrButton primary={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn-primary"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_or_falsy(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprOrButton primary={false} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn-default"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprAndButton(props) {
    return <button class={props.active && "chx-btn-active"}>{props.children}</button>
}

@test
public func universal_ssr_expr_and_active(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprAndButton active={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn-active"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_and_inactive(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprAndButton active={false} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprCompareButton(props) {
    return <button disabled={props.variant === "primary"}>{props.children}</button>
}

@test
public func universal_ssr_expr_compare_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprCompareButton variant="primary" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button disabled="true"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_expr_compare_attr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprCompareButton variant="secondary" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprConcatButton(props) {
    return <button class={"chx-btn chx-btn-" + props.variant + " chx-btn-" + props.size}>{props.children}</button>
}

@test
public func universal_ssr_expr_concat(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprConcatButton variant="primary" size="sm" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><button class="chx-btn chx-btn-primary chx-btn-sm"></button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrExprClassName(props) {
    return <div class="chx-card" class={props.className}>{props.children}</div>
}

@test
public func universal_ssr_expr_dual_class(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprClassName className="my-card" /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div class="chx-card my-card"></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// JS-side parity: the client bundle must contain the same expressions, valid JS.
@test
public func universal_ssr_expr_js_parity(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprButton variant="primary" /> }
    var js = page.getJs()
    if(js.contains("chx-btn-primary")) {
        env.success("ternary class expression present in JS bundle")
    } else {
        env.error("ternary class expression missing from JS bundle")
        env.info(js.data())
    }
}

// Duplicate class attributes must be merged on the JS side (space-joined, with
// reactive unwrapping) to match the SSR HTML's class merging.
@test
public func universal_ssr_expr_js_class_merge(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprClassName className="my-card" /> }
    var js = page.getJs()
    var merged = js.contains("$__uni_value(w))") && js.contains(".filter(Boolean).join")
    var noDupKeys = !js.contains("\"class\": \"chx-card\", \"class\"")
    if(merged && noDupKeys) {
        env.success("duplicate class attrs merged reactively on JS side")
    } else {
        env.error("duplicate class attrs not merged on JS side")
        env.info(js.data())
    }
}

// A spread + duplicate class combination must still merge through $_um.
#universal SsrExprSpreadClass(props) {
    return <button {...props} class="chx-btn" class={props.className}>x</button>
}

@test
public func universal_ssr_expr_spread_class_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrExprSpreadClass className="extra" variant="primary" /> }
    var js = page.getJs()
    var html = page.getHtml()
    if(js.contains("$_um") && html.contains("chx-btn extra")) {
        env.success("spread + class merge works")
    } else {
        env.error("spread + class merge broken")
        env.info(html.data())
        env.info(js.data())
    }
}
