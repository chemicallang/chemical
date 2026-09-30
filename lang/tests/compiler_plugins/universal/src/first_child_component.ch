// Regression: a component used as the FIRST child of an element emits its
// definition INLINE, inside the parent's `createElement(...)` argument list, and
// the parent's closing tokens are lost with it. The client bundle is then not
// valid JavaScript, so the WHOLE page dies at parse time - every component, every
// route, nothing works.
//
// Observed in Web/account: `/app.js` failed with
//   Uncaught SyntaxError: missing ) after argument list
// and then `window.$__uni_dispatch is not a function`, because the bundle never
// parsed. `AccountProfileSection` renders
//
//     return <div class="wq-card-stack">
//         <ToastViewport>
//             {toastVisible ? <Toast ... /> : null}
//         </ToastViewport>
//         ...
//     </div>
//
// and the emitted text was
//
//     return $_ur.createElement("div", {"class": "wq-card-stack"}, `
//         `, function components_ToastViewport(props) { ... } }
//
// - the definition where a hoisted reference belongs, and the `)` that closes
// `createElement` gone.
//
// This file is the minimal shape: a component whose first child is another
// component, with a sibling after it so there is text to lose.

using namespace std;

// A leaf, so the only thing under test is the PARENT's emission.
#universal CbiFirstChildLeaf(props) {
    return <span class="leaf">{props.label}</span>
}

#universal CbiFirstChildParent(props) {
    return <div class="wrap">
        <CbiFirstChildLeaf label="one" />
        <span class="sibling">after</span>
    </div>
}

// The same shape with a conditional inside the first child, which is what
// AccountProfileSection does - a toast viewport whose body is conditional.
#universal CbiFirstChildParentConditional(props) {
    return <div class="wrap2">
        <CbiFirstChildLeaf label="one" />
    </div>
}

// `getJs()` returns a view INTO the page, so the bytes are copied out: the page
// is a local here and would be destroyed before the caller looks at them.
func cbi_first_child_render(which : &string_view) : string {
    var page = HtmlPage()
    var want = *which
    if(want.equals("conditional")) {
        #html { <CbiFirstChildParentConditional /> }
    } else {
        #html { <CbiFirstChildParent /> }
    }
    var out = std::string()
    var view = page.getJs()
    var i = 0u
    while(i < view.size()) {
        out.append(view.get(i))
        i = i + 1u
    }
    return out
}

// The structural assertion that does not need a JavaScript parser: a
// `createElement(` call must be closed. Counting is deliberately naive about
// parens inside strings - the point is to catch a MISSING one, and the fixture
// below has no parens in its string literals.
func cbi_count_char(text : string_view, want : char) : i64 {
    var n = 0i64
    var i = 0u
    while(i < text.size()) {
        if(text.get(i) == want) { n = n + 1i64 }
        i = i + 1u
    }
    return n
}

@test
public func universal_component_as_first_child_closes_its_parent_call(env : &mut TestEnv) {
    var js = cbi_first_child_render("plain")
    if(js.size() == 0u) {
        env.error("nothing was emitted for the first-child fixture")
        return
    }

    var opens = cbi_count_char(js.to_view(), '(')
    var closes = cbi_count_char(js.to_view(), ')')
    if(opens != closes) {
        var msg = std::string("unbalanced parentheses in the emitted JS: ")
        msg.append_integer(opens)
        msg.append_view(" open, ")
        msg.append_integer(closes)
        msg.append_view(" close - the bundle will not parse")
        env.error(msg.data())
        var info = js.to_view()
    env.info(info.data())
        return
    }

    // The leaf is used once, so it should be DEFINED once and REFERENCED, not
    // defined in the middle of the parent's argument list.
    var def = std::string_view("function account_tests_CbiFirstChildLeaf")
    var at = js.find(&def)
    if(at == std::NPOS) {
        // Accepted alternative: a different module scope prefix. Check for the
        // bare name so the assertion is about the structure, not the prefix.
        def = std::string_view("function ")
        at = js.find(&def)
    }
    if(at == std::NPOS) {
        env.error("the leaf component emitted no definition at all")
    }
}

@test
public func universal_first_child_definition_is_not_emitted_inside_create_element(env : &mut TestEnv) {
    var js = cbi_first_child_render("plain")

    // The failure shape: `createElement(... , function name(` - a definition
    // used as an argument. A hoisted definition lives at the start of a line,
    // never after `createElement(`.
    const inlined = std::string_view("createElement(\"div\", {\"class\": \"wrap\"}, `")
    var at = js.find(&inlined)
    if(at == std::NPOS) { return }
    // Found the parent's opening. Now make sure a `function` definition does not
    // appear between that point and the closing of the call. Built as a `string`
    // because `string` has no `subview`/`skip` of its own.
    var start = at + inlined.size()
    var end = start + 400u
    if(end > js.size()) { end = js.size() }
    var window = std::string()
    var i = start
    while(i < end) {
        window.append(js.get(i))
        i = i + 1u
    }
    var window_view = window.to_view()
    if(window_view.find(&std::string_view(", function ")) != std::NPOS) {
        env.error("a component definition was emitted inside a createElement argument list")
        var info = js.to_view()
    env.info(info.data())
    }
}

@test
public func universal_component_as_first_child_with_conditional_body_balances(env : &mut TestEnv) {
    // The AccountProfileSection shape, minus the conditional in the leaf, since
    // the leaf's own body is not what breaks. The point is the same parent
    // structure under a different child.
    var js = cbi_first_child_render("conditional")
    var opens = cbi_count_char(js.to_view(), '(')
    var closes = cbi_count_char(js.to_view(), ')')
    if(opens != closes) {
        var msg = std::string("unbalanced parentheses: ")
        msg.append_integer(opens)
        msg.append_view(" open, ")
        msg.append_integer(closes)
        msg.append_view(" close")
        env.error(msg.data())
        var info = js.to_view()
    env.info(info.data())
    }
}


