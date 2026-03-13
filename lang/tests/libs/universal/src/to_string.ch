func (page : &HtmlPage) getComponentId(index : size_t) : std::string_view {
    var view = page.getHtml()
    var search = std::string_view("id=\"u")
    var i : size_t = 0;
    var count : size_t = 0;
    while(i < view.size()) {
        if(view.subview(i, view.size()).starts_with(search)) {
            if(count == index) {
                var start = i + 4;
                var end = start;
                while(end < view.size() && view.get(end) != '"') end++;
                return view.subview(start, end);
            }
            count++;
        }
        i++;
    }
    return std::string_view("not_found");
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
    var str = std::string();
    str.append_expr(`<div id="${page.getComponentId(0)}" data-u-comp="universal_lib_test_Greeting"><span>Hello</span></div>`);
    view_equals(env, page.getHtml(), str.to_view());
}

@test
public func universal_element_in_html_works_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <Greeting /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_Greeting(props) {const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);window.$_uq.push(['${page.getComponentId(0)}','universal_lib_test_Greeting',{}]);`)
    view_equals(env, page.getJs(), str.to_view());
}

#universal ComponentChild(props) {
    return <div><Greeting /></div>
}

@test
public func universal_component_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComponentChild /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_Greeting(props) {const tpl=document.createElement('template');tpl.innerHTML='<span>Hello</span>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_Greeting.__hydrate(n,props||{});return n;}universal_lib_test_Greeting.__hydrate=(root,props)=>{};if(window.$_ureg)window.$_ureg('universal_lib_test_Greeting',universal_lib_test_Greeting);function universal_lib_test_ComponentChild(props) {const tpl=document.createElement('template');tpl.innerHTML='<div><span>Hello</span></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_ComponentChild.__hydrate(n,props||{});return n;}universal_lib_test_ComponentChild.__hydrate=(root,props)=>{{const c=(window.$_u&&window.$_u['universal_lib_test_Greeting'])||window['universal_lib_test_Greeting'];if(c&&c.__hydrate){c.__hydrate($_ut(root,[0]),{});}}};if(window.$_ureg)window.$_ureg('universal_lib_test_ComponentChild',universal_lib_test_ComponentChild);window.$_uq.push(['${page.getComponentId(0)}','universal_lib_test_ComponentChild',{}]);`)
    view_equals(env, page.getJs(), str.to_view());
}

#universal ClassMerge(props) {
    return <div {...props} class="base-class"></div>
}

@test
public func universal_class_merge(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ClassMerge class="extra" /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_ClassMerge\" class=\"base-class\"></div>`)
    view_equals(env, page.getHtml(), html.to_view())
    
    var js = std::string()
    js.append_expr(`function universal_lib_test_ClassMerge(props) {const tpl=document.createElement('template');tpl.innerHTML='<div class=\"base-class\"></div>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_ClassMerge.__hydrate(n,props||{});return n;}universal_lib_test_ClassMerge.__hydrate=(root,props)=>{{const el=$_ut(root,[]);let v=\"\";let h=false;const a=(x)=>{if(x==null||x===false)return;const s=\"\"+x;if(s.length===0)return;if(h)v+=\" \";v+=s;h=true;};a(props.className);a(props.class);a('base-class');if(h) el.setAttribute('class', v);}{const el=$_ut(root,[]);let v=\"\";let h=false;const a=(x)=>{if(x==null||x===false)return;const s=\"\"+x;if(s.length===0)return;if(h)v+=\";\";v+=s;h=true;};a(props.style);if(h) el.setAttribute('style', v);}};if(window.$_ureg)window.$_ureg('universal_lib_test_ClassMerge',universal_lib_test_ClassMerge);window.$_uq.push(['${page.getComponentId(0)}','universal_lib_test_ClassMerge',{\"class\":\"extra\"}]);`)
    view_equals(env, page.getJs(), js.to_view())
}

#universal Button(props) {
    return <button {...props} class="btn-base">{props.children}</button>
}

#universal ButtonPrimary(props) {
    return <Button {...props} class="btn-primary">{props.children}</Button>
}

@test
public func universal_button_variant(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ButtonPrimary>Click Me</ButtonPrimary> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_ButtonPrimary\"><button class=\"btn-base btn-primary\">Click Me</button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal DeepPropPassing(props) {
    return <div title={props.title}>
        <span>{props.label}</span>
    </div>
}

@test
public func universal_deep_prop_passing(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DeepPropPassing title="Hover Me" label="Text" /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_DeepPropPassing\"><div title=\"Hover Me\"><span>Text</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal MultiChild(props) {
    return <div>
        <span>A</span>
        <span>B</span>
    </div>
}

@test
public func universal_multi_child(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MultiChild /> }
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_MultiChild\"><div><span>A</span><span>B</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal StateTest(props) {
    state count = 0;
    return <button onClick={() => count++}>{count}</button>
}

@test
public func universal_state_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StateTest /> }
    var str = std::string()
    str.append_expr(`function universal_lib_test_StateTest(props) {const tpl=document.createElement('template');tpl.innerHTML='<button><span>0</span></button>';const root=tpl.content.firstElementChild||tpl.content.firstChild;if(!root)return document.createTextNode('');const n=root.cloneNode(true);universal_lib_test_StateTest.__hydrate(n,props||{});return n;}universal_lib_test_StateTest.__hydrate=(root,props)=>{const count = $_us(0);count.subscribe(v=>$_ut(root,[0]).textContent=v);$_ut(root,[]).addEventListener('click',() => count.value++);};if(window.$_ureg)window.$_ureg('universal_lib_test_StateTest',universal_lib_test_StateTest);window.$_uq.push(['${page.getComponentId(0)}','universal_lib_test_StateTest',{}]);`)
    view_equals(env, page.getJs(), str.to_view());
}

