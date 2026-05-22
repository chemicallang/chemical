// Regression test: return statement followed by another statement on next line
// should not merge them into `return <expr>`. The Chemical syntax treats
// newlines as statement separators, so `return` on its own line is a bare return.

#universal ReturnGuardFetch(props) {
    if(props.cond) return
    fetch()
    return <span>ok</span>
}

@test
public func universal_return_guard_fetch_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <ReturnGuardFetch />
    }
    var str = std::string()
    str.append_expr(`function universal_lib_test_ReturnGuardFetch(props) { if(window.$__uni_value(props.cond)) return; fetch(); return $_ur.createElement("span", {}, ${"` ok `"}); }\nwindow.$__uni_dispatch('universal_lib_test_ReturnGuardFetch', document.getElementById('u${page.getComponentId(0)}'), {});\n`)
    view_equals(env, page.getJs(), str.to_view())
}

#universal ReturnGuardStateAssign(props) {
    state x = 0
    if(props.cond) return
    x = 1
    return <span>{x}</span>
}

@test
public func universal_return_guard_state_assign_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <ReturnGuardStateAssign />
    }
    var str = std::string()
    str.append_expr(`function universal_lib_test_ReturnGuardStateAssign(props) { const x = $_us(0); if(window.$__uni_value(props.cond)) return; x.value = 1; return $_ur.createElement("span", {}, x); }\nwindow.$__uni_dispatch('universal_lib_test_ReturnGuardStateAssign', document.getElementById('u${page.getComponentId(0)}'), {});\n`)
    view_equals(env, page.getJs(), str.to_view())
}
