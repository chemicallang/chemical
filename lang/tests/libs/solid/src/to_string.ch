func get_initial_js() : std::string_view {
    return "if(!window.$_su){window.$_su=(compRef,props,...children)=>{const p=props?{...props}:{};if(children&&children.length){p.children=children.length===1?children[0]:children;}const host=document.createElement('span');let stop=false;let h=0;const resolve=()=>{if(typeof compRef==='string'){if(window.$_u&&window.$_u[compRef])return window.$_u[compRef];if(window[compRef])return window[compRef];return null;}return compRef;};const mount=()=>{if(stop)return;const comp=resolve();if(!comp){h=(window.requestAnimationFrame?window.requestAnimationFrame(mount):setTimeout(mount,16));return;}let node=null;if(window.$_uc){node=window.$_uc(comp,p);}else{const out=comp(p);if(out&&out.nodeType)node=out;else if(out&&out.root&&out.root.nodeType){node=out.root;if(out.initialize)out.initialize(node,p);}else if(typeof out==='string'||(out&&out.html!==undefined)){const tpl=document.createElement('template');tpl.innerHTML=typeof out==='string'?out:out.html;node=tpl.content.firstElementChild||tpl.content.firstChild;if(node&&out&&out.initialize)out.initialize(node,p);}else if(out&&out.t!==undefined&&window.$_urn){node=window.$_urn(out);}}host.innerHTML='';if(node)host.appendChild(node);};mount();return host;};}"
}

func check_initial_js(env : &mut TestEnv, v : std::string_view) : std::string_view {
    const initial = get_initial_js();
    if(v.size() < initial.size()) {
        env.error("solid initial js doesn't equal expected, less size");
        return v;
    }
    const gen_init = v.subview(0, initial.size())
    if(initial.equals(gen_init)) {
        return v.skip(initial.size())
    }
    env.error("solid initial js doesn't equal expected");
    return v.skip(initial.size())
}

#solid Greeting(props) {
    return <span>Hello</span>
}

@test
public func solid_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_sm(document.currentScript, Greeting, {});</script>");
}

@test
public func solid_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function Greeting(props) { return $_sh(\"span\", {}, ` Hello `); }");
}

#solid EmptyElement(props) {
    return <span />
}

@test
public func solid_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EmptyElement(props) { return $_sh(\"span\", {}); }");
}

#solid ElementChild(props) {
    return <div><span /></div>
}

@test
public func solid_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ElementChild(props) { return $_sh(\"div\", {}, $_sh(\"span\", {})); }");
}

#solid PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func solid_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function PropsTest(props) { return $_sh(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }");
}

#solid NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func solid_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function NumericProp(props) { return $_sh(\"div\", {\"tabIndex\": 1}); }");
}

#solid SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func solid_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function SpreadProps(props) { return $_sh(\"div\", $_s.mergeProps({}, props, {})); }");
}

#solid FragmentTest(props) {
    return <><span/></>
}

@test
public func solid_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function FragmentTest(props) { return [$_sh(\"span\", {})]; }");
}

#solid ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func solid_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentChild(props) { return $_sh(\"div\", {}, $_s.createComponent(Greeting, {})); }");
}

#solid ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func solid_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentProps(props) { return $_s.createComponent(Greeting, {\"text\": \"hi\"}); }");
}

/**
#solid TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func solid_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function TernaryTest(cond) { return $_sh(\"div\", {}, () => cond ? $_sh(\"span\", {}, ` a `) : $_sh(\"span\", {}, ` b `)); }");
}
**/

#solid MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func solid_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function MapTest(items) { return $_sh(\"ul\", {}, () => items.map((i) => $_sh(\"li\", {}, i))); }");
}

#solid EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func solid_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EventTest(props) { return $_sh(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }");
}