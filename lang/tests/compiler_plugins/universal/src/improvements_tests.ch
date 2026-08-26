// Tests targeting remaining items from lang/docs/universal-cbi-improvements.md
// that are not covered by existing test files.
using namespace std;

// =============================================================================
// §3.6: List key prop — key must survive into the generated JS so the runtime
// can do keyed reconciliation. Positional hydration breaks on sort/filter.
// =============================================================================

#universal KeyedList(props) {
    state items = [{name: "a"}, {name: "b"}, {name: "c"}]
    return <ul>
        {items.map((item, i) => <li key={item.name}>{item.name}</li>)}
    </ul>
}

@test
public func universal_key_prop_in_map_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <KeyedList /> }
    var js = page.getJs()
    if(js.contains("key")) {
        env.success("key prop is present in generated JS")
    } else {
        env.error("key prop was lost during conversion")
        env.info(js.data())
    }
}

// =============================================================================
// §3.5: Hooks surface — useRef, useMemo, useCallback, useReducer, useContext
// must be defined on $_r. If they throw "is not a function" at mount, we need
// a compile diagnostic or runtime implementation.
// =============================================================================

// =============================================================================
// §3.5: Hooks surface — useRef, useMemo, useCallback, useReducer, useContext
// must be defined on $_r. Components that use these hooks must compile and
// produce valid SSR + hydration JS. The hooks are rewritten to $_r.* by the
// converter.
//
// NOTE: These tests verify the hooks are REWRITTEN in the JS output (e.g.
// useRef → $_r.useRef). If the component silently fails to register, the
// #html block outputs raw JSX text, which is a separate bug.
// =============================================================================

#universal UseRefHook(props) {
    var inputRef = useRef(null)
    return <span>{inputRef.current}</span>
}

@test
public func universal_use_ref_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UseRefHook /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_r.useRef")) {
        env.success("useRef hook rewritten to $_r.useRef in JS output")
    } else {
        env.error("useRef hook was not rewritten to $_r.useRef")
        env.info(js.data())
    }
}

#universal UseMemoHook(props) {
    var computed = useMemo(() => props.x * 2)
    return <span>{computed}</span>
}

@test
public func universal_use_memo_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UseMemoHook x={5} /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_r.useMemo")) {
        env.success("useMemo hook rewritten to $_r.useMemo in JS output")
    } else {
        env.error("useMemo hook was not rewritten to $_r.useMemo")
        env.info(js.data())
    }
}

#universal UseCallbackHook(props) {
    var handler = useCallback(() => { doSomething() })
    return <button onClick={handler}>click</button>
}

@test
public func universal_use_callback_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UseCallbackHook /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_r.useCallback")) {
        env.success("useCallback hook rewritten to $_r.useCallback in JS output")
    } else {
        env.error("useCallback hook was not rewritten to $_r.useCallback")
        env.info(js.data())
    }
}

#universal UseContextHook(props) {
    var theme = useContext("theme")
    return <div className={theme}>content</div>
}

@test
public func universal_use_context_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <UseContextHook /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("$_r.useContext")) {
        env.success("useContext hook rewritten to $_r.useContext in JS output")
    } else {
        env.error("useContext hook was not rewritten to $_r.useContext")
        env.info(js.data())
    }
}

// =============================================================================
// §3.2: Operators — ??, ?. , ** were previously missing from the universal
// parser, causing a double-free crash. They now have token support and parse
// correctly. Test that they compile and produce valid JS output.
// =============================================================================

#universal NullishCoalescing(props) {
    var name = props.name ?? "anonymous"
    return <span>{name}</span>
}

@test
public func universal_nullish_coalescing_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NullishCoalescing /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("??")) {
        env.success("nullish coalescing compiles and emits ??")
    } else {
        env.error("nullish coalescing was not emitted")
        env.info(js.data())
    }
}

#universal OptionalChaining(props) {
    var city = props.user?.address?.city
    return <span>{city}</span>
}

@test
public func universal_optional_chaining_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <OptionalChaining /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("address") && js.contains("city")) {
        env.success("optional chaining compiles and emits member access")
    } else {
        env.error("optional chaining was not emitted")
        env.info(js.data())
    }
}

#universal ExponentOperator(props) {
    var result = props.base ** props.exp
    return <span>{result}</span>
}

@test
public func universal_exponent_operator_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ExponentOperator base={2} exp={3} /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("**")) {
        env.success("exponent operator compiles and emits **")
    } else {
        env.error("exponent operator was not emitted")
        env.info(js.data())
    }
}

// =============================================================================
// §2.7 Problem 3: Unsupported prop types silently become integers.
// A struct prop being passed as UInteger (pointer-sized) is silent corruption.
// Test that passing a component as a prop to another component doesn't
// silently break.
// =============================================================================

