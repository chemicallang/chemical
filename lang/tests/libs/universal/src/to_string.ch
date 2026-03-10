#universal Greeting(props) {
    return <span>Hello</span>
}

@test
public func universal_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), """<div id="u3449757589519204369" data-u-comp="universal_lib_test_Greeting"><span>Hello</span></div>""");
}

@test
public func universal_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHeadJsOnly(), "");
}

@test
public func universal_element_in_html_works_head_js2(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    view_equals(env, page.getJs(), "function universal_lib_test_Greeting(props){const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__template='<span>Hello</span>';universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449758207994494993','universal_lib_test_Greeting',{}]);if(window.$_uf)window.$_uf();");
}

@test
public func universal_element_in_html_works_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Greeting /> }
    view_equals(env, page.getJs(), "function universal_lib_test_Greeting(props){const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__template='<span>Hello</span>';universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449758482905956373','universal_lib_test_Greeting',{}]);if(window.$_uf)window.$_uf();");
}

#universal EmptyElement(props) {
    return <span />
}

@test
public func universal_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, page.getJs(), "function universal_lib_test_EmptyElement(props){const tpl=document.createElement('template');tpl.innerHTML='<span></span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_EmptyElement.__hydrate(n,props||{});return n;}universal_lib_test_EmptyElement.__template='<span></span>';universal_lib_test_EmptyElement.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_EmptyElement',universal_lib_test_EmptyElement);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449758860863078425','universal_lib_test_EmptyElement',{}]);if(window.$_uf)window.$_uf();");
}

#universal ElementChild(props) {
    return <div><span /></div>
}

@test
public func universal_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, page.getJs(), "function universal_lib_test_ElementChild(props){const tpl=document.createElement('template');tpl.innerHTML='<div><span></span></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_ElementChild.__hydrate(n,props||{});return n;}universal_lib_test_ElementChild.__template='<div><span></span></div>';universal_lib_test_ElementChild.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_ElementChild',universal_lib_test_ElementChild);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449759238820200473','universal_lib_test_ElementChild',{}]);if(window.$_uf)window.$_uf();");
}

#universal PropsTest(props) {
    return <div id="myId" className="foo"></div>
}

@test
public func universal_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, page.getJs(), "function universal_lib_test_PropsTest(props){const tpl=document.createElement('template');tpl.innerHTML='<div id=\"myId\" className=\"foo\"></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_PropsTest.__hydrate(n,props||{});return n;}universal_lib_test_PropsTest.__template='<div id=\"myId\" className=\"foo\"></div>';universal_lib_test_PropsTest.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_PropsTest',universal_lib_test_PropsTest);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449759616777322518','universal_lib_test_PropsTest',{}]);if(window.$_uf)window.$_uf();");
}

#universal NumericProp(props) {
    return <div tabIndex={1}></div>
}

@test
public func universal_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, page.getJs(), "function universal_lib_test_NumericProp(props){const tpl=document.createElement('template');tpl.innerHTML='<div tabIndex=\"1\"></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_NumericProp.__hydrate(n,props||{});return n;}universal_lib_test_NumericProp.__template='<div tabIndex=\"1\"></div>';universal_lib_test_NumericProp.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_NumericProp',universal_lib_test_NumericProp);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449759994734444568','universal_lib_test_NumericProp',{}]);if(window.$_uf)window.$_uf();");
}

#universal SpreadProps(props) {
    return <div {...props}></div>
}

@test
public func universal_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, page.getJs(), "function universal_lib_test_SpreadProps(props){const tpl=document.createElement('template');tpl.innerHTML='<div></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_SpreadProps.__hydrate(n,props||{});return n;}universal_lib_test_SpreadProps.__template='<div></div>';universal_lib_test_SpreadProps.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_SpreadProps',universal_lib_test_SpreadProps);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449760372691566616','universal_lib_test_SpreadProps',{}]);if(window.$_uf)window.$_uf();");
}

#universal FragmentTest(props) {
    return <><span/></>
}

@test
public func universal_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, page.getJs(), "function universal_lib_test_FragmentTest(props){const tpl=document.createElement('template');tpl.innerHTML='<span></span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_FragmentTest.__hydrate(n,props||{});return n;}universal_lib_test_FragmentTest.__template='<span></span>';universal_lib_test_FragmentTest.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_FragmentTest',universal_lib_test_FragmentTest);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449760750648688665','universal_lib_test_FragmentTest',{}]);if(window.$_uf)window.$_uf();");
}

#universal ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func universal_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, page.getJs(), "function universal_lib_test_Greeting(props){const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__template='<span>Hello</span>';universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);function universal_lib_test_ComponentChild(props) { return $_ur.createElement(\"div\", {}, $_ur.createElement(universal_lib_test_Greeting, {})); }if(window.$_ureg)window.$_ureg('universal_lib_test_ComponentChild',universal_lib_test_ComponentChild);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449761128605810715','universal_lib_test_ComponentChild',{}]);if(window.$_uf)window.$_uf();");
}

#universal ComponentProps(props) {
    return <Greeting text="hi" />
}

@test
public func universal_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, page.getJs(), "function universal_lib_test_Greeting(props){const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__template='<span>Hello</span>';universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);function universal_lib_test_ComponentProps(props) { return $_ur.createElement(universal_lib_test_Greeting, {\"text\": \"hi\"}); }if(window.$_ureg)window.$_ureg('universal_lib_test_ComponentProps',universal_lib_test_ComponentProps);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449761506562932763','universal_lib_test_ComponentProps',{}]);if(window.$_uf)window.$_uf();");
}

#universal TernaryTest(cond) {
    return <div>{cond ? <span>a</span> : <span>b</span>}</div>
}

@test
public func universal_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, page.getJs(), "function universal_lib_test_TernaryTest(cond) { return $_ur.createElement(\"div\", {}, cond ? $_ur.createElement(\"span\", {}, ` a `) : $_ur.createElement(\"span\", {}, ` b `)); }if(window.$_ureg)window.$_ureg('universal_lib_test_TernaryTest',universal_lib_test_TernaryTest);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449761884520054808','universal_lib_test_TernaryTest',{}]);if(window.$_uf)window.$_uf();");
}

#universal MapTest(items) {
    return <ul>{items.map(i => <li>{i}</li>)}</ul>
}

@test
public func universal_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, page.getJs(), "function universal_lib_test_MapTest(items) { return $_ur.createElement(\"ul\", {}, items.map((i) => $_ur.createElement(\"li\", {}, i))); }if(window.$_ureg)window.$_ureg('universal_lib_test_MapTest',universal_lib_test_MapTest);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449762262477176852','universal_lib_test_MapTest',{}]);if(window.$_uf)window.$_uf();");
}

#universal EventTest(props) {
    return <button onClick={() => alert("hi")}>click</button>
}

@test
public func universal_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, page.getJs(), "function universal_lib_test_EventTest(props){const tpl=document.createElement('template');tpl.innerHTML='<button>click</button>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_EventTest.__hydrate(n,props||{});return n;}universal_lib_test_EventTest.__template='<button>click</button>';universal_lib_test_EventTest.__hydrate=(root,props)=>{$_ut(root,[]).addEventListener('click',() => alert(\"hi\"));};if(window.$_ureg)window.$_ureg('universal_lib_test_EventTest',universal_lib_test_EventTest);window.$_uq=window.$_uq||[];window.$_uq.push(['u3449762640434298902','universal_lib_test_EventTest',{}]);if(window.$_uf)window.$_uf();");
}
