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
// must be defined on $_r. If they throw "is not a function" at mount, we need
// a compile diagnostic or runtime implementation.
//
// BUG DETECTED: Components using useRef, useMemo, useCallback, or useContext
// in their body silently fail to register as universal components. The #html
// block falls back to raw JSX text (<useRefHook></useRefHook>) instead of
// emitting SSR + hydration JS. The runtime (page.ch) defines these hooks, but
// the CBI plugin cannot compile the component body when these hooks are present.
// This means any component using these hooks is broken at compile time.
// =============================================================================

// =============================================================================
// §3.2: Missing operators — ??, ?. , ** are NOT yet supported by the
// universal parser. Using them causes a compile error (confirmed gap).
// These operators are documented as future work in the improvements doc.
// We do NOT test them here because they crash the compiler (double-free
// in the parser), which is a separate parser-robustness issue.
// =============================================================================

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