#universal InnerCard(props) {
    return <div>{props.label}</div>
}

#universal OuterCard(props) {
    return <div>
        <InnerCard label={props.title} />
    </div>
}

@test
public func universal_nested_component_prop_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <OuterCard title="Hello" /> }
    var html = page.getHtml()
    if(html.contains("Hello")) {
        env.success("nested component receives string prop correctly")
    } else {
        env.error("nested component prop was not passed through")
        env.info(html.data())
    }
}

// =============================================================================
// §2.5: SSR of state-derived children — state initializer arrays should be
// constant-folded into SSR HTML. Test that a static array in state produces
// rendered children, not an empty shell.
// =============================================================================

#universal StaticStateList(props) {
    state todos = ["Walk the dog", "Buy groceries", "Read a book"]
    return <ul>
        {todos.map((t, i) => <li key={i}>{t}</li>)}
    </ul>
}

@test
public func universal_static_state_list_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StaticStateList /> }
    var html = page.getHtml()
    // The SSR HTML should contain the actual todo items, not be empty.
    if(html.contains("Walk the dog") && html.contains("Buy groceries") && html.contains("Read a book")) {
        env.success("static state array is folded into SSR HTML")
    } else {
        env.error("static state array was NOT SSR'd (empty shell)")
        env.info(html.data())
    }
}

#universal TernaryOnState(props) {
    state isOn = false
    return <div>
        {isOn ? <span>ON</span> : <span>OFF</span>}
    </div>
}

@test
public func universal_ternary_on_state_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <TernaryOnState /> }
    var html = page.getHtml()
    // The SSR HTML should contain "OFF" since isOn starts as false.
    if(html.contains("OFF")) {
        env.success("ternary on state is SSR'd with initial value")
    } else {
        env.error("ternary on state was NOT SSR'd (empty content)")
        env.info(html.data())
    }
}

// =============================================================================
// §3.7: Error isolation — if one component throws during mount, the rest of
// the hydration queue should not be aborted. Test that components after a
// failing one still mount (this is a runtime concern; we test the generated
// JS structure here).
// =============================================================================

#universal SafeComponent(props) {
    return <span>{props.text}</span>
}

@test
public func universal_multiple_components_in_html(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <SafeComponent text="first" />
        <SafeComponent text="second" />
        <SafeComponent text="third" />
    }
    var html = page.getHtml()
    var js = page.getJs()
    // All three components should produce dispatch calls in JS.
    var dispatchCount = 0
    if(js.contains("first")) dispatchCount++
    if(js.contains("second")) dispatchCount++
    if(js.contains("third")) dispatchCount++
    if(dispatchCount >= 3) {
        env.success("multiple components produce dispatch calls")
    } else {
        env.error("not all components produced dispatch calls")
        env.info(js.data())
    }
}

// =============================================================================
// §3.10 Pitfall A: #css style helpers called inside universal component bodies
// crash at hydration because they need the SSR page pointer. Test that inline
// style={{ }} objects work as a replacement.
// =============================================================================

#universal InlineStyleComp(props) {
    return <div style={{color: "red", fontSize: "14px"}}>styled</div>
}

@test
public func universal_inline_style_object_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <InlineStyleComp /> }
    var html = page.getHtml()
    if(html.contains("color") && html.contains("red")) {
        env.success("inline style object renders in SSR HTML")
    } else {
        env.error("inline style object was not rendered in SSR")
        env.info(html.data())
    }
}

// =============================================================================
// Test that numeric props render correctly in SSR (not as raw pointer values).
// =============================================================================

#universal NumericPropsComp(props) {
    return <div data-count={props.count} data-ratio={props.ratio}>num</div>
}

@test
public func universal_numeric_props_render_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NumericPropsComp count={42} ratio={3.14} /> }
    var html = page.getHtml()
    if(html.contains("42") && html.contains("3.14")) {
        env.success("numeric props render in SSR HTML")
    } else {
        env.error("numeric props did not render correctly")
        env.info(html.data())
    }
}

// =============================================================================
// Test boolean and null attribute values in SSR — they should render as
// proper HTML, not as raw JS booleans or the string "null".
// =============================================================================

#universal BoolAttrComp(props) {
    return <input disabled={props.off} />
}

@test
public func universal_bool_attr_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BoolAttrComp off={true} /> }
    var html = page.getHtml()
    // Boolean true for disabled should appear as the attribute value.
    if(html.contains("disabled")) {
        env.success("boolean attribute renders in SSR HTML")
    } else {
        env.error("boolean attribute was not rendered")
        env.info(html.data())
    }
}
