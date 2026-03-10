#react Greeting(props) {
    return <span>Hello</span>
}

@test
public func react_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_rm(document.currentScript, react_lib_test_Greeting, {});</script>");
}

@test
public func react_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }");
}

#react EmptyElement(props) {
    return <span />
}

@test
public func react_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_EmptyElement(props) { return $_r.createElement(\"span\", {}); }");
}

#react ElementChild(props) {
    return <div><span /></div>
}

@test
public func react_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ElementChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(\"span\", {})); }");
}

#react PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func react_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_PropsTest(props) { return $_r.createElement(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }");
}

#react NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func react_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_NumericProp(props) { return $_r.createElement(\"div\", {\"tabIndex\": 1}); }");
}

#react SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func react_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_SpreadProps(props) { return $_r.createElement(\"div\", {...props}); }");
}

#react FragmentTest(props) {
    return <><span/></>
}

@test
public func react_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_FragmentTest(props) { return $_r.createElement($_r.Fragment, null, $_r.createElement(\"span\", {})); }");
}

#react ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func react_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }function react_lib_test_ComponentChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(react_lib_test_Greeting, {})); }");
}

#react ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func react_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }function react_lib_test_ComponentProps(props) { return $_r.createElement(react_lib_test_Greeting, {\"text\": \"hi\"}); }");
}

#react TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func react_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_TernaryTest(cond) { return $_r.createElement(\"div\", {}, cond ? $_r.createElement(\"span\", {}, ` a `) : $_r.createElement(\"span\", {}, ` b `)); }");
}

#react MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func react_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_MapTest(items) { return $_r.createElement(\"ul\", {}, items.map((i) => $_r.createElement(\"li\", {}, i))); }");
}

#react EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func react_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_EventTest(props) { return $_r.createElement(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }");
}