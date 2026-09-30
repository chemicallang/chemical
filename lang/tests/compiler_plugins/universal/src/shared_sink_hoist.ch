// =============================================================================
// Regression: a shared sink must not swallow a component definition's hoist.
//
// With a `SharedAssets` sink attached, page-independent output — component
// definitions above all — is routed into the SINK's `js_data`, not into the
// page's `pageJs`. `get_js_pos` and `move_js_range` measured and moved `pageJs`.
// The offsets belonged to a different buffer, `move_js_range` bounds-checked
// them, returned silently, and the hoist became a no-op.
//
// The definition therefore stayed exactly where it was appended: inside the
// `createElement(...)` argument list of the component that used it. The rest of
// the argument list then followed with no comma, so the file was not JavaScript:
//
//     return $_ur.createElement("div", {"class": "outer"}, `
//         `, function SinkInner(props) { ... }
//     $_uc_c(SinkInner, {children: [...]}), `
//         `, $_ur.createElement("span", {"class": "sib"}, `after`), ...
//
// In Web/account that killed the whole signed-in shell at parse time — every
// route, every component — with a green test suite, because every other bundle
// test asserts on SUBSTRINGS and a bundle that cannot parse still contains every
// string anyone thought to look for. `shared_assets_dedupes_definitions_across_pages`
// in shared_assets.ch passed the whole time: the definition was present, just in
// the wrong place, and that test counts definitions rather than checking where
// they are.
//
// The trigger is the SINK, not the component shape. The same tree with no sink
// attached emits correctly, which is why a "component used as the first child"
// test — the obvious guess, and the one this file replaced — passes with the bug
// in place. Both cases are below: the sink case asserts the fix, the no-sink case
// is the control that proves the trigger really is the sink.
// =============================================================================

// A leaf, so the only thing under test is the PARENT's emission.
#universal SinkInner(props) {
    return <span class="sink-inner">{props.children}</span>
}

#universal SinkOuter(props) {
    return <div class="sink-outer">
        <SinkInner><b>hi</b></SinkInner>
        <span class="sink-sibling">after</span>
    </div>
}

// `#html` needs a variable called `page` in scope, so a page that is not named
// `page` goes through a helper. Same reason `shared_assets.ch` has
// `shared_build_greeting`.
func sink_hoist_render(page : &mut HtmlPage) {
    #html { <SinkOuter /> }
}

func sink_hoist_count_char(text : std::string_view, want : char) : i64 {
    var n = 0i64
    var i = 0u
    while(i < text.size()) {
        if(text.get(i) == want) { n = n + 1i64 }
        i = i + 1u
    }
    return n
}

// A hoisted definition is a top-level `function name(props) {`. A definition used
// as a call argument appears after `", function "`, which is the exact shape of
// the bug and needs no JavaScript parser to detect.
//
// The literal lives inside the function rather than at file scope: a file-scope
// `const x = std::string_view(...)` is a call at top level and does not resolve.
func sink_hoist_assert_no_inlined_definition(env : &mut TestEnv, label : &std::string_view, js : &std::string) : bool {
    const inlined_def = std::string_view(", function ")
    var view = js.to_view()
    if(view.contains(&inlined_def)) {
        env.error("a component definition was emitted inside a call argument list instead of being hoisted")
        env.info(label.data())
        env.info(js.data())
        return false
    }

    // Kept as well because it catches a different, also-real class — output
    // truncated mid-expression, which is what a failed emit looks like. It is
    // NOT the check that catches this bug: the broken bundle's parentheses and
    // braces were perfectly balanced. Counting does not understand strings, so a
    // bracket inside a string literal would make it complain; the fixtures here
    // have none.
    var opens = sink_hoist_count_char(view, '(')
    var closes = sink_hoist_count_char(view, ')')
    if(opens != closes) {
        env.error("unbalanced parentheses in the emitted JS")
        env.info(label.data())
        return false
    }
    return true
}

@test
public func shared_sink_hoists_a_nested_component_definition(env : &mut TestEnv) {
    var shared = shared_assets("sink_hoist")
    var page = HtmlPage()
    page.attach_shared(shared)
    page.defaultUniversalSetup()
    sink_hoist_render(&mut page)

    var bundle = std::string()
    bundle.append_view(shared.js())

    if(!sink_hoist_assert_no_inlined_definition(env, &std::string_view("with a shared sink attached"), &bundle)) {
        return
    }

    // And the leaf must actually be there as a hoisted definition, referenced
    // rather than inlined — an empty bundle must not pass this by having nothing
    // wrong with it.
    const sig = std::string_view("SinkInner(props)")
    if(!bundle.to_view().contains(&sig)) {
        env.error("the nested component emitted no definition into the sink")
        env.info(bundle.data())
        return
    }
    env.success("a nested component definition is hoisted out of the call it was emitted in")
}

@test
public func no_sink_hoists_a_nested_component_definition_too(env : &mut TestEnv) {
    // The control. Same tree, no sink: this passed before the fix and must keep
    // passing, which is what makes the test above meaningful — if this one ever
    // starts failing, the regression is not the sink path.
    var page = HtmlPage()
    page.defaultUniversalSetup()
    sink_hoist_render(&mut page)

    var js = std::string()
    js.append_view(page.getJs())

    if(!sink_hoist_assert_no_inlined_definition(env, &std::string_view("with no sink attached"), &js)) {
        return
    }
    env.success("the same tree emits correctly without a sink, so the sink is the trigger")
}

@test
public func shared_sink_hoist_survives_a_second_page_on_the_same_sink(env : &mut TestEnv) {
    // The account warms the sink from one render and then every request page
    // attaches it, so the second page's definitions are appended to a sink that
    // already has content in it. That is the state in which the offsets were most
    // wrong, so it gets its own test.
    var shared = shared_assets("sink_hoist_two")

    var warm = HtmlPage()
    warm.attach_shared(shared)
    warm.defaultUniversalSetup()
    sink_hoist_render(&mut warm)

    var request = HtmlPage()
    request.attach_shared(shared)
    request.defaultUniversalSetup()
    sink_hoist_render(&mut request)

    var bundle = std::string()
    bundle.append_view(shared.js())

    if(!sink_hoist_assert_no_inlined_definition(env, &std::string_view("sink warmed then reused"), &bundle)) {
        return
    }
    env.success("definitions stay hoisted when a second page renders into an already-warm sink")
}

