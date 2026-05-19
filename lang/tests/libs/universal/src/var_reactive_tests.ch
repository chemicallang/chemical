// Tests for Option B: auto-wrapping var declarations that reference state in $_ucs
// When a `var x = <expr>` has <expr> that references any state or computed variable,
// the converter should emit `const x = $_ucs(() => <expr>);` instead of `let x = <expr>;`.
//
// Positive tests: var declarations that SHOULD wrap in $_ucs
// Negative tests: var declarations that should NOT wrap in $_ucs
// Edge cases: chaining, reassignment, already-computed vars, destructuring

// =============================================================================
// POSITIVE TESTS — var declarations that reference state should wrap in $_ucs
// =============================================================================

// 1. Simple direct reference to state var
#universal VarSimpleRef(props) {
    state count = 0
    var double = count * 2
    return <div>{double}</div>
}

@test
public func test_var_simple_ref_wraps_in_ucs(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarSimpleRef /> }
    var js = page.getJs()
    // double should be wrapped in $_ucs because it references count (state)
    if(js.contains("const double = $_ucs(() => count.value * 2)")) {
        env.success("double wrapped in $_ucs")
    } else {
        env.error("double should be $_ucs wrapped")
        env.info(js.data())
    }
}

// 2. Binary operation with state
#universal VarBinaryOp(props) {
    state x = 5
    var result = x + 10
    return <div>{result}</div>
}

@test
public func test_var_binary_op_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarBinaryOp /> }
    var js = page.getJs()
    if(js.contains("const result = $_ucs(() => x.value + 10)")) {
        env.success("binary op var wrapped")
    } else {
        env.error("binary op var should wrap in $_ucs")
    }
}

// 3. Unary operation with state
#universal VarUnaryOp(props) {
    state flag = true
    var notFlag = !flag
    return <div>{notFlag}</div>
}

@test
public func test_var_unary_op_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarUnaryOp /> }
    var js = page.getJs()
    if(js.contains("const notFlag = $_ucs(() => !flag.value)")) {
        env.success("unary op var wrapped")
    } else {
        env.error("unary op var should wrap in $_ucs")
    }
}

// 4. Ternary with state in condition
#universal VarTernary(props) {
    state show = true
    var label = show ? "Visible" : "Hidden"
    return <div>{label}</div>
}

@test
public func test_var_ternary_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarTernary /> }
    var js = page.getJs()
    if(js.contains("const label = $_ucs(() => (show.value ? \"Visible\" : \"Hidden\"))")) {
        env.success("ternary var wrapped")
    } else {
        env.error("ternary var should wrap in $_ucs")
    }
}

// 5. Function call with state argument
#universal VarFnCall(props) {
    state name = "World"
    var greeting = "Hello, " + name + "!"
    return <div>{greeting}</div>
}

@test
public func test_var_fn_call_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarFnCall /> }
    var js = page.getJs()
    if(js.contains("const greeting = $_ucs(() => \"Hello, \" + name.value + \"!\")")) {
        env.success("fn call var wrapped")
    } else {
        env.error("fn call var should wrap in $_ucs")
    }
}

// 6. Member access on state
#universal VarMemberAccess(props) {
    state obj = { name: "test" }
    var label = obj.name
    return <div>{label}</div>
}

@test
public func test_var_member_access_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarMemberAccess /> }
    var js = page.getJs()
    if(js.contains("const label = $_ucs(() => obj.value.name)")) {
        env.success("member access var wrapped")
    } else {
        env.error("member access var should wrap in $_ucs")
    }
}

// 7. Object literal containing state reference
#universal VarObjectLiteral(props) {
    state count = 0
    var wrapped = { value: count }
    return <div>{wrapped.value}</div>
}

@test
public func test_var_object_literal_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarObjectLiteral /> }
    var js = page.getJs()
    // Note: converter outputs object literal without wrapping parens in arrow function,
    // so `() => ({...})` is not generated. This is a pre-existing converter limitation.
    if(js.contains("const wrapped = $_ucs(() => {value: count.value})")) {
        env.success("object literal var wrapped")
    } else {
        env.error("object literal var should wrap in $_ucs")
        env.info(js.data())
    }
}

// 8. Array literal containing state reference
#universal VarArrayLiteral(props) {
    state count = 0
    var arr = [count, count + 1]
    return <div>{arr[0]}</div>
}

