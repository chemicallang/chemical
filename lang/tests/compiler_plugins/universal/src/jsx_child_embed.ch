using namespace std;

// Regression tests for a `${...}` Chemical embed used as a JSX *child*.
//
// BUG (pre-fix): the JSX-child lexer split `${expr}` into a literal `$` text
// child plus a `{expr}` expression container. SSR therefore rendered a stray
// `$` and the client bundle emitted the server-only Chemical expression
// (`probe_emit(page, ...)`), producing malformed JS.
//
// FIX: JSX child mode now lexes `${` as a `ChemicalStart`, the parser keeps the
// whole embed as a child node, and the converter skips ChemicalValue children in
// the JavaScript target (their SSR markup is adopted during hydration).

func jsx_child_test_emit(page : &mut HtmlPage, who : string_view) {
    var msg = string("CHILD-")
    msg.append_view(&who)
    #html { <b class="child-emit">{msg}</b> }
}

#universal JsxChildEmbedApp(props) {
    return <div class="pw">${jsx_child_test_emit(page, "PROBE")}</div>
}

@test
public func universal_jsx_child_chemical_embed_renders_at_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <JsxChildEmbedApp /> }
    var html = page.getHtml()

    // The emitter ran at SSR time and its markup sits inside the host element.
    const emitted = std::string_view("<div class=\"pw\"><b class=\"child-emit\">CHILD-PROBE</b></div>")
    if(html.find(&emitted) == std::NPOS) {
        env.error("a ${...} JSX child must render its server emission in place")
        env.info(html.data())
        return
    }

    // No stray literal `$` child.
    const stray = std::string_view("<div class=\"pw\">$</div>")
    if(html.find(&stray) != std::NPOS) {
        env.error("a ${...} JSX child must not render a literal '$'")
        return
    }
    const strayOpen = std::string_view("class=\"pw\">$")
    if(html.find(&strayOpen) != std::NPOS) {
        env.error("a ${...} JSX child must not begin with a literal '$'")
    }
}

@test
public func universal_jsx_child_chemical_embed_not_in_client_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <JsxChildEmbedApp /> }
    var js = page.getJs()

    // The server-only emitter must not leak into the client bundle.
    if(js.find(&std::string_view("jsx_child_test_emit")) != std::NPOS) {
        env.error("a server-only ${...} JSX child must not be emitted into client JS")
        env.info(js.data())
        return
    }
    // The host element still hydrates on the client.
    if(js.find(&std::string_view("\"pw\"")) == std::NPOS) {
        env.error("the host element of a ${...} JSX child must still emit on the client")
        env.info(js.data())
    }
}
