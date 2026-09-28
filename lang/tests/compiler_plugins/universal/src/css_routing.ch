// =============================================================================
// CSS/JS routing: `#css`/`#js` are always the page's own bundle; `#globalcss`
// prefers the shared assets sink (falling back to the page). Component
// `style { }` and component definitions follow the component (shared sink when
// attached). These tests pin that contract.
// =============================================================================

#universal RoutingStyledBox(props) {
    var box = style {
        color: rgb(11 22 33);
    }
    return <span class={box}>{props.children}</span>
}

func routing_emit_global_js(page : &mut HtmlPage) {
    #globaljs {
        window.$routing_global_js = 1;
    }
}

@test
public func css_macro_is_always_page_local(env : &mut TestEnv) {
    var shared = shared_assets("routing")
    var page = HtmlPage()
    page.attach_shared(shared)

    #css {
        .routing-page-marker { color: blue; }
    }

    var page_css = std::string()
    page_css.append_view(page.getCss())
    var sink_css = std::string()
    sink_css.append_view(shared.css())

    if(!page_css.contains("routing-page-marker")) {
        env.error("#css must go into the page's CSS even with a shared sink attached")
        return
    }
    if(sink_css.contains("routing-page-marker")) {
        env.error("#css must never go into the shared sink")
        return
    }
    env.success("#css is always page-local")
}

@test
public func globalcss_prefers_shared_assets(env : &mut TestEnv) {
    var shared = shared_assets("routing")
    var page = HtmlPage()
    page.attach_shared(shared)

    #globalcss {
        .routing-global-marker { color: green; }
    }

    var sink_css = std::string()
    sink_css.append_view(shared.css())
    var page_css = std::string()
    page_css.append_view(page.getCss())

    if(!sink_css.contains("routing-global-marker")) {
        env.error("#globalcss must go into the shared sink when one is attached")
        return
    }
    if(page_css.contains("routing-global-marker")) {
        env.error("#globalcss must not also go into the page when a sink is attached")
        return
    }
    env.success("#globalcss prefers the shared sink")
}

@test
public func globalcss_falls_back_to_page(env : &mut TestEnv) {
    var page = HtmlPage()

    #globalcss {
        .routing-fallback-marker { color: green; }
    }

    var page_css = std::string()
    page_css.append_view(page.getCss())
    if(!page_css.contains("routing-fallback-marker")) {
        env.error("#globalcss must fall back to the page when no sink is attached")
        return
    }
    env.success("#globalcss falls back to the page")
}

@test
public func js_macro_is_always_page_local(env : &mut TestEnv) {
    var shared = shared_assets("routing")
    var page = HtmlPage()
    page.attach_shared(shared)

    #js {
        window.$routing_page_marker = 1;
    }

    var page_js = std::string()
    page_js.append_view(page.getJs())
    var sink_js = std::string()
    sink_js.append_view(shared.js())

    if(!page_js.contains("routing_page_marker")) {
        env.error("#js must go into the page's JS even with a shared sink attached")
        return
    }
    if(sink_js.contains("routing_page_marker")) {
        env.error("#js must never go into the shared bundle")
        return
    }
    env.success("#js is always page-local")
}

@test
public func style_block_follows_component_into_shared_sink(env : &mut TestEnv) {
    var shared = shared_assets("routing")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()

    #html { <RoutingStyledBox>x</RoutingStyledBox> }

    var sink_css = std::string()
    sink_css.append_view(shared.css())
    var page_css = std::string()
    page_css.append_view(page.getCss())

    if(!sink_css.contains("rgb(11 22 33)")) {
        env.error("component style { } must go into the shared sink when one is attached")
        env.info(sink_css.data())
        return
    }
    if(page_css.contains("rgb(11 22 33)")) {
        env.error("component style { } must not also go into the page when a sink is attached")
        return
    }
    env.success("component style { } follows the component into the shared sink")
}

@test
public func globaljs_prefers_shared_assets(env : &mut TestEnv) {
    var shared = shared_assets("routing")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()

    routing_emit_global_js(&mut page)

    var sink_js = std::string()
    sink_js.append_view(shared.js())
    var page_js = std::string()
    page_js.append_view(page.getJs())

    if(!sink_js.contains("routing_global_js")) {
        env.error("#globaljs must go into the shared bundle when a sink is attached")
        return
    }
    if(page_js.contains("routing_global_js")) {
        env.error("#globaljs must not also go into the page when a sink is attached")
        return
    }
    env.success("#globaljs prefers the shared bundle")
}

@test
public func globaljs_falls_back_to_page(env : &mut TestEnv) {
    var page = HtmlPage()

    routing_emit_global_js(&mut page)

    var page_js = std::string()
    page_js.append_view(page.getJs())
    if(!page_js.contains("routing_global_js")) {
        env.error("#globaljs must fall back to the page when no sink is attached")
        return
    }
    env.success("#globaljs falls back to the page")
}

@test
public func globaljs_is_emitted_once_across_pages(env : &mut TestEnv) {
    var shared = shared_assets("routing")

    var p1 = HtmlPage()
    p1.attach_shared(shared)
    p1.defaultUniversalSetup()
    routing_emit_global_js(&mut p1)
    routing_emit_global_js(&mut p1)

    var p2 = HtmlPage()
    p2.attach_shared(shared)
    p2.defaultUniversalSetup()
    routing_emit_global_js(&mut p2)

    var sink_js = std::string()
    sink_js.append_view(shared.js())
    var sink_view = sink_js.to_view()
    const n = shared_count(&sink_view, &std::string_view("routing_global_js"))
    if(n != 1) {
        env.error("#globaljs must be emitted exactly once across pages sharing a sink")
        env.info(sink_js.data())
        return
    }
    env.success("#globaljs de-duplicated across pages")
}