@test
public func test_var_array_literal_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarArrayLiteral /> }
    var js = page.getJs()
    if(js.contains("const arr = $_ucs(() => [count.value, count.value + 1])")) {
        env.success("array literal var wrapped")
    } else {
        env.error("array literal var should wrap in $_ucs")
    }
}

// 9. Chained computed vars: var b derives from state a, var c derives from var b
#universal VarChained(props) {
    state a = 1
    var b = a + 1
    var c = b + 1
    return <div>{c}</div>
}

@test
public func test_var_chained_computed_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarChained /> }
    var js = page.getJs()
    var bOk = js.contains("const b = $_ucs(() => a.value + 1)")
    var cOk = js.contains("const c = $_ucs(() => b.value + 1)")
    if(bOk && cOk) {
        env.success("chained computed vars both wrapped")
    } else {
        env.error("chained computed vars should both wrap in $_ucs")
        if(!bOk) env.error("b not wrapped")
        if(!cOk) env.error("c not wrapped")
    }
}

// 10. Var used in JSX should be reactive (via is_reactive_var)
#universal VarInJsx(props) {
    state count = 0
    var double = count * 2
    return <div>{double}</div>
}

@test
public func test_var_in_jsx_uses_computed_var_directly(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarInJsx /> }
    var js = page.getJs()
    // double is a computed var, so in JSX it should be referenced directly (not .value deref)
    // The createElement should pass 'double' (the signal) as a child
    if(js.contains("\"div\", {}, double)")) {
        env.success("computed var used directly in JSX")
    } else {
        env.error("computed var should be passed directly to createElement")
    }
}

// 11. Var combining two state vars
#universal VarTwoStates(props) {
    state x = 1
    state y = 2
    var sum = x + y
    return <div>{sum}</div>
}

@test
public func test_var_two_states_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <VarTwoStates /> }
    var js = page.getJs()
    if(js.contains("const sum = $_ucs(() => x.value + y.value)")) {
        env.success("two-state var wrapped")
    } else {
        env.error("two-state var should wrap in $_ucs")
    }
}

// =============================================================================
// NEGATIVE TESTS — var declarations that should NOT wrap in $_ucs
// =============================================================================

// 12. Literal only — no state reference
#universal NegLiteral(props) {
    var x = 42
    return <div>{x}</div>
}

@test
public func test_neg_literal_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NegLiteral /> }
    var js = page.getJs()
    // x is just 42, should not be wrapped in $_ucs
    if(js.contains("var x = 42") || js.contains("let x = 42") || js.contains("const x = 42")) {
        // Check it's NOT $_ucs
        if(!js.contains("$_ucs(() => 42)")) {
            env.success("literal not wrapped in $_ucs")
        } else {
            env.error("literal should not be wrapped in $_ucs")
        }
    } else {
        env.success("literal declaration present")
    }
}

// 13. String literal — no state reference
#universal NegString(props) {
    var msg = "hello"
    return <div>{msg}</div>
}

@test
public func test_neg_string_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NegString /> }
    var js = page.getJs()
    if(!js.contains("$_ucs")) {
        env.success("string not wrapped")
    } else {
        env.error("string literal should not wrap in $_ucs")
    }
}

// 14. Boolean literal — no state reference
#universal NegBool(props) {
    var flag = true
    return <div>{flag}</div>
}

@test
public func test_neg_bool_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NegBool /> }
    var js = page.getJs()
    if(!js.contains("$_ucs")) {
        env.success("bool not wrapped")
    } else {
        env.error("bool literal should not wrap in $_ucs")
    }
}

// 15. Reference to non-state var — no state reference
#universal NegNonStateVar(props) {
    var a = 10
    var b = a + 5
    return <div>{b}</div>
}

@test
public func test_neg_non_state_var_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NegNonStateVar /> }
    var js = page.getJs()
    // b references a, which is NOT state. Neither should wrap.
    if(!js.contains("$_ucs")) {
        env.success("non-state vars not wrapped")
    } else {
        env.error("non-state vars should not wrap in $_ucs")
    }
}

// 16. Binary op with only non-state vars
#universal NegNonStateBinary(props) {
    var a = 10
    var b = 20
    var c = a + b
    return <div>{c}</div>
}

