#universal TernarySsr(props) {
    return <div>{props.show ? <span>True</span> : <span>False</span>}</div>
}

@test
public func universal_ternary_ssr_true(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <TernarySsr show={true} />
    }
    
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>True</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ternary_ssr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <TernarySsr show={false} />
    }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>False</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}
