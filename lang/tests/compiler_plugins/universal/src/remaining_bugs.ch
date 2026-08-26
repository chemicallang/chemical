// Tests for remaining bugs documented in lang/docs/universal-cbi-improvements.md
// These bugs were identified but had no test coverage.

// =============================================================================
// §2.7 P3: Unsupported prop types silently become integers.
// When a prop value has a type that can't be converted to SsrAttributeValue
// (e.g., a struct), the converter falls through to wrapArgAttrValueVariantCall("UInteger", value).
// This silently corrupts the JS bundle — a struct prop becomes a pointer-sized number.
//
// The fix: emit a compile diagnostic for unsupported prop types instead of
// silently wrapping as UInteger. At minimum, test that function-typed props
// and object-typed props are handled correctly.
// =============================================================================

#universal FuncPropComp(props) {
    return <button onClick={props.onClick}>click</button>
}

@test
public func universal_func_prop_not_corrupted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FuncPropComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The onClick prop should be passed as a function reference in the dispatch
    // props, not coerced to an integer.
    if(js.contains("onClick")) {
        env.success("function prop is present in JS output")
    } else {
        env.error("function prop was lost or corrupted")
        env.info(js.data())
    }
}

// =============================================================================
// §2.7 P4: SpecialAttrs fixed arrays overflow.
// SpecialAttrs in ssr.ch uses fixed [32]/[32]/[64] arrays with bounds checks
// (if count < 32/64). Already fixed — attributes beyond the limit are silently
// dropped rather than overflowing. Test that a component with many attributes
// compiles without crashing.
// =============================================================================

#universal ManyAttrsComp(props) {
    return <div a1={props.v} a2={props.v} a3={props.v} a4={props.v} a5={props.v}
               a6={props.v} a7={props.v} a8={props.v} a9={props.v} a10={props.v}
               a11={props.v} a12={props.v} a13={props.v} a14={props.v} a15={props.v}
               a16={props.v} a17={props.v} a18={props.v} a19={props.v} a20={props.v}
               a21={props.v} a22={props.v} a23={props.v} a24={props.v} a25={props.v}
               a26={props.v} a27={props.v} a28={props.v} a29={props.v} a30={props.v}
               a31={props.v} a32={props.v} a33={props.v} a34={props.v} a35={props.v}
               a36={props.v} a37={props.v} a38={props.v} a39={props.v} a40={props.v}
               a41={props.v} a42={props.v} a43={props.v} a44={props.v} a45={props.v}
               a46={props.v} a47={props.v} a48={props.v} a49={props.v} a50={props.v}
               a51={props.v} a52={props.v} a53={props.v} a54={props.v} a55={props.v}
               a56={props.v} a57={props.v} a58={props.v} a59={props.v} a60={props.v}
               a61={props.v} a62={props.v} a63={props.v} a64={props.v} a65={props.v}
               >many</div>
}

@test
public func universal_many_attrs_compile(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ManyAttrsComp v="x" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // Component should compile and render without stack overflow.
    // If the component didn't register, the HTML contains raw JSX.
    if(html.contains("ManyAttrsComp")) {
        env.error("component with >64 attrs did not register (possible overflow)")
        env.info(html.data())
    } else {
        env.success("component with >64 attrs compiled and rendered")
    }
}

// =============================================================================
// §3.10 Pitfall A: #css style helpers crash at hydration.
// A #css { } helper takes the SSR page pointer. On the client, the component
// is invoked as factory(props) with no page argument, so calling the helper
// throws during hydration.
//
// The fix: either reject #css usage inside #universal components, or make
// #css a no-op that returns a stable class name on the client. Test that
// inline style={{ }} objects work as the recommended replacement.
// =============================================================================

#universal InlineStyleVsCssComp(props) {
    return <div style={{color: props.color, fontSize: "14px"}}>styled</div>
}

@test
public func universal_inline_style_not_corrupted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <InlineStyleVsCssComp color="red" /> }
    var html = std::string()
    html.append_view(page.getHtml())
    var js = std::string()
    js.append_view(page.getJs())
    // The inline style should render in SSR HTML as a style attribute
    // with CSS property format (e.g. style="color: red; font-size: 14px")
    if(html.contains("style=") && html.contains("font-size")) {
        env.success("inline style object renders in SSR")
    } else {
        env.error("inline style object was not rendered in SSR")
        env.info(html.data())
    }
    // The JS should not reference any #css helper function
    if(js.contains("#css") || js.contains("css_helper")) {
        env.error("JS references #css helper (will crash at hydration)")
        env.info(js.data())
    } else {
        env.success("JS does not reference #css helper")
    }
}

// =============================================================================
// §2.6 P4: No error isolation in hydration dispatch.
// $__uni_dispatch / $__universal_flush have no try/catch; a single failing
// component aborts the hydration queue for the rest of the page.
//
// NOTE: try/catch in emitted JS causes performance regression. Error isolation
// must be implemented in the runtime (defaultUniversalSetup) or via a separate
// error-boundary component, NOT in the emitted dispatch calls. This test
// documents the current behavior as a known limitation.
// =============================================================================

@test
public func universal_dispatch_no_error_isolation(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <SafeComponent text="first" />
        <SafeComponent text="second" />
    }
    var js = std::string()
    js.append_view(page.getJs())
    // Currently dispatch calls have NO try/catch — this is a known limitation.
    // Error isolation must come from the runtime or an error-boundary component.
    if(!js.contains("try")) {
        env.success("confirmed: no try/catch in emitted dispatch (runtime-level isolation needed)")
    } else {
        env.error("unexpected try/catch in emitted dispatch")
        env.info(js.data())
    }
}

// =============================================================================
// §2.5 SSR renders state-derived children empty.
// State initializer arrays should be constant-folded into SSR HTML.
// Test that a .map() over a state array produces SSR output.
// =============================================================================

#universal StateMapSSR(props) {
    state items = ["Alpha", "Beta", "Gamma"]
    return <ul>
        {items.map((item, i) => <li key={i}>{item}</li>)}
    </ul>
}

@test
public func universal_state_map_ssr_not_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <StateMapSSR /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // The SSR HTML should contain the actual items, not be empty.
    if(html.contains("Alpha") && html.contains("Beta") && html.contains("Gamma")) {
        env.success("state array .map() is SSR'd with actual content")
    } else {
        env.error("state array .map() produced empty SSR content")
        env.info(html.data())
    }
}

// =============================================================================
// §3.5 Incomplete hooks surface: useMemo/useCallback/useRef/useContext must
// be rewritten to $_r.* in the JS output. (Verified they compile correctly.)
// =============================================================================

#universal HookRewriteCheck(props) {
    state x = 1
    var memoized = useMemo(() => x * 2)
    var cb = useCallback(() => { foo() })
    return <span>{memoized}</span>
}

@test
public func universal_hooks_rewritten_to_r(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <HookRewriteCheck /> }
    var js = std::string()
    js.append_view(page.getJs())
    var useMemoOk = js.contains("$_r.useMemo")
    var useCallbackOk = js.contains("$_r.useCallback")
    if(useMemoOk && useCallbackOk) {
        env.success("useMemo and useCallback rewritten to $_r.*")
    } else {
        if(!useMemoOk) env.error("useMemo not rewritten to $_r.useMemo")
        if(!useCallbackOk) env.error("useCallback not rewritten to $_r.useCallback")
        env.info(js.data())
    }
}
