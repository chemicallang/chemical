#universal LogicalOrSsr(props) {
    return <div>{props.hide || <span>Shown by Default</span>}</div>
}

@test
public func universal_logical_or_ssr_true(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LogicalOrSsr hide={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_logical_or_ssr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LogicalOrSsr hide={false} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>Shown by Default</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal NestedConditionalSsr(props) {
    return <div>{props.show && (props.other ? <span>Both</span> : <span>Only Show</span>)}</div>
}

@test
public func universal_nested_conditional_ssr_both(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedConditionalSsr show={true} other={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>Both</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal FragmentConditionalSsr(props) {
    return <div>{props.show && <><p>1</p><p>2</p></>}</div>
}

@test
public func universal_fragment_conditional_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentConditionalSsr show={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><p>1</p><p>2</p></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal LogicalNotSsr(props) {
    return <div>{!props.show && <span>Hidden is False</span>}</div>
}

@test
public func universal_logical_not_ssr_false(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LogicalNotSsr show={false} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><div><span>Hidden is False</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}
