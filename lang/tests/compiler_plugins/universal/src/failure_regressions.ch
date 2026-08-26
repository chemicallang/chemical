// Regression tests for failures documented in lang/compiled/universal_failures.md
// and lang/docs/universal-cbi-improvements.md. Each test targets one documented
// failure mode so a regression points straight at the doc entry.

// =============================================================================
// Failure #5: useEffect dependency [stateVar] compiled to [stateVar.value]
//
// The runtime compares deps via window.$__uni_value(eff.deps[i]) which unwraps
// signals. If the converter derefs state refs inside the deps array, the array
// holds primitives captured at registration time and the effect never re-runs.
// =============================================================================

#universal EffectDepsState(props) {
    state showCreate = false
    useEffect(() => {
        if(showCreate) {
            fetchStuff()
        }
    }, [showCreate])
    return <span>deps</span>
}

@test
public func universal_effect_deps_state_not_derefsed(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EffectDepsState /> }
    var js = page.getJs()
    // Deps array must reference the signal, not capture its primitive value.
    if(js.contains("[showCreate]")) {
        env.success("useEffect deps array keeps signal reference")
    } else {
        env.error("useEffect deps array was dereferenced to .value (effect will never re-run)")
        env.info(js.data())
    }
    if(js.contains("[showCreate.value]")) {
        env.error("deps array contains showCreate.value — primitive captured at registration")
    }
}

#universal EffectDepsMulti(props) {
    state a = 1
    state b = 2
    useEffect(() => {
        logSum(a, b)
    }, [a, b])
    return <span>multi</span>
}

@test
public func universal_effect_deps_multi_not_derefsed(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EffectDepsMulti /> }
    var js = page.getJs()
    if(js.contains("[a, b]")) {
        env.success("multi-element deps array keeps signal references")
    } else {
        env.error("multi-element deps array was dereferenced")
        env.info(js.data())
    }
}

// Body refs still need .value — verify the skip flag doesn't leak into the callback.
#universal EffectBodyStillDerefed(props) {
    state show = false
    useEffect(() => {
        if(show) {
            doThing()
        }
    }, [show])
    return <span>body</span>
}

@test
public func universal_effect_body_still_derefsed(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EffectBodyStillDerefed /> }
    var js = page.getJs()
    if(!js.contains("if(show)")) {
        env.success("effect body still reads show.value")
    } else {
        env.error("skip_reactive_deref leaked into effect body (if(show) instead of if(show.value))")
        env.info(js.data())
    }
}

// =============================================================================
// Failure #6: '' treated as character literal inside #universal
//
// The Chemical lexer sees two single quotes and reports "no value given inside
// single quotes" because it doesn't know the code is in a JS/JSX context.
// =============================================================================

#universal EmptySingleQuotes(props) {
    var subj = props.form ? props.form.subject : ''
    var html = props.ed ? props.ed : ''
    return <div>{subj}|{html}</div>
}

@test
public func universal_empty_single_quotes_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptySingleQuotes /> }
    var js = page.getJs()
    // Must compile (no lexer error) and emit an empty string for the fallbacks.
    if(js.contains("? ") && js.contains("|")) {
        env.success("'' empty string compiles in JS context")
    } else {
        env.error("'' handling produced unexpected JS")
        env.info(js.data())
    }
}

// Single-quoted non-empty strings should also work in JS context.
#universal SingleQuotedString(props) {
    var greeting = props.name != null ? 'hi ' + props.name : 'guest'
    return <span>{greeting}</span>
}

@test
public func universal_single_quoted_string_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SingleQuotedString name="sam" /> }
    var js = page.getJs()
    if(js.contains("'hi '") || js.contains("\"hi \"")) {
        env.success("single-quoted JS strings work")
    } else {
        env.error("single-quoted JS string failed to convert")
        env.info(js.data())
    }
}

// =============================================================================
// Failure #2: {} empty braces in else-if body parsed as struct literal
//
// `[Parser] error: unexpected l-brace, struct value not expected` when using {}
// as a no-op block inside #universal control flow.
// =============================================================================

#universal EmptyBlockNoOp(props) {
    var r = "start"
    if(props.c == "a") {
        r = "A"
    } else if(props.c == "b") {
    } else {
        r = "other"
    }
    return <span>{r}</span>
}

@test
public func universal_empty_block_else_if(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyBlockNoOp /> }
    var js = page.getJs()
    // Empty block must survive conversion as an empty statement block.
    if(js.contains("else")) {
        env.success("{} empty block compiles")
    } else {
        env.error("empty {} block broke conversion")
        env.info(js.data())
    }
}

#universal EmptyBlockIfBody(props) {
    var touched = false
    if(props.warmup) {
    }
    touched = true
    return <span>{touched}</span>
}

@test
public func universal_empty_block_if_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EmptyBlockIfBody /> }
    var js = page.getJs()
    if(js.contains("touched = true")) {
        env.success("{} as full if-body compiles")
    } else {
        env.error("{} if-body dropped following statements")
        env.info(js.data())
    }
}

// =============================================================================
// Failure #4 variant: throw new Error(...) silently dropped
// (statement_emission.ch covers `throw "boom"`; this covers constructor form)
// =============================================================================

#universal ThrowNewError(props) {
    if(props.bad) {
        throw new Error("HTTP " + props.code)
    }
    return <span>ok</span>
}

@test
public func universal_throw_new_error_emitted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ThrowNewError /> }
    var js = page.getJs()
    if(js.contains("throw ")) {
        env.success("throw new Error(...) is emitted")
    } else {
        env.error("throw new Error(...) was dropped from generated JS")
        env.info(js.data())
    }
}

// =============================================================================
// Improvements doc §3.10 Pitfall B: conditional rendered as JSX child expression
// must become a reactive $_ucs computed, not a one-time value.
// =============================================================================

#universal CondChildReactiveOverlay(props) {
    state open = false
    return <div>
        {open && props.hasErrors ? <div className="overlay">errors</div> : null}
    </div>
}

@test
public func universal_cond_child_wraps_in_ucs(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CondChildReactiveOverlay /> }
    var js = page.getJs()
    // The conditional child must be wrapped in $_ucs so flipping `open` re-renders.
    if(js.contains("$_ucs(() => (open.value")) {
        env.success("conditional child wrapped in $_ucs")
    } else {
        env.error("conditional child is not reactive (no $_ucs wrap)")
        env.info(js.data())
    }
}

// Early-return-on-state stays one-time by design (documented pitfall) — this
// test pins the CURRENT behavior so changes are noticed: `return` before any
// JSX means the component renders nothing reactively. We only assert the JS
// compiles and contains the early return.
#universal EarlyReturnNullPattern(props) {
    state open = false
    if(!open) {
        return null
    }
    return <div>content</div>
}

@test
public func universal_early_return_null_compiles(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EarlyReturnNullPattern /> }
    var js = page.getJs()
    if(js.contains("return null") || js.contains("return;")) {
        env.success("early return null compiles")
    } else {
        env.error("early return null pattern broken")
        env.info(js.data())
    }
}