#universal NestedUniversal(props) {
    return <Greeting />
}

@test
public func universal_nested_universal_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedUniversal /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_NestedUniversal\"><div id=\"${page.getComponentId(1)}\" data-u-comp=\"universal_lib_test_Greeting\"><span>Hello</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal NestedUniversalProps(props) {
    return <DeepPropPassing title={props.title} label={props.label} />
}

@test
public func universal_nested_universal_props_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedUniversalProps title="T" label="L" /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_NestedUniversalProps\"><div id=\"${page.getComponentId(1)}\" data-u-comp=\"universal_lib_test_DeepPropPassing\"><div title=\"T\"><span>L</span></div></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal ConditionalRender(props) {
    return <div>{props.show ? <span>Show</span> : <span>Hide</span>}</div>
}

@test
public func universal_conditional_render_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ConditionalRender show={true} /> }
    
    var js = std::string()
    js.append_expr(`function universal_lib_test_ConditionalRender(props) { return $_ur.createElement(\"div\", {}, props.show ? $_ur.createElement(\"span\", {}, ${"` Show `"}) : $_ur.createElement(\"span\", {}, ${"` Hide `"})); }if(window.$_ureg)window.$_ureg('universal_lib_test_ConditionalRender',universal_lib_test_ConditionalRender);window.$_uq.push(['${page.getComponentId(0)}','universal_lib_test_ConditionalRender',{\"show\":true}]);`)
    view_equals(env, page.getJs(), js.to_view())
}

#universal EventNested(props) {
    return <button onClick={props.onClick}>{props.children}</button>
}

#universal EventParent(props) {
    state clicked = false;
    return <EventNested onClick={() => clicked = true}>
        {clicked ? "Clicked" : "Click Me"}
    </EventNested>
}

@test
public func universal_event_nested_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EventParent /> }
    // Test hydration and event binding through components
}

#universal SpreadAndStaticAttr(props) {
    return <div {...props} id="static" data-val="123"></div>
}

@test
public func universal_spread_and_static_attr_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadAndStaticAttr title="Hello" /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_SpreadAndStaticAttr\"><div title=\"Hello\" id=\"static\" data-val=\"123\"></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal ComplexClassMerge(props) {
    return <div {...props} class={props.active? "active" : "inactive"} class="base"></div>
}

@test
public func universal_complex_class_merge_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ComplexClassMerge active={true} class="extra" /> }
    // Test merging of spread, dynamic class, and static className in JS hydration
}

#universal FragParent(props) {
    return <>
        <Greeting />
        <div>Middle</div>
        <Greeting />
    </>
}

@test
public func universal_frag_parent_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FragParent /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_FragParent\"><div id=\"${page.getComponentId(1)}\" data-u-comp=\"universal_lib_test_Greeting\"><span>Hello</span></div><div>Middle</div><div id=\"${page.getComponentId(2)}\" data-u-comp=\"universal_lib_test_Greeting\"><span>Hello</span></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal ListParent(props) {
    return <ul>
        {props.items.map(item => <Greeting />)}
    </ul>
}

@test
public func universal_list_parent_js(env : &mut TestEnv) {
    // Test list of components JS emission
}

#universal AttrChemValue(props) {
    return <div data-val={props.val}></div>
}

@test
public func universal_attr_chem_value_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    const myVal = 42;
    #html { <AttrChemValue val={myVal} /> }
    
    var html = std::string()
    html.append_expr(`<div id=\"${page.getComponentId(0)}\" data-u-comp=\"universal_lib_test_AttrChemValue\"><div data-val=\"42\"></div></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}

#universal EmptyBlock(props) {
    return <div></div>
}

@test
public func universal_empty_block_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyBlock /> }
    // Test empty block emission
}

#universal MultiState(props) {
    state a = 1;
    state b = 2;
    return <div>{a} + {b} = {a.value + b.value}</div>
}

@test
public func universal_multi_state_js(env : &mut TestEnv) {
    // Test multiple state initialization and subscription
}

func dummy_class_1(page : &mut HtmlPage) : *char {
    return "dc_1"
}

func dummy_class_2(page : &mut HtmlPage) : *char {
    return "dc_2"
}

#universal MyButton(props) {
    return <button {...props} class={${dummy_class_1(page)}}>{props.children}</button>
}

#universal MyButtonPrimary(props) {
    return <MyButton {...props} class={${dummy_class_2(page)}}>{props.children}</MyButton>
}

// TODO: doesn't work, because page in the universal component links with function param
// when inlined, the page should be relinked with the variable page present below, which isn't a reference
// so c translation can take a pointer, currently this won't work in llvm anyway
func my_button_primary_html(page : &mut HtmlPage) {
    #html { <MyButtonPrimary class="my_class">Hello</MyButtonPrimary> }
}

@test
public func test_simple_button_works(env : &mut TestEnv) {
    var page = HtmlPage()
    my_button_primary_html(page)
    var html = std::string()
    html.append_expr(`<div id="${page.getComponentId(0)}" data-u-comp="universal_lib_test_MyButtonPrimary"><button class="dc_1 dc_2">Hello</button></div>`)
    view_equals(env, page.getHtml(), html.to_view())
}