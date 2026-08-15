// SSR body-statement support: component-body `var` declarations, `if`
// assignments and string concatenation must be emitted into the SSR Chemical
// function so JSX attributes/children that reference the locals render. Before
// this feature, `var variant = props.variant || "default"` was dropped and
// `data-variant={variant}` silently disappeared from the SSR HTML.

#universal SsrLocalVariant(props) {
    var variant = props.variant || "default"
    var size = props.size || "md"
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    var out = classes + " " + "base-cls"
    return <span data-variant={variant} data-size={size} class={out}>{props.children}</span>
}

@test
public func universal_ssr_local_var_defaults(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrLocalVariant>Hi</SsrLocalVariant> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><span class=" base-cls" data-variant="default" data-size="md">Hi</span></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_local_var_props_and_if(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrLocalVariant variant="destructive" size="lg" className="custom" >Done</SsrLocalVariant> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><span class="custom base-cls" data-variant="destructive" data-size="lg">Done</span></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal SsrLocalTernary(props) {
    var bordered = props.bordered ? "true" : "false"
    return <span data-bordered={bordered}>{bordered}</span>
}

@test
public func universal_ssr_local_ternary(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrLocalTernary bordered={true} /> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><span data-bordered="true">true</span></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}
