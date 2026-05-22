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
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }\n");
}

@test
public func react_element_used_twice_in_html_creates_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting /><Greeting />
    }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }\n");
}

#react GreetingTwice(props) {
    return <div><Greeting /><Greeting /></div>
}

@test
public func react_component_used_twice_emits_once(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <GreetingTwice />
    }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }\nfunction react_lib_test_GreetingTwice(props) { return $_r.createElement(\"div\", {}, $_r.createElement(react_lib_test_Greeting, {}), $_r.createElement(react_lib_test_Greeting, {})); }\n");
}

#react EmptyElement(props) {
    return <span />
}

@test
public func react_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_EmptyElement(props) { return $_r.createElement(\"span\", {}); }\n");
}

#react ElementChild(props) {
    return <div><span /></div>
}

@test
public func react_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ElementChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(\"span\", {})); }\n");
}

#react PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func react_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_PropsTest(props) { return $_r.createElement(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }\n");
}

#react NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func react_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_NumericProp(props) { return $_r.createElement(\"div\", {\"tabIndex\": 1}); }\n");
}

#react SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func react_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_SpreadProps(props) { return $_r.createElement(\"div\", {...props}); }\n");
}

#react FragmentTest(props) {
    return <><span/></>
}

@test
public func react_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_FragmentTest(props) { return $_r.createElement($_r.Fragment, null, $_r.createElement(\"span\", {})); }\n");
}

#react ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func react_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }\nfunction react_lib_test_ComponentChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(react_lib_test_Greeting, {})); }\n");
}

#react ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func react_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }\nfunction react_lib_test_ComponentProps(props) { return $_r.createElement(react_lib_test_Greeting, {\"text\": \"hi\"}); }\n");
}

#react TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func react_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_TernaryTest(cond) { return $_r.createElement(\"div\", {}, (cond ? $_r.createElement(\"span\", {}, ` a `) : $_r.createElement(\"span\", {}, ` b `))); }\n");
}

#react DollarIdentifierTest(props) {
    return <div>{window.$flag ? window.$title : window.$fallback}</div>
}

@test
public func react_dollar_identifier_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DollarIdentifierTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_DollarIdentifierTest(props) { return $_r.createElement(\"div\", {}, (window.$flag ? window.$title : window.$fallback)); }\n");
}

#react StrictTypeofCheck(store) {
    return <div>{!store || typeof store !== "object" ? "bad" : "ok"}</div>
}

@test
public func react_strict_typeof_check_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StrictTypeofCheck /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_StrictTypeofCheck(store) { return $_r.createElement(\"div\", {}, (!store || typeof store !== \"object\" ? \"bad\" : \"ok\")); }\n");
}

#react ParenthesizedTernaryLabel(item) {
    return <div>{(item.checked ? "[x] " : "[ ] ") + item.text}</div>
}

@test
public func react_parenthesized_ternary_with_member_access_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ParenthesizedTernaryLabel /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ParenthesizedTernaryLabel(item) { return $_r.createElement(\"div\", {}, ((item.checked ? \"[x] \" : \"[ ] \")) + item.text); }\n");
}

#react MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func react_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_MapTest(items) { return $_r.createElement(\"ul\", {}, items.map((i) => $_r.createElement(\"li\", {}, i))); }\n");
}

#react EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func react_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_EventTest(props) { return $_r.createElement(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }\n");
}

#react BacktickText(props) {
    return <p>The code is `sync.status` and `lastSyncedAt`.</p>
}

@test
public func react_backtick_in_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BacktickText /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_BacktickText(props) { return $_r.createElement(\"p\", {}, ` The code is \\`sync.status\\` and \\`lastSyncedAt\\`. `); }\n");
}

#react VarDeclTest(props) {
    var x = 10
    return <div>{x}</div>
}

@test
public func react_var_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarDeclTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_VarDeclTest(props) { var x = 10; return $_r.createElement(\"div\", {}, x); }\n");
}

#react ConstStringTest(props) {
    const name = "test"
    return <div>{name}</div>
}

@test
public func react_const_string_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ConstStringTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ConstStringTest(props) { const name = \"test\"; return $_r.createElement(\"div\", {}, name); }\n");
}

#react LetDeclTest(props) {
    let count = 0
    return <div>{count}</div>
}

@test
public func react_let_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LetDeclTest /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_LetDeclTest(props) { let count = 0; return $_r.createElement(\"div\", {}, count); }\n");
}

#react IfReturnGuard(props) {
    if(props.cond) return null
    return <span>ok</span>
}

@test
public func react_if_return_guard(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <IfReturnGuard /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_IfReturnGuard(props) { if(props.cond) return null; return $_r.createElement(\"span\", {}, ` ok `); }\n");
}

#react ComplexBody(props) {
    var x = 10
    const name = "test"
    if(x > 5) {
        return <div>{name}</div>
    }
    return <span>small</span>
}

@test
public func react_complex_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComplexBody /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ComplexBody(props) { var x = 10; const name = \"test\"; if(x > 5) { return $_r.createElement(\"div\", {}, name); } return $_r.createElement(\"span\", {}, ` small `); }\n");
}

#react ForLoopBody(props) {
    var items = []
    for(var i = 0; i < 3; i++) {
        items.push(i)
    }
    return <div>{items}</div>
}

@test
public func react_for_loop_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ForLoopBody /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ForLoopBody(props) { var items = []; for(var i = 0;i < 3;i++) { items.push(i); } return $_r.createElement(\"div\", {}, items); }\n");
}

#react FnCallBody(props) {
    var result = props.greet("hello")
    return <div>{result}</div>
}

@test
public func react_fn_call_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FnCallBody /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_FnCallBody(props) { var result = props.greet(\"hello\"); return $_r.createElement(\"div\", {}, result); }\n");
}

#react ObjectInBody(props) {
    var obj = { a : 1, b : "two" }
    return <div>{obj.a}</div>
}

@test
public func react_object_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ObjectInBody /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ObjectInBody(props) { var obj = {a: 1, b: \"two\"}; return $_r.createElement(\"div\", {}, obj.a); }\n");
}

#react ArrowFnInBody(props) {
    var double = (n) => n * 2
    return <div>{double(5)}</div>
}

@test
public func react_arrow_fn_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ArrowFnInBody /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ArrowFnInBody(props) { var double = (n) => n * 2; return $_r.createElement(\"div\", {}, double(5)); }\n");
}

#react ReturnGuardAssignment(props) {
    var x = 0
    if(props.cond) return
    x = 1
    return <span>{x}</span>
}

@test
public func react_return_guard_assignment(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ReturnGuardAssignment /> }
    view_equals(env, page.getHeadJs(), "function react_lib_test_ReturnGuardAssignment(props) { var x = 0; if(props.cond) return; x = 1; return $_r.createElement(\"span\", {}, x); }\n");
}
