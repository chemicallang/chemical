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
    view_equals(env, page.getHeadJs(), "function solid_lib_test_TernaryTest(cond) { return $_sh(\"div\", {}, () => (cond ? $_sh(\"span\", {}, ` a `) : $_sh(\"span\", {}, ` b `))); }\n");
}

#solid DollarIdentifierTest(props) {
    return <div>{window.$flag ? window.$title : window.$fallback}</div>
}

@test
public func solid_dollar_identifier_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DollarIdentifierTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_DollarIdentifierTest(props) { return $_sh(\"div\", {}, (window.$flag ? window.$title : window.$fallback)); }\n");
}

#solid StrictTypeofCheck(store) {
    return <div>{!store || typeof store !== "object" ? "bad" : "ok"}</div>
}

@test
public func solid_strict_typeof_check_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StrictTypeofCheck /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_StrictTypeofCheck(store) { return $_sh(\"div\", {}, (!store || typeof store !== \"object\" ? \"bad\" : \"ok\")); }\n");
}

#solid ParenthesizedTernaryLabel(item) {
    return <div>{(item.checked ? "[x] " : "[ ] ") + item.text}</div>
}

@test
public func solid_parenthesized_ternary_with_member_access_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ParenthesizedTernaryLabel /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ParenthesizedTernaryLabel(item) { return $_sh(\"div\", {}, ((item.checked ? \"[x] \" : \"[ ] \")) + item.text); }\n");
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
    view_equals(env, page.getHeadJs(), "function solid_lib_test_EventTest(props) { return $_sh(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }\n");
}

#solid BacktickText(props) {
    return <p>The code is `sync.status` and `lastSyncedAt`.</p>
}

@test
public func solid_backtick_in_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BacktickText /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_BacktickText(props) { return $_sh(\"p\", {}, ` The code is \\`sync.status\\` and \\`lastSyncedAt\\`. `); }\n");
}

#solid VarDeclTest(props) {
    var x = 10
    return <div>{x}</div>
}

@test
public func solid_var_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarDeclTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_VarDeclTest(props) { var x = 10; return $_sh(\"div\", {}, x); }\n");
}

#solid ConstStringTest(props) {
    const name = "test"
    return <div>{name}</div>
}

@test
public func solid_const_string_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ConstStringTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ConstStringTest(props) { const name = \"test\"; return $_sh(\"div\", {}, name); }\n");
}

#solid LetDeclTest(props) {
    let count = 0
    return <div>{count}</div>
}

@test
public func solid_let_decl_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LetDeclTest /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_LetDeclTest(props) { let count = 0; return $_sh(\"div\", {}, count); }\n");
}

#solid IfReturnGuard(props) {
    if(props.cond) return null
    return <span>ok</span>
}

@test
public func solid_if_return_guard(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <IfReturnGuard /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_IfReturnGuard(props) { if(props.cond) return null; return $_sh(\"span\", {}, ` ok `); }\n");
}

#solid ComplexBody(props) {
    var x = 10
    const name = "test"
    if(x > 5) {
        return <div>{name}</div>
    }
    return <span>small</span>
}

@test
public func solid_complex_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComplexBody /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ComplexBody(props) { var x = 10; const name = \"test\"; if(x > 5) { return $_sh(\"div\", {}, name); } return $_sh(\"span\", {}, ` small `); }\n");
}

#solid ForLoopBody(props) {
    var items = []
    for(var i = 0; i < 3; i++) {
        items.push(i)
    }
    return <div>{items}</div>
}

@test
public func solid_for_loop_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ForLoopBody /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ForLoopBody(props) { var items = []; for(var i = 0;i < 3;i++) { items.push(i); } return $_sh(\"div\", {}, items); }\n");
}

#solid FnCallBody(props) {
    var result = props.greet("hello")
    return <div>{result}</div>
}

@test
public func solid_fn_call_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FnCallBody /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_FnCallBody(props) { var result = props.greet(\"hello\"); return $_sh(\"div\", {}, result); }\n");
}

#solid ObjectInBody(props) {
    var obj = { a : 1, b : "two" }
    return <div>{obj.a}</div>
}

@test
public func solid_object_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ObjectInBody /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ObjectInBody(props) { var obj = {a: 1, b: \"two\"}; return $_sh(\"div\", {}, obj.a); }\n");
}

#solid ArrowFnInBody(props) {
    var double = (n) => n * 2
    return <div>{double(5)}</div>
}

@test
public func solid_arrow_fn_in_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ArrowFnInBody /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ArrowFnInBody(props) { var double = (n) => n * 2; return $_sh(\"div\", {}, () => double(5)); }\n");
}

#solid ReturnGuardAssignment(props) {
    var x = 0
    if(props.cond) return
    x = 1
    return <span>{x}</span>
}

@test
public func solid_return_guard_assignment(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ReturnGuardAssignment /> }
    view_equals(env, page.getHeadJs(), "function solid_lib_test_ReturnGuardAssignment(props) { var x = 0; if(props.cond) return; x = 1; return $_sh(\"span\", {}, x); }\n");
}