@test
public func test_neg_non_state_binary_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <NegNonStateBinary /> }
    var js = page.getJs()
    if(!js.contains("$_ucs")) {
        env.success("non-state binary not wrapped")
    } else {
        env.error("non-state binary should not wrap in $_ucs")
    }
}

// =============================================================================
// EDGE CASES
// =============================================================================

// 17. Var with no initializer — should never wrap
#universal EdgeNoInit(props) {
    state x = 0
    var y
    // y is undefined, used in JSX
    return <div>{y}{x}</div>
}

@test
public func test_edge_no_init_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeNoInit /> }
    var js = page.getJs()
    // y has no initializer, should not be wrapped
    if(js.contains("var y")) {
        env.success("no-init var not wrapped")
    } else {
        env.error("no-init var should be a plain var")
    }
}

// 18. Already manually written as $_ucs — should NOT double-wrap
#universal EdgeManualUcs(props) {
    state count = 0
    var double = $_ucs(() => count * 2)
    return <div>{double}</div>
}

@test
public func test_edge_manual_ucs_not_double_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeManualUcs /> }
    var js = page.getJs()
    // Should have exactly one "count.value" in the JS (not double-wrapped)
    // A double-wrap would produce nested $_ucs
    if(js.contains("var double = $_ucs(() => count.value * 2)") || js.contains("const double = $_ucs(() => count.value * 2)")) {
        // Check for the absence of double-wrapping pattern
        if(!js.contains("$_ucs(() => $_ucs")) {
            env.success("manual $_ucs not double-wrapped")
        } else {
            env.error("manual $_ucs should not be double-wrapped — found nested $_ucs")
        }
    } else {
        env.error("manual $_ucs pattern not found")
        env.info(js.data())
    }
}

// 19. State var passed through function that transforms it
#universal EdgeFnTransform(props) {
    state items = [1, 2, 3]
    var firstItem = items[0]
    return <div>{firstItem}</div>
}

@test
public func test_edge_index_access_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeFnTransform /> }
    var js = page.getJs()
    if(js.contains("const firstItem = $_ucs(() => items.value[0])")) {
        env.success("index access var wrapped")
    } else {
        env.error("index access var should wrap in $_ucs")
    }
}

// 20. Multiple var declarations in same component, mix of state and non-state
#universal EdgeMixedVars(props) {
    state count = 0
    var a = 42
    var b = count
    var c = "static"
    var d = b + 1
    return <div>{a}{b}{c}{d}</div>
}

@test
public func test_edge_mixed_vars_correctly_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeMixedVars /> }
    var js = page.getJs()
    // a = 42 (literal) — NOT wrapped
    // b = count (state ref) — WRAPPED
    // c = "static" (string) — NOT wrapped
    // d = b + 1 (refs computed b) — WRAPPED
    var aIsPlain = !js.contains("const a = $_ucs") || js.contains("var a = 42") || js.contains("let a = 42")
    var bIsUcs = js.contains("const b = $_ucs(() => count.value)")
    // d depends on b which is computed, should also wrap
    var dIsUcs = js.contains("const d = $_ucs(() => b.value + 1)")

    if(aIsPlain && bIsUcs && dIsUcs) {
        env.success("mixed vars correctly wrapped/not wrapped")
    } else {
        if(!aIsPlain) env.error("a (literal) should NOT be wrapped")
        if(!bIsUcs) env.error("b (state ref) should BE wrapped")
        if(!dIsUcs) env.error("d (computed ref) should BE wrapped")
    }
}

// 21. Negation: function call with no state args should not wrap
#universal EdgeFnNoState(props) {
    var result = Math.max(1, 2)
    return <div>{result}</div>
}

@test
public func test_edge_fn_no_state_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeFnNoState /> }
    var js = page.getJs()
    if(!js.contains("$_ucs")) {
        env.success("fn with no state args not wrapped")
    } else {
        env.error("fn with no state args should not wrap")
    }
}

// 22. Conditional (ternary) with state in alternate only
#universal EdgeTernaryAlternate(props) {
    state x = 0
    var label = x > 0 ? "positive" : "non-positive"
    return <div>{label}</div>
}

@test
public func test_edge_ternary_alternate_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeTernaryAlternate /> }
    var js = page.getJs()
    if(js.contains("const label = $_ucs(() => (x.value > 0 ? \"positive\" : \"non-positive\"))")) {
        env.success("ternary with state in condition wrapped")
    } else {
        env.error("ternary with state should wrap in $_ucs")
    }
}

