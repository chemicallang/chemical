// A carriage return inside a JSX text child must not reach the client bundle.
//
// THE BUG. `universal_cbi` emitted the client half of a JSX text node from the raw
// source. A whitespace child between two elements therefore became "\r\n        " in
// the vnode. The SSR half emitted the same bytes into the HTML, and the HTML parser
// then normalised CRLF to LF inside text content (HTML spec, not a browser quirk), so
// the DOM held "\n        " while the client held "\r\n        ". Every multi-line JSX
// child in a CRLF-sourced project reported "hydration mismatch: text node differs
// from SSR", once per gap, until the runtime gave up at 25.
//
// THE FIX normalises newlines on the CLIENT side only. SSR is left alone on purpose:
// the browser is what normalises, so the client has to match the browser rather than
// the source.
//
// WHY A LITERAL CR BYTE. This repository is LF, so a fixture written with ordinary
// line endings passes with or without the fix and tests nothing. The trigger has to
// be in the source text itself, and a CR embedded in the inter-element gap is the
// smallest deterministic way to express "this text child contains a carriage return".
//
// The assertion is on the emitted JS, not on a hydrated DOM, because the emitted
// bundle is the converter's own output: this fails the moment `normalize_jsx_newlines`
// stops being applied to JSXText, with no browser in the loop.

// Inter-element whitespace containing a real CR (0x0D) before the second span.
#universal CrLfTextChild(props) {
    return <div data-testid="t"><span>A</span>  <span>B</span></div>
}

@test
public func universal_crlf_jsx_text_child_normalized_in_client_bundle(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <CrLfTextChild /> }
    var js = page.getJs()

    // A raw CR byte anywhere in the bundle means source newlines leaked into a
    // string literal.
    if(js.contains("\r")) {
        env.error("client bundle contains a raw carriage return")
        env.info(js.data())
        return
    }

    // Or the escaped form: a JS string literal holding "\r" rather than "\n".
    if(js.contains("\\r")) {
        env.error("client bundle contains an escaped \\r in a string literal")
        env.info(js.data())
        return
    }

    // Guard against the fixture silently losing its CR, which would make this test
    // green for the wrong reason. The gap must still be in the bundle as newlines.
    if(!js.contains("\\n")) {
        env.error("fixture did not emit a text child with newlines at all")
        env.info(js.data())
        return
    }

    env.success("a CRLF in a JSX text child is normalized in the client bundle")
}
