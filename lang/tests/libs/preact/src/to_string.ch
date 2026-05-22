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
    view_equals(env, page.getHeadJs(), "function preact_lib_test_TernaryTest(cond) { return $_p.h(\"div\", {}, (cond ? $_p.h(\"span\", {}, ` a `) : $_p.h(\"span\", {}, ` b `))); }\n");
}

#preact DollarIdentifierTest(props) {
    return <div>{window.$flag ? window.$title : window.$fallback}</div>
}

@test
public func preact_dollar_identifier_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DollarIdentifierTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_DollarIdentifierTest(props) { return $_p.h(\"div\", {}, (window.$flag ? window.$title : window.$fallback)); }\n");
}

#preact StrictTypeofCheck(store) {
    return <div>{!store || typeof store !== "object" ? "bad" : "ok"}</div>
}

@test
public func preact_strict_typeof_check_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StrictTypeofCheck /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_StrictTypeofCheck(store) { return $_p.h(\"div\", {}, (!store || typeof store !== \"object\" ? \"bad\" : \"ok\")); }\n");
}

#preact ParenthesizedTernaryLabel(item) {
    return <div>{(item.checked ? "[x] " : "[ ] ") + item.text}</div>
}

@test
public func preact_parenthesized_ternary_with_member_access_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ParenthesizedTernaryLabel /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ParenthesizedTernaryLabel(item) { return $_p.h(\"div\", {}, ((item.checked ? \"[x] \" : \"[ ] \")) + item.text); }\n");
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

#preact VarDeclTest(props) {
    var x = 10
    return <div>{x}</div>
}

@test
public func preact_var_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarDeclTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_VarDeclTest(props) { var x = 10; return $_p.h(\"div\", {}, x); }\n");
}

#preact ConstStringTest(props) {
    const name = "test"
    return <div>{name}</div>
}

@test
public func preact_const_string_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ConstStringTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ConstStringTest(props) { const name = \"test\"; return $_p.h(\"div\", {}, name); }\n");
}

#preact LetDeclTest(props) {
    let count = 0
    return <div>{count}</div>
}

@test
public func preact_let_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LetDeclTest /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_LetDeclTest(props) { let count = 0; return $_p.h(\"div\", {}, count); }\n");
}

#preact IfReturnGuard(props) {
    if(props.cond) return null
    return <span>ok</span>
}

@test
public func preact_if_return_guard(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <IfReturnGuard /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_IfReturnGuard(props) { if(props.cond) return null; return $_p.h(\"span\", {}, ` ok `); }\n");
}

#preact ComplexBody(props) {
    var x = 10
    const name = "test"
    if(x > 5) {
        return <div>{name}</div>
    }
    return <span>small</span>
}

@test
public func preact_complex_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComplexBody /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ComplexBody(props) { var x = 10; const name = \"test\"; if(x > 5) { return $_p.h(\"div\", {}, name); } return $_p.h(\"span\", {}, ` small `); }\n");
}

#preact ForLoopBody(props) {
    var items = []
    for(var i = 0; i < 3; i++) {
        items.push(i)
    }
    return <div>{items}</div>
}

@test
public func preact_for_loop_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ForLoopBody /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ForLoopBody(props) { var items = []; for(var i = 0;i < 3;i++) { items.push(i); } return $_p.h(\"div\", {}, items); }\n");
}

#preact FnCallBody(props) {
    var result = props.greet("hello")
    return <div>{result}</div>
}

@test
public func preact_fn_call_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FnCallBody /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_FnCallBody(props) { var result = props.greet(\"hello\"); return $_p.h(\"div\", {}, result); }\n");
}

#preact ObjectInBody(props) {
    var obj = { a : 1, b : "two" }
    return <div>{obj.a}</div>
}

@test
public func preact_object_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ObjectInBody /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ObjectInBody(props) { var obj = {a: 1, b: \"two\"}; return $_p.h(\"div\", {}, obj.a); }\n");
}

#preact ArrowFnInBody(props) {
    var double = (n) => n * 2
    return <div>{double(5)}</div>
}

@test
public func preact_arrow_fn_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ArrowFnInBody /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ArrowFnInBody(props) { var double = (n) => n * 2; return $_p.h(\"div\", {}, double(5)); }\n");
}

#preact ReturnGuardAssignment(props) {
    var x = 0
    if(props.cond) return
    x = 1
    return <span>{x}</span>
}

@test
public func preact_return_guard_assignment(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ReturnGuardAssignment /> }
    view_equals(env, page.getHeadJs(), "function preact_lib_test_ReturnGuardAssignment(props) { var x = 0; if(props.cond) return; x = 1; return $_p.h(\"span\", {}, x); }\n");
}
