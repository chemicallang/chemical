#preact Greeting(props) {
    return <span>Hello</span>
}

@test
public func preact_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_pm(document.currentScript, preact_lib_test_Greeting, {});</script>");
}

@test
public func preact_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }\n");
}

@test
public func preact_component_used_twice_in_html_emits_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting /><Greeting />
    }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }\n");
}

#preact GreetingTwice(props) {
    return <div><Greeting /><Greeting /></div>
}

@test
public func preact_component_used_twice_emits_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <GreetingTwice />
    }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }\nfunction preact_lib_test_GreetingTwice(props) { return $_p.h(\"div\", {}, $_p.h(preact_lib_test_Greeting, {}), $_p.h(preact_lib_test_Greeting, {})); }\n");
}

#preact EmptyElement(props) {
    return <span />
}

@test
public func preact_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_EmptyElement(props) { return $_p.h(\"span\", {}); }\n");
}

#preact ElementChild(props) {
    return <div><span /></div>
}

@test
public func preact_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ElementChild(props) { return $_p.h(\"div\", {}, $_p.h(\"span\", {})); }\n");
}

#preact PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func preact_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_PropsTest(props) { return $_p.h(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }\n");
}

#preact NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func preact_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_NumericProp(props) { return $_p.h(\"div\", {\"tabIndex\": 1}); }\n");
}

#preact SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func preact_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_SpreadProps(props) { return $_p.h(\"div\", {...props}); }\n");
}

#preact FragmentTest(props) {
    return <><span/></>
}

@test
public func preact_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_FragmentTest(props) { return $_p.h($_p.Fragment, null, $_p.h(\"span\", {})); }\n");
}

#preact ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func preact_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }\nfunction preact_lib_test_ComponentChild(props) { return $_p.h(\"div\", {}, $_p.h(preact_lib_test_Greeting, {})); }\n");
}

#preact ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func preact_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }\nfunction preact_lib_test_ComponentProps(props) { return $_p.h(preact_lib_test_Greeting, {\"text\": \"hi\"}); }\n");
}

#preact TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func preact_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_TernaryTest(cond) { return $_p.h(\"div\", {}, cond ? $_p.h(\"span\", {}, ` a `) : $_p.h(\"span\", {}, ` b `)); }\n");
}

#preact DollarIdentifierTest(props) {
    return <div>{window.$flag ? window.$title : window.$fallback}</div>
}

@test
public func preact_dollar_identifier_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DollarIdentifierTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_DollarIdentifierTest(props) { return $_p.h(\"div\", {}, window.$flag ? window.$title : window.$fallback); }\n");
}

#preact StrictTypeofCheck(store) {
    return <div>{!store || typeof store !== "object" ? "bad" : "ok"}</div>
}

@test
public func preact_strict_typeof_check_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StrictTypeofCheck /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_StrictTypeofCheck(store) { return $_p.h(\"div\", {}, !store || typeof store !== \"object\" ? \"bad\" : \"ok\"); }\n");
}

#preact ParenthesizedTernaryLabel(item) {
    return <div>{(item.checked ? "[x] " : "[ ] ") + item.text}</div>
}

@test
public func preact_parenthesized_ternary_with_member_access_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ParenthesizedTernaryLabel /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ParenthesizedTernaryLabel(item) { return $_p.h(\"div\", {}, (item.checked ? \"[x] \" : \"[ ] \") + item.text); }\n");
}

#preact MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func preact_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_MapTest(items) { return $_p.h(\"ul\", {}, items.map((i) => $_p.h(\"li\", {}, i))); }\n");
}

#preact EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func preact_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_EventTest(props) { return $_p.h(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }\n");
}

#preact BacktickText(props) {
    return <p>The code is `sync.status` and `lastSyncedAt`.</p>
}

@test
public func preact_backtick_in_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BacktickText /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_BacktickText(props) { return $_p.h(\"p\", {}, ` The code is \\`sync.status\\` and \\`lastSyncedAt\\`. `); }\n");
}
