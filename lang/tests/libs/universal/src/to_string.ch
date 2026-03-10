func get_initial_js() : std::string_view {
    return "if(!window.$_us){window.$_us=(v)=>{let val=v;const subs=[];return{get value(){return val;},set value(n){val=n;for(let i=0;i<subs.length;i++)subs[i](val);},subscribe(fn){subs.push(fn);}};};}if(!window.$_ur){window.$_ur={Fragment:{},createElement:(t,p,...c)=>({t,p:p||{},c})};window.$_urn=(v)=>{if(v==null||v===false||v===true)return document.createTextNode('');if(v.nodeType)return v;if(v.subscribe&&v.value!==undefined){const n=document.createTextNode(''+v.value);v.subscribe((x)=>n.textContent=''+x);return n;}if(Array.isArray(v)){const f=document.createDocumentFragment();for(let i=0;i<v.length;i++)f.appendChild(window.$_urn(v[i]));return f;}if(typeof v==='string'||typeof v==='number')return document.createTextNode(''+v);if(v&&v.t!==undefined){if(v.t===window.$_ur.Fragment){const f=document.createDocumentFragment();for(let i=0;i<v.c.length;i++)f.appendChild(window.$_urn(v.c[i]));return f;}if(typeof v.t==='function'){return window.$_urn(v.t({...v.p,children:v.c}));}const e=document.createElement(v.t);const props=v.p||{};for(const k in props){const pv=props[k];if(k.startsWith('on')&&typeof pv==='function'){e.addEventListener(k.substring(2).toLowerCase(),pv);}else if(pv&&pv.subscribe&&pv.value!==undefined){if(k in e)e[k]=pv.value;else e.setAttribute(k,''+pv.value);pv.subscribe((x)=>{if(k in e)e[k]=x;else e.setAttribute(k,''+x);});}else if(k==='className'){e.setAttribute('class',pv);}else if(pv!==false&&pv!=null){if(k in e)e[k]=pv;else e.setAttribute(k,''+pv);}}for(let i=0;i<v.c.length;i++)e.appendChild(window.$_urn(v.c[i]));return e;}return document.createTextNode(''+v);};}window.$_r=window.$_r||window.$_ur;if(!window.$_ut){window.$_ut=(e,x)=>{for(let i=0;i<x.length;i++)e=e.children[x[i]];return e;};}if(!window.$_um){window.$_um=(e,comp,props)=>{const data=props||{};const out=comp(data);if(out==null){e.remove();return;}if(out.nodeType){e.replaceWith(out);return;}if(out.root&&out.root.nodeType){const root=out.root;e.replaceWith(root);if(out.initialize)out.initialize(root,data);return;}if(typeof out==='string'||out.html!==undefined){const tpl=document.createElement('template');tpl.innerHTML=typeof out==='string'?out:out.html;const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root){e.remove();return;}e.replaceWith(root);if(out.initialize)out.initialize(root,data);return;}if(out&&out.t!==undefined){e.replaceWith(window.$_urn(out));return;}e.remove();};}if(!window.$_uc){window.$_uc=(factory,props)=>{const out=factory(props||{});if(out==null)return null;if(out.nodeType)return out;if(out.root&&out.root.nodeType)return out.root;if(typeof out==='string'||out.html!==undefined){const tpl=document.createElement('template');tpl.innerHTML=typeof out==='string'?out:out.html;const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return null;if(out.initialize)out.initialize(root,props||{});return root;}if(out&&out.t!==undefined)return window.$_urn(out);return null;};}"
}

func check_initial_js(env : &mut TestEnv, v : std::string_view) : std::string_view {
    const initial = get_initial_js();
    if(v.size() < initial.size()) {
        env.error("universal initial js doesn't equal expected, less size");
        return v;
    }
    const gen_init = v.subview(0, initial.size())
    if(initial.equals(gen_init)) {
        return v.skip(initial.size())
    }
    env.error("universal initial js doesn't equal expected");
    return v.skip(initial.size())
}

#universal Greeting(props) {
    return <span>Hello</span>
}

@test
public func universal_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), """<div id="u3449757589519204369" data-u-comp="Greeting"><span>Hello</span></div>""");
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
    view_equals(env, check_initial_js(env, page.getJs()), "function Greeting(props) { return $_r.createElement(\"span\", {}, ` Hello `); }if(window.$_ureg)window.$_ureg('Greeting',Greeting);");
}

