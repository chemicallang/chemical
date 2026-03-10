func get_initial_js() : std::string_view {
    return "if(!window.$_pu){window.$_pu=(compRef,props,...children)=>{const p=props?{...props}:{};if(children&&children.length){p.children=children.length===1?children[0]:children;}const U=(pp)=>{const ref=$_ph.useRef(null);$_ph.useLayoutEffect(()=>{const host=ref.current;if(!host)return;let stop=false;let h=0;const resolve=()=>{if(typeof compRef==='string'){if(window.$_u&&window.$_u[compRef])return window.$_u[compRef];if(window[compRef])return window[compRef];return null;}return compRef;};const mount=()=>{if(stop)return;const comp=resolve();if(!comp){h=(window.requestAnimationFrame?window.requestAnimationFrame(mount):setTimeout(mount,16));return;}let node=null;if(window.$_uc){node=window.$_uc(comp,pp||{});}else{const out=comp(pp||{});if(out&&out.nodeType)node=out;else if(out&&out.root&&out.root.nodeType){node=out.root;if(out.initialize)out.initialize(node,pp||{});}else if(typeof out==='string'||(out&&out.html!==undefined)){const tpl=document.createElement('template');tpl.innerHTML=typeof out==='string'?out:out.html;node=tpl.content.firstElementChild||tpl.content.firstChild;if(node&&out&&out.initialize)out.initialize(node,pp||{});}else if(out&&out.t!==undefined&&window.$_urn){node=window.$_urn(out);}}if(node){host.innerHTML='';host.appendChild(node);}else{host.innerHTML='';}};mount();return()=>{stop=true;if(window.cancelAnimationFrame&&window.requestAnimationFrame&&h)window.cancelAnimationFrame(h);else if(h)clearTimeout(h);};},[pp]);return $_p.h('span',{ref});};return $_p.h(U,p);};}"
}

func check_initial_js(env : &mut TestEnv, v : std::string_view) : std::string_view {
    const initial = get_initial_js();
    if(v.size() < initial.size()) {
        env.error("preact initial js doesn't equal expected, less size");
        return v;
    }
    const gen_init = v.subview(0, initial.size())
    if(initial.equals(gen_init)) {
        return v.skip(initial.size())
    }
    env.error("preact initial js doesn't equal expected");
    return v.skip(initial.size())
}

#preact Greeting(props) {
    return <span>Hello</span>
}

@test
public func preact_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_pm(document.currentScript, Greeting, {});</script>");
}

@test
public func preact_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function Greeting(props) { return $_p.h(\"span\", {}, ` Hello `); }");
}

#preact EmptyElement(props) {
    return <span />
}

@test
public func preact_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EmptyElement(props) { return $_p.h(\"span\", {}); }");
}

#preact ElementChild(props) {
    return <div><span /></div>
}

@test
public func preact_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ElementChild(props) { return $_p.h(\"div\", {}, $_p.h(\"span\", {})); }");
}

#preact PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func preact_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function PropsTest(props) { return $_p.h(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }");
}

#preact NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func preact_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function NumericProp(props) { return $_p.h(\"div\", {\"tabIndex\": 1}); }");
}

#preact SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func preact_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function SpreadProps(props) { return $_p.h(\"div\", {...props}); }");
}

#preact FragmentTest(props) {
    return <><span/></>
}

@test
public func preact_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function FragmentTest(props) { return $_p.h($_p.Fragment, null, $_p.h(\"span\", {})); }");
}

#preact ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func preact_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentChild(props) { return $_p.h(\"div\", {}, $_p.h(Greeting, {})); }");
}

#preact ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func preact_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentProps(props) { return $_p.h(Greeting, {\"text\": \"hi\"}); }");
}

/**
#preact TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func preact_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function TernaryTest(cond) { return $_p.h(\"div\", {}, cond ? $_p.h(\"span\", {}, ` a `) : $_p.h(\"span\", {}, ` b `)); }");
}
**/

#preact MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func preact_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function MapTest(items) { return $_p.h(\"ul\", {}, items.map((i) => $_p.h(\"li\", {}, i))); }");
}

#preact EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func preact_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EventTest(props) { return $_p.h(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }");
}