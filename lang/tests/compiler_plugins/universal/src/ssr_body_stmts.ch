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

// ---------------------------------------------------------------------------
// Conditional returns: `if(tag == "span") { return <span/> } return <p/>` must
// dispatch to exactly one branch in SSR (JS `return` semantics).
// ---------------------------------------------------------------------------

#universal SsrCondTag(props) {
    var tag = props.as || "p"
    if(tag == "span") { return <span class="tag-span">{props.children}</span> }
    if(tag == "div") { return <div class="tag-div">{props.children}</div> }
    return <p class="tag-p">{props.children}</p>
}

@test
public func universal_ssr_conditional_return_dispatch(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrCondTag as="span">Hi</SsrCondTag> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><span class="tag-span">Hi</span></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_conditional_return_default(env : &mut TestEnv) {
    // Unset `as` falls through to the default <p> branch — exactly one branch.
    var page = HtmlPage()
    #html { <SsrCondTag>Plain</SsrCondTag> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><p class="tag-p">Plain</p></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// Numeric props (Integer variant from the caller) compared against literals
// must match like JS loose equality (`props.level == 2` with level=2).
#universal SsrNumericLevel(props) {
    var level = props.level || 1
    if(level == 2) { return <h2>{props.children}</h2> }
    if(level == 3) { return <h3>{props.children}</h3> }
    return <h1>{props.children}</h1>
}

@test
public func universal_ssr_numeric_prop_equals(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrNumericLevel level={2}>T</SsrNumericLevel> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><h2>T</h2></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_numeric_prop_default(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrNumericLevel>One</SsrNumericLevel> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><h1>One</h1></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

// ---------------------------------------------------------------------------
// ${...} embeds inside JSX concat expressions: the JS bundle must contain the
// string content as a quoted literal, NOT a bare mangled identifier (which
// would be a ReferenceError in the browser).
// ---------------------------------------------------------------------------

func ssr_concat_style(page : &mut HtmlPage) : *char {
    return #css {
        color: rgb(1, 2, 3);
    }
}

#universal SsrConcatEmbed(props) {
    var classes = props.class || ""
    return <span class={classes + " " + ${ssr_concat_style(page)}}>{props.children}</span>
}

@test
public func universal_ssr_concat_embed_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrConcatEmbed class="extra">Hi</SsrConcatEmbed> }
    var html = std::string()
    html.append_expr(`<div id="u${page.getComponentId(0)}"><span class="extra hD7pDyR">Hi</span></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

@test
public func universal_ssr_concat_embed_js_quoted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SsrConcatEmbed>Hi</SsrConcatEmbed> }
    var js = std::string()
    js.append_view(page.getJs())
    // The client bundle must embed the CSS hash as a quoted string literal.
    // A bare mangled identifier here would throw a ReferenceError on hydrate.
    var needle = std::string_view("\"class\": classes + \" \" + \"")
    if(js.contains(&needle)) {
        return;
    }
    env.error("JS bundle must embed the CSS hash as a quoted string literal");
    env.info(js.data())
}