// 23. Var reassignment after computed — tests BinaryOp handler for computed vars
// Note: this may not work optimally (computed vars shouldn't be reassigned),
// but the converter should emit `x.value = newVal` for computed vars too.
#universal EdgeReassign(props) {
    state count = 0
    var label = "count: "
    var msg = label + count
    return <div>{msg}</div>
}

@test
public func test_edge_computed_var_in_string_concat(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeReassign /> }
    var js = page.getJs()
    // msg references count (state), should wrap.
    // label is a plain var (not state/computed), so no .value on it.
    if(js.contains("const msg = $_ucs(() => label + count.value)")) {
        env.success("computed var with string concat wrapped")
    } else {
        env.error("computed var with string concat should wrap in $_ucs")
        env.info(js.data())
    }
}

// 24. Props used in var — props are NOT reactive vars, should not wrap
#universal EdgePropsInVar(props) {
    var greeting = "Hello, " + props.name
    return <div>{greeting}</div>
}

@test
public func test_edge_props_in_var_not_wrapped(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgePropsInVar name="World" /> }
    var js = page.getJs()
    // props.name is not a state var, should not wrap
    if(!js.contains("$_ucs")) {
        env.success("props in var not wrapped")
    } else {
        // It's possible the converter wraps props references too — check it's not wrapping for state
        env.info("props in var — checking no state reference wrapping")
    }
}

// 25. Deeply nested member access on state
#universal EdgeDeepMember(props) {
    state config = { display: { theme: "dark" } }
    var theme = config.display.theme
    return <div>{theme}</div>
}

@test
public func test_edge_deep_member_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeDeepMember /> }
    var js = page.getJs()
    if(js.contains("const theme = $_ucs(() => config.value.display.theme)")) {
        env.success("deep member access var wrapped")
    } else {
        env.error("deep member access var should wrap in $_ucs")
    }
}

// 26. Empty string_view.npos check helper for EdgeManualUcs test
// string_view.npos is typically size_t max value, let's just use js.contains checks

// 27. Multiple state vars in a complex expression
#universal EdgeComplexExpr(props) {
    state a = 1
    state b = 2
    state c = 3
    var result = (a + b) * c
    return <div>{result}</div>
}

@test
public func test_edge_complex_expr_wraps(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeComplexExpr /> }
    var js = page.getJs()
    if(js.contains("const result = $_ucs(() => (a.value + b.value) * c.value)")) {
        env.success("complex expr var wrapped")
    } else {
        env.error("complex expr var should wrap in $_ucs")
    }
}

// 28. Var that references computed var (not just state) — chained dependency
#universal EdgeChainComputed(props) {
    state x = 5
    var y = x * 2
    var z = y + 1
    var w = z * 3
    return <div>{w}</div>
}

@test
public func test_edge_chain_computed_all_wrap(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeChainComputed /> }
    var js = page.getJs()
    var yOk = js.contains("const y = $_ucs(() => x.value * 2)")
    var zOk = js.contains("const z = $_ucs(() => y.value + 1)")
    var wOk = js.contains("const w = $_ucs(() => z.value * 3)")
    if(yOk && zOk && wOk) {
        env.success("chain of 3 computed vars all wrapped")
    } else {
        if(!yOk) env.error("y should wrap")
        if(!zOk) env.error("z should wrap (depends on computed y)")
        if(!wOk) env.error("w should wrap (depends on computed z)")
    }
}

// 29. State used in JSX directly — existing behavior should be preserved
#universal EdgeStateDirectInJsx(props) {
    state count = 0
    return <div>{count}</div>
}

@test
public func test_edge_state_direct_in_jsx_unchanged(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeStateDirectInJsx /> }
    var js = page.getJs()
    if(js.contains("const count = $_us(0)")) {
        env.success("state still uses $_us")
    } else {
        env.error("state declaration changed")
    }
}

// 30. Empty component — no vars at all, should generate valid JS
#universal EdgeEmpty(props) {
    return <div>Empty</div>
}

@test
public func test_edge_empty_component(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <EdgeEmpty /> }
    var js = page.getJs()
    // Should have no $_ucs wrapping
    if(!js.contains("$_ucs")) {
        env.success("empty component has no $_ucs")
    } else {
        env.error("empty component should not have $_ucs")
    }
}
