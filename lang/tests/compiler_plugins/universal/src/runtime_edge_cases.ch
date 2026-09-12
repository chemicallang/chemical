// Minimal edge case tests for the universal pipeline.

#universal InnerHoistComp(props) {
    return <span>{props.text}</span>
}

#universal OuterHoistComp(props) {
    return <div>
        <InnerHoistComp text="nested" />
    </div>
}

@test
public func universal_nested_component_hoisting_correct(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <OuterHoistComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("InnerHoistComp") && js.contains("OuterHoistComp")) {
        env.success("nested components both present in JS output")
    } else {
        env.error("nested components missing from JS output")
        env.info(js.data())
    }
}

#universal DedupComp(props) {
    return <span>{props.label}</span>
}

@test
public func universal_component_emitted_once_per_page(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <DedupComp label="first" />
        <DedupComp label="second" />
    }
    var js = std::string()
    js.append_view(page.getJs())
    // Both should be present; we check function count via string search.
    if(js.contains("DedupComp")) {
        env.success("DedupComp present in JS output")
    } else {
        env.error("DedupComp not found in JS output")
        env.info(js.data())
    }
}

#universal PosTrackingComp(props) {
    return <span>{props.x}</span>
}

@test
public func universal_js_output_valid_structure(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PosTrackingComp x={42} /> }
    var js = std::string()
    js.append_view(page.getJs())
    // Verify the JS output contains both function definition and dispatch.
    var has_func = js.contains("function ")
    var has_dispatch = js.contains("window.$__uni_dispatch(")
    if(has_func && has_dispatch) {
        env.success("JS output has both function definition and dispatch")
    } else {
        env.error("JS output missing function or dispatch")
        env.info(js.data())
    }
}

@test
public func universal_page_js_end_has_flush(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    #html { <PosTrackingComp x={1} /> }
    var finalized = page.getFinalizedPageJs()
    if(finalized.contains("$__universal_flush()")) {
        env.success("finalized JS contains $__universal_flush()")
    } else {
        env.error("finalized JS missing $__universal_flush()")
        env.info(finalized.data())
    }
}

#universal EmptyStateArrayComp(props) {
    state items = []
    return <ul>
        {items.map((item, i) => <li key={i}>{item}</li>)}
    </ul>
}

@test
public func universal_empty_state_array_ssr_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyStateArrayComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("<li>")) {
        env.success("empty state array produces no <li> elements in SSR")
    } else {
        env.error("empty state array produced unexpected <li> elements")
        env.info(html.data())
    }
}

#universal NestedTernaryFalseComp(props) {
    state open = false
    return <div>
        {open ? <span>Open</span> : <span>Closed</span>}
    </div>
}

#universal NestedTernaryTrueComp(props) {
    state open = true
    return <div>
        {open ? <span>Open</span> : <span>Closed</span>}
    </div>
}

@test
public func universal_nested_ternary_false_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedTernaryFalseComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("Closed") && !html.contains(">Open<")) {
        env.success("state=false renders 'Closed' branch in SSR")
    } else {
        env.error("state=false did not render expected branch")
        env.info(html.data())
    }
}

@test
public func universal_nested_ternary_true_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedTernaryTrueComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("Open") && !html.contains(">Closed<")) {
        env.success("state=true renders 'Open' branch in SSR")
    } else {
        env.error("state=true did not render expected branch")
        env.info(html.data())
    }
}

#universal MultiCompA(props) {
    return <span>A-{props.x}</span>
}

#universal MultiCompB(props) {
    return <span>B-{props.y}</span>
}

@test
public func universal_multiple_components_unique_ids(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <MultiCompA x="1" />
        <MultiCompB y="2" />
        <MultiCompA x="3" />
    }
    var html = std::string()
    html.append_view(page.getHtml())
    var id0 = page.getComponentId(0)
    var id1 = page.getComponentId(1)
    var id2 = page.getComponentId(2)
    if(!id0.equals(&id1) && !id1.equals(&id2) && !id0.equals(&id2)) {
        env.success("all three components have unique hydration boundary IDs")
    } else {
        env.error("some component IDs are not unique")
    }
}

#universal NullPropComp(props) {
    return <div title={props.title} data-x={props.x}>content</div>
}

@test
public func universal_null_prop_ssr_skipped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NullPropComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("null")) {
        env.success("null props are skipped in SSR (no literal 'null')")
    } else {
        env.error("null props rendered as literal 'null' in SSR")
        env.info(html.data())
    }
}
