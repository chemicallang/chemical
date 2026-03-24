#solid Greeting(props) {
    return <span>Hello</span>
}

@test
public func solid_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_sm(document.currentScript, solid_lib_test_Greeting, {});</script>");
}

@test
public func solid_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }\n");
}

@test
public func solid_component_used_twice_in_html_emits_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting /><Greeting />
    }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }\n");
}

#solid GreetingTwice(props) {
    return <div><Greeting /><Greeting /></div>
}

@test
public func solid_component_used_twice_emits_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <GreetingTwice />
    }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }\nfunction solid_lib_test_GreetingTwice(props) { return $_sh(\"div\", {}, $_s.createComponent(solid_lib_test_Greeting, {}), $_s.createComponent(solid_lib_test_Greeting, {})); }\n");
}

#solid EmptyElement(props) {
    return <span />
}

@test
public func solid_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_EmptyElement(props) { return $_sh(\"span\", {}); }\n");
}

#solid ElementChild(props) {
    return <div><span /></div>
}

@test
public func solid_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ElementChild(props) { return $_sh(\"div\", {}, $_sh(\"span\", {})); }\n");
}

#solid PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func solid_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_PropsTest(props) { return $_sh(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }\n");
}

#solid NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func solid_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_NumericProp(props) { return $_sh(\"div\", {\"tabIndex\": 1}); }\n");
}

#solid SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func solid_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_SpreadProps(props) { return $_sh(\"div\", $_s.mergeProps({}, props, {})); }\n");
}

#solid FragmentTest(props) {
    return <><span/></>
}

@test
public func solid_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_FragmentTest(props) { return [$_sh(\"span\", {})]; }\n");
}

#solid ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func solid_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }\nfunction solid_lib_test_ComponentChild(props) { return $_sh(\"div\", {}, $_s.createComponent(solid_lib_test_Greeting, {})); }\n");
}

#solid ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func solid_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }\nfunction solid_lib_test_ComponentProps(props) { return $_s.createComponent(solid_lib_test_Greeting, {\"text\": \"hi\"}); }\n");
}

#solid TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func solid_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_TernaryTest(cond) { return $_sh(\"div\", {}, () => cond ? $_sh(\"span\", {}, ` a `) : $_sh(\"span\", {}, ` b `)); }\n");
}

#solid MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func solid_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_MapTest(items) { return $_sh(\"ul\", {}, () => items.map((i) => $_sh(\"li\", {}, i))); }\n");
}

#solid EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func solid_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_EventTest(props) { return $_sh(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }");
}