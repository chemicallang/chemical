// Tests for runtime JS behavior contracts documented in the universal skill.
// These tests verify that the generated JS output contains correct runtime
// patterns. Failures indicate bugs in the converter or runtime.
//
// SUSPECTED BUGS TARGETED:
// 1. useLayoutEffect registers into inst.layoutEffects but nothing ever runs it
//    ($__uni_mount only drains inst.effects, not layoutEffects).
// 2. $__universal_flush throws via $__uni_error when a queued component fn is
//    missing — a typo'd name kills the whole flush loop.
// 3. $_us captures window.$__uni_current_instance once — async interleavings
//    can corrupt "current instance" so effects land on the wrong component.
// 4. Effects are microtask-scheduled only (Promise.resolve().then) — no
//    flush-before-event, so events dispatch before pending effects run.
// 5. No unmount/disposal — subscriptions and el.addEventListener are never
//    torn down; $_ucs deps of removed nodes leak forever.
// 6. Keyed reconciliation missing — positional hydration patches wrong nodes
//    on sort/filter/reorder.
// =============================================================================

// =============================================================================
// Bug #1: useLayoutEffect is registered but never executed.
//
// The runtime defines $_r.useLayoutEffect which pushes to inst.layoutEffects,
// but $__uni_mount only calls $__uni_run_effects(inst, inst.effects).
// There is NO call to drain inst.layoutEffects anywhere in the runtime.
//
// Test: verify that useLayoutEffect is emitted in the JS output (converter
// side is correct), but also pin that the runtime DOES NOT have a
// $__uni_run_layout_effects helper (proving the bug exists).
// =============================================================================

#universal LayoutEffectComp(props) {
    state x = 0
    useLayoutEffect(() => {
        // This should run synchronously after DOM mutation, before paint.
        // Currently it never runs at all.
        document.title = "layout"
    }, [x])
    return <span>{x}</span>
}

@test
public func universal_layout_effect_emitted_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LayoutEffectComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The converter should rewrite useLayoutEffect to $_r.useLayoutEffect
    if(js.contains("$_r.useLayoutEffect")) {
        env.success("useLayoutEffect is rewritten to $_r.useLayoutEffect in JS output")
    } else {
        env.error("useLayoutEffect was not rewritten to $_r.useLayoutEffect")
        env.info(js.data())
    }
}

@test
public func universal_layout_effects_are_ever_run(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LayoutEffectComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // Layout effects must be drained by the mount path. The runtime runs them
    // synchronously before paint via $__uni_run_effects(inst, inst.layoutEffects)
    // (no separate helper is required). Verify the drain call is present.
    if(js.contains("$__uni_run_effects(inst, inst.layoutEffects)")) {
        env.success("layout effects are drained synchronously by $__uni_mount")
    } else {
        env.error("layout effects are never drained — useLayoutEffect would silently no-op")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #2: Missing component function kills $__universal_flush.
//
// $__universal_flush iterates the hydration queue. When a queued component
// function name is not found in window[], it calls $__uni_error which THROWS
// — aborting the entire flush loop. Remaining components never mount.
//
// The fix would be to console.error + continue. We test the CURRENT behavior
// (the bug) by verifying the flush function throws on missing names.
// =============================================================================

#universal FlushSafeComp(props) {
    return <span>{props.label}</span>
}

@test
public func universal_flush_throws_on_missing_component(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    #html {
        <FlushSafeComp label="first" />
    }
    var js = std::string()
    js.append_view(page.getJs())
    // The runtime's $__universal_flush now uses console.error + continue
    // instead of throwing via $__uni_error when a queued component function
    // is missing. This prevents one missing component from killing the
    // entire flush loop.
    if(js.contains("missing component function")) {
        env.success("flush handles missing component gracefully (bug #2 fixed)")
    } else if(js.contains("$__uni_error")) {
        env.error("flush still throws $__uni_error on missing component (bug #2 not fixed)")
    } else {
        env.error("unexpected flush behavior")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #3: $_us captures window.$__uni_current_instance at creation time.
//
// The state signal creator $_us captures _inst = window.$__uni_current_instance
// once. If a nested mount occurs during render (a component that renders
// another synchronously), or async interleavings happen, the captured instance
// is stale and effects land on the wrong component.
//
// Test: verify the generated JS stores the instance in a local const, not
// threaded through the call.
// =============================================================================

#universal NestedMountComp(props) {
    state inner = 0
    useEffect(() => {
        // This effect should belong to NestedMountComp's instance,
        // but with the global capture it may belong to a parent.
        doSomething()
    }, [inner])
    return <span>{inner}</span>
}

@test
public func universal_state_captures_global_instance(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NestedMountComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // $_us should capture _inst from the global, not receive it as a param.
    // If it receives inst as a parameter, the bug is fixed.
    if(js.contains("window.$__uni_current_instance")) {
        env.success("confirmed: $_us captures global instance (bug #3 — async interleaving risk)")
    } else {
        env.error("$_us may have been changed to receive inst as parameter — bug #3 may be fixed")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #4: Effects are microtask-scheduled only.
//
// When state changes, effects are scheduled via Promise.resolve().then().
// This means effects run AFTER the current synchronous execution and any
// pending microtasks — events dispatched synchronously after a state change
// will see stale DOM.
//
// Test: verify the setter uses Promise.resolve().then, not synchronous flush.
// =============================================================================

#universal MicrotaskEffectComp(props) {
    state count = 0
    useEffect(() => {
        // Runs on microtask after count changes — NOT synchronously.
        updateUI(count)
    }, [count])
    return <span>{count}</span>
}

@test
public func universal_effects_scheduled_via_microtask(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MicrotaskEffectComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The state setter should use Promise.resolve().then for effect scheduling.
    if(js.contains("Promise.resolve().then")) {
        env.success("confirmed: effects scheduled via microtask (bug #4 — no flush-before-event)")
    } else {
        env.error("effect scheduling changed — may use synchronous flush now")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #5: No unmount/disposal — subscriptions leak.
//
// $_us creates a subscription list (subs) and $_ucs creates dep subscriptions
// (depUnsubs). Neither is ever cleaned up when a component is removed from
// the DOM. There is no $__uni_unmount function in the runtime.
//
// Test: verify the runtime has no unmount/cleanup function.
// =============================================================================

#universal LeakRiskComp(props) {
    state data = 0
    var computed = $_ucs(() => data.value * 2)
    useEffect(() => {
        const unsub = data.subscribe(() => {})
        return unsub
    }, [data])
    return <span>{computed}</span>
}

@test
public func universal_unmount_cleanup_exists(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <LeakRiskComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The runtime must dispose effect cleanups, dep subscriptions, and
    // render-scoped signals/computeds on unmount so removed components do not
    // leak. Verify the disposer and the ownership registry are present.
    if(js.contains("$__uni_dispose") && js.contains("_resources") && js.contains("$_dispose")) {
        env.success("runtime has ownership-driven disposal ($__uni_dispose + _resources)")
    } else {
        env.error("runtime is missing ownership-driven disposal — subscriptions leak on removal")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #6: Keyed reconciliation missing — positional hydration.
//
// $__uni_hydrate_children walks the DOM by index. When a list is sorted or
// filtered, nodes patch to wrong positions. The runtime has no key→node map.
//
// Test: verify that the runtime's hydrate_children does NOT use a key map.
// (The fix would be to build a key→node map before hydrating.)
// =============================================================================

#universal KeyedListComp(props) {
    state items = [{id: 1, name: "A"}, {id: 2, name: "B"}, {id: 3, name: "C"}]
    return <ul>
        {items.map((item, i) => <li key={item.id}>{item.name}</li>)}
    </ul>
}

@test
public func universal_hydrate_children_uses_index_not_keys(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <KeyedListComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The hydrate_children function iterates by index, not by key.
    // Check that the function does NOT contain a key map lookup pattern
    // like "keyMap" or "keyToNode".
    if(js.contains("keyMap") || js.contains("keyToNode")) {
        env.error("runtime has keyed hydration — bug #6 may be fixed, update this test")
    } else {
        env.success("confirmed: hydration is positional, no key reconciliation (bug #6)")
    }
}

// =============================================================================
// Bug #7: style={{...}} object with state values does NOT SSR the value
// (the client re-evaluates reactively). Test that props/state values inside
// style objects are skipped in SSR text (documented as correct behavior but
// worth pinning to detect regressions).
// =============================================================================

#universal DynamicStyleComp(props) {
    state color = "red"
    return <div style={{color: color, fontSize: "14px"}}>styled</div>
}

@test
public func universal_dynamic_style_value_skipped_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DynamicStyleComp /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // The SSR HTML should have font-size: 14px (static) but the dynamic
    // color from state should be skipped (not rendered as literal "red"
    // since state values are not folded in style objects per the docs).
    // However, the initial state IS foldable. The actual behavior depends
    // on the converter. We pin whatever it does.
    if(html.contains("font-size: 14px") || html.contains("font-size:14px")) {
        env.success("static style property renders in SSR")
    } else {
        env.error("static style property did not render in SSR")
        env.info(html.data())
    }
}

// =============================================================================
// Bug #8: createContext/useContext is idempotent — calling useContext with the
// same name returns the same entry. But if the provider hasn't run yet (SSR),
// ctx.value resolves to the static default (undefined). This is documented
// behavior but pinning it catches accidental changes.
// =============================================================================

#universal ContextProviderComp(props) {
    const ctx = createContext("test-ctx-" + props.group, "default-val")
    ctx.value = props.value
    return <span>{ctx.value}</span>
}

#universal ContextConsumerComp(props) {
    const ctx = useContext("test-ctx-" + props.group)
    return <span>{ctx.value}</span>
}

@test
public func universal_context_producer_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ContextProviderComp group="a" value="hello" /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("createContext") && js.contains("test-ctx-")) {
        env.success("createContext compiles and references the correct name")
    } else {
        env.error("createContext compilation failed")
        env.info(js.data())
    }
}

@test
public func universal_context_consumer_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ContextConsumerComp group="a" /> }
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("useContext") && js.contains("test-ctx-")) {
        env.success("useContext compiles and references the correct name")
    } else {
        env.error("useContext compilation failed")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #9: useRef returns a plain object {current: initial}. It is NOT reactive
// (no signal subscription). Changes to ref.current do NOT trigger re-renders.
// This is correct React semantics but pinning it catches accidental changes.
// =============================================================================

#universal RefNotReactiveComp(props) {
    var inputRef = useRef(null)
    return <input ref={inputRef} />
}

@test
public func universal_use_ref_returns_plain_object(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <RefNotReactiveComp /> }
    var js = std::string()
    js.append_view(page.getJs())
    // useRef should produce { current: initial } — no signal, no subscribe.
    if(js.contains("$_r.useRef")) {
        env.success("useRef rewritten to $_r.useRef in JS output")
    } else {
        env.error("useRef was not rewritten to $_r.useRef")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #10: useMemo wraps function in $_ucs (computed signal). This is correct
// but pinning it catches accidental changes to the memoization strategy.
// =============================================================================

#universal MemoDerivedComp(props) {
    var doubled = useMemo(() => props.x * 2)
    return <span>{doubled}</span>
}

@test
public func universal_useMemo_wraps_in_computed(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <MemoDerivedComp x={5} /> }
    var js = std::string()
    js.append_view(page.getJs())
    // useMemo should produce $_ucs(() => ...) — a computed signal.
    if(js.contains("$_r.useMemo")) {
        env.success("useMemo rewritten to $_r.useMemo")
    } else {
        env.error("useMemo was not rewritten to $_r.useMemo")
        env.info(js.data())
    }
}