@test
public func universal_element_in_html_works_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Greeting /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function Greeting(props){const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);Greeting.__hydrate(n,props||{});return n;}Greeting.__template='<span>Hello</span>';Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('Greeting',Greeting);");
}

#universal EmptyElement(props) {  }

@test
public func universal_empty_element(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyElement /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function EmptyElement(props){const tpl=document.createElement('template');tpl.innerHTML='<span></span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);EmptyElement.__hydrate(n,props||{});return n;}EmptyElement.__template='<span></span>';EmptyElement.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('EmptyElement',EmptyElement);");
}

#universal ElementChild(props) {  }

@test
public func universal_element_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ElementChild /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function ElementChild(props){const tpl=document.createElement('template');tpl.innerHTML='<div><span></span></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);ElementChild.__hydrate(n,props||{});return n;}ElementChild.__template='<div><span></span></div>';ElementChild.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('ElementChild',ElementChild);");
}

#universal PropsTest(props) {  }

@test
public func universal_props_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <PropsTest /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function PropsTest(props){const tpl=document.createElement('template');tpl.innerHTML='<div id=\"myId\" className=\"foo\"></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);PropsTest.__hydrate(n,props||{});return n;}PropsTest.__template='<div id=\"myId\" className=\"foo\"></div>';PropsTest.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('PropsTest',PropsTest);");
}

#universal NumericProp(props) {  }

@test
public func universal_numeric_prop(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericProp /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function NumericProp(props){const tpl=document.createElement('template');tpl.innerHTML='<div tabIndex=\"1\"></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);NumericProp.__hydrate(n,props||{});return n;}NumericProp.__template='<div tabIndex=\"1\"></div>';NumericProp.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('NumericProp',NumericProp);");
}

#universal SpreadProps(props) {  }

@test
public func universal_spread_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadProps /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function SpreadProps(props){const tpl=document.createElement('template');tpl.innerHTML='<div></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);SpreadProps.__hydrate(n,props||{});return n;}SpreadProps.__template='<div></div>';SpreadProps.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('SpreadProps',SpreadProps);");
}

#universal FragmentTest(props) {  }

@test
public func universal_fragment_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragmentTest /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function FragmentTest(props){const tpl=document.createElement('template');tpl.innerHTML='<span></span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);FragmentTest.__hydrate(n,props||{});return n;}FragmentTest.__template='<span></span>';FragmentTest.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('FragmentTest',FragmentTest);");
}

#universal ComponentChild(props) {  }

@test
public func universal_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function ComponentChild(props) { return $_ur.createElement(\"div\", {}, $_ur.createElement(Greeting, {})); }if(window.$_ureg)window.$_ureg('ComponentChild',ComponentChild);");
}

#universal ComponentProps(props) {  }

@test
public func universal_component_props(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentProps /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function ComponentProps(props) { return $_ur.createElement(Greeting, {\"text\": \"hi\"}); }if(window.$_ureg)window.$_ureg('ComponentProps',ComponentProps);");
}

#universal TernaryTest(props) {  }

@test
public func universal_ternary_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryTest /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function TernaryTest(cond) { return $_ur.createElement(\"div\", {}, cond ? $_ur.createElement(\"span\", {}, ` a `) : $_ur.createElement(\"span\", {}, ` b `)); }if(window.$_ureg)window.$_ureg('TernaryTest',TernaryTest);");
}

#universal MapTest(props) {  }

@test
public func universal_map_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MapTest /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function MapTest(items) { return $_ur.createElement(\"ul\", {}, items.map((i) => $_ur.createElement(\"li\", {}, i))); }if(window.$_ureg)window.$_ureg('MapTest',MapTest);");
}

#universal EventTest(props) {  }

@test
public func universal_event_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventTest /> }
    view_equals(env, check_initial_js(env, page.getJs()), "function EventTest(props){const tpl=document.createElement('template');tpl.innerHTML='<button>click</button>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);EventTest.__hydrate(n,props||{});return n;}EventTest.__template='<button>click</button>';EventTest.__hydrate=(root,props)=>{$_ut(root,[]).addEventListener('click',() => alert(\"hi\"));};if(window.$_ureg)window.$_ureg('EventTest',EventTest);");
}
