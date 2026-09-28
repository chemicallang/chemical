// =============================================================================
// `style { … }` — component-scoped CSS blocks inside #universal components.
//
// The block is parsed by the CSS parser, returns the generated (deterministic)
// class name as a compile-time constant, and emits the CSS through the same
// path as `#css`/`#styled` (so it follows the component: shared sink when
// attached, otherwise the page).
// =============================================================================

#universal StyleBlockBadge(props) {
    var badge = style {
        color: red;
        display: inline-flex;
        &:hover { color: blue; }
    }
    return <span class={badge}>{props.children}</span>
}

#universal StyleBlockPair(props) {
    var a = style { color: red; }
    var b = style { display: block; }
    return <span class={a + " " + b}>x</span>
}

@test
public func style_block_emits_css_and_returns_class(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    #html { <StyleBlockBadge>hi</StyleBlockBadge> }

    var css = std::string()
    css.append_view(page.getCss())
    if(!css.contains("color:red")) {
        env.error("style block declarations missing from page CSS")
        env.info(css.data())
        return
    }
    if(!css.contains("hover") || !css.contains("blue")) {
        env.error("nested selector in a style block was not emitted")
        env.info(css.data())
        return
    }
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("class=\"")) {
        env.error("the class returned by style { } was not applied")
        env.info(html.data())
        return
    }
    env.success("style block emits scoped CSS and returns a usable class")
}

@test
public func style_block_is_deterministic_and_deduplicated(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    #html {
        <StyleBlockBadge>a</StyleBlockBadge>
        <StyleBlockBadge>b</StyleBlockBadge>
    }

    var css = std::string()
    css.append_view(page.getCss())
    var view = css.to_view()
    // Two instances of the same component must emit its rule exactly once.
    if(shared_count(&view, &std::string_view("color:red")) != 1) {
        env.error("style block rule must be emitted once even for repeated instances")
        env.info(css.data())
        return
    }
    env.success("style block rule is deterministic and emitted once")
}

@test
public func style_block_supports_multiple_blocks_per_component(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    #html { <StyleBlockPair /> }

    var css = std::string()
    css.append_view(page.getCss())
    if(!css.contains("color:red") || !css.contains("display:block")) {
        env.error("each style block must emit its own class/rules")
        env.info(css.data())
        return
    }
    env.success("multiple style blocks per component are supported")
}
