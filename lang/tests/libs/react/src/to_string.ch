func get_initial_js() : std::string_view {
    return "if(!window.$_ru){window.$_ru=(compRef,props,...children)=>{const p=props?{...props}:{};if(children&&children.length){p.children=children.length===1?children[0]:children;}const U=(pp)=>{const ref=$_r.useRef(null);$_r.useLayoutEffect(()=>{const host=ref.current;if(!host)return;let stop=false;let h=0;const resolve=()=>{if(typeof compRef==='string'){if(window.$_u&&window.$_u[compRef])return window.$_u[compRef];if(window[compRef])return window[compRef];return null;}return compRef;};const mount=()=>{if(stop)return;const comp=resolve();if(!comp){h=(window.requestAnimationFrame?window.requestAnimationFrame(mount):setTimeout(mount,16));return;}let node=null;if(window.$_uc){node=window.$_uc(comp,pp||{});}else{const out=comp(pp||{});if(out&&out.nodeType)node=out;else if(out&&out.root&&out.root.nodeType){node=out.root;if(out.initialize)out.initialize(node,pp||{});}else if(typeof out==='string'||(out&&out.html!==undefined)){const tpl=document.createElement('template');tpl.innerHTML=typeof out==='string'?out:out.html;node=tpl.content.firstElementChild||tpl.content.firstChild;if(node&&out&&out.initialize)out.initialize(node,pp||{});}else if(out&&out.t!==undefined&&window.$_urn){node=window.$_urn(out);}}if(node){host.innerHTML='';host.appendChild(node);}else{host.innerHTML='';}};mount();return()=>{stop=true;if(window.cancelAnimationFrame&&window.requestAnimationFrame&&h)window.cancelAnimationFrame(h);else if(h)clearTimeout(h);};},[pp]);return $_r.createElement('span',{ref});};return $_r.createElement(U,p);};}"
}

func check_initial_js(env : &mut TestEnv, v : std::string_view) : std::string_view {
    const initial = get_initial_js();
    if(v.size() < initial.size()) {
        env.error("react initial js doesn't equal expected, less size");
        return v;
    }
    const gen_init = v.subview(0, initial.size())
    if(initial.equals(gen_init)) {
        return v.skip(initial.size())
    }
    env.error("react initial js doesn't equal expected");
    return v.skip(initial.size())
}

#react Greeting(props) {
    return <span>Hello</span>
}

@test
public func react_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_rm(document.currentScript, Greeting, {});</script>");
}

@test
public func react_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }");
}

#react EmptyElement(props) {
    return <span />
}

@test
public func react_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EmptyElement(props) { return $_r.createElement(\"span\", {}); }");
}

#react ElementChild(props) {
    return <div><span /></div>
}

@test
public func react_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ElementChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(\"span\", {})); }");
}

#react PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func react_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function PropsTest(props) { return $_r.createElement(\"div\", {\"id\": \"myId\", \"className\": \"foo\"}); }");
}

#react NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func react_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function NumericProp(props) { return $_r.createElement(\"div\", {\"tabIndex\": 1}); }");
}

#react SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func react_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function SpreadProps(props) { return $_r.createElement(\"div\", {...props}); }");
}

#react FragmentTest(props) {
    return <><span/></>
}

@test
public func react_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function FragmentTest(props) { return $_r.createElement($_r.Fragment, null, $_r.createElement(\"span\", {})); }");
}

#react ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func react_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentChild(props) { return $_r.createElement(\"div\", {}, $_r.createElement(Greeting, {})); }");
}

#react ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func react_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function ComponentProps(props) { return $_r.createElement(Greeting, {\"text\": \"hi\"}); }");
}

/**
#react TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func react_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function TernaryTest(cond) { return $_r.createElement(\"div\", {}, cond ? $_r.createElement(\"span\", {}, ` a `) : $_r.createElement(\"span\", {}, ` b `)); }");
}
**/

#react MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func react_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function MapTest(items) { return $_r.createElement(\"ul\", {}, items.map((i) => $_r.createElement(\"li\", {}, i))); }");
}

#react EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func react_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, check_initial_js(env, page.getHeadJs()), "function EventTest(props) { return $_r.createElement(\"button\", {\"onClick\": () => alert(\"hi\")}, ` click `); }");
}