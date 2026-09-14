// Regression tests for #html macro output bugs fixed in html_cbi:
//
//   1. HTML character references in a component child's text must be decoded in
//      the emitted client vnode (SSR HTML keeps them escaped). Regression for
//      `<CodeText>&lt;CodeText&gt;</CodeText>` hydrating to the literal
//      "&lt;CodeText&gt;".
//   2. A parent universal component's hydration dispatch must precede its
//      children's dispatches, so the parent instance exists when children
//      resolve scoped context (ToggleGroup items in a top-level #html tree).
//   3. Universal components inside table structure (table/thead/tbody/tfoot/tr/
//      colgroup) must use a comment hydration boundary. A wrapper <span> there
//      is foster-parented by the HTML parser and corrupts the table.

#universal RegEntityHost(props) {
    return <code>{props.children}</code>
}

#universal RegOrderParent(props) {
    return <div>{props.children}</div>
}

#universal RegOrderChild(props) {
    return <span>child</span>
}

#universal RegTableCell(props) {
    return <th>{props.children}</th>
}

@test
public func html_entity_in_component_child_is_decoded_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <RegEntityHost>&lt;CodeText&gt;</RegEntityHost>
    }
    var js = page.getJs()
    var html = page.getHtml()
    if(js.contains("\"children\":[\"&lt;CodeText&gt;\"]")) {
        env.error("component child entity was not decoded in the emitted client vnode")
        env.info(js.data())
        return
    }
    if(!js.contains("\"children\":[\"<CodeText>\"]")) {
        env.error("expected decoded child text in the emitted client vnode")
        env.info(js.data())
        return
    }
    if(!html.contains("&lt;CodeText&gt;")) {
        env.error("SSR HTML must keep the escaped entity")
        env.info(html.data())
        return
    }
    env.success("component child entity decoded in JS and preserved in SSR HTML")
}

@test
public func html_component_parent_dispatch_precedes_child_dispatch(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <RegOrderParent><RegOrderChild /></RegOrderParent>
    }
    var js = page.getJs()
    var parentNeedle = std::string_view("'universal_lib_test_RegOrderParent'")
    var childNeedle = std::string_view("'universal_lib_test_RegOrderChild'")
    if(!js.contains(&parentNeedle) || !js.contains(&childNeedle)) {
        env.error("expected dispatches for both the parent and child component")
        env.info(js.data())
        return
    }
    var parentIdx = js.find(&parentNeedle)
    var childIdx = js.find(&childNeedle)
    if(parentIdx < childIdx) {
        env.success("parent dispatch precedes child dispatch")
    } else {
        env.error("child dispatch precedes parent dispatch")
        env.info(js.data())
    }
}

@test
public func html_table_context_uses_comment_boundary(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <table><thead><tr><RegTableCell>H</RegTableCell></tr></thead></table>
    }
    var html = page.getHtml()
    var js = page.getJs()
    if(!html.contains("<!--u")) {
        env.error("expected a comment hydration boundary inside table structure")
        env.info(html.data())
        return
    }
    if(html.contains("<tr><span")) {
        env.error("<span> wrapper inside <tr> would be foster-parented and corrupt the table")
        env.info(html.data())
        return
    }
    if(!js.contains("window.$__uni_boundary(")) {
        env.error("table-context dispatch must resolve a comment boundary")
        env.info(js.data())
        return
    }
    if(!js.contains("\"root\"")) {
        env.error("table-context component must hydrate its own root element")
        env.info(js.data())
        return
    }
    env.success("table-context component uses a comment boundary and root hydration")
}
