// Phase 2 component fixture. The `document` read is inside a lambda, so it is
// handler position for the hydration-safety check (WEB-BODY-004) and only the
// ambient-root check (WEB-BODY-008) can report it. In the positive build the
// client-only `provider` module supplies the `document` root; the negative copy
// removes that import and must fail with WEB-BODY-008.
#universal DocProbe(props) {
    var read = () => { console.log(document.title) }
    return <div onClick={read}>probe</div>
}

// The render site: a real page body in the same module as the component, so the
// fixture exercises the normal `HtmlPage`/`#html` context.
func render_two_side(page : &mut HtmlPage) {
    page.appendTitle("Index two side fixture")

    #html {
        <DocProbe />
    }
}
