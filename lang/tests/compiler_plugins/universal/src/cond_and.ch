#universal ConditionalAnd(props) {
    return <div>{props.show && <span>Shown</span>}</div>
}

@test
public func universal_conditional_and_ssr_true(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <ConditionalAnd show={true} />
    }
    
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>Shown</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_conditional_and_ssr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <ConditionalAnd show={false} />
    }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}
