// Components-library layout regressions.
//
// The Input/TextArea styles set `width:100%` together with padding + border, so
// they must use `box-sizing:border-box` or they overflow their container. This
// fixture mirrors the signup/forgot layout: a width-constrained container with a
// Field-wrapped Input and a submit Button.

#universal FieldLayoutHost(props) {
    return <div data-testid="layout-host" style="width:260px;box-sizing:border-box;padding:1.25rem;">
        <div style="display:grid;gap:1rem;">
            <Field label="Email">
                <Input type="email" name="email" placeholder="you@example.com" />
            </Field>
            <Button type="submit">Send reset link</Button>
        </div>
    </div>
}

#universal_test("input uses border-box and does not overflow its container") {
    <FieldLayoutHost />
    <script>
        const input = byCss('input[name="email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.css('box-sizing')).toBe('border-box')
        const parentW = input.el.parentElement.getBoundingClientRect().width
        const inputW = input.el.getBoundingClientRect().width
        expect(inputW <= parentW + 0.5).toBeTruthy()
    </script>
}

#universal_test("field input is a sibling of the button not nested inside it") {
    <FieldLayoutHost />
    <script>
        const input = byCss('input[name="email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.closest('button')).toBe(null)
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
    </script>
}

// Mirrors the account forgot-password form: two conditional (empty at SSR)
// blocks BEFORE a Field, followed by a Button. In the hydrated DOM the Field's
// root `<label>` was observed turning into `<button>` and absorbing the Button's
// attributes (type/size/data-variant/data-size/aria-busy) — i.e. the Field and
// Button hydration nodes got merged.
#universal ForgotLikeForm(props) {
    state error = ""
    state status = ""
    state loading = false
    return <form style="display:grid;gap:1rem;">
        {error ? <Alert variant="error" description={error} /> : null}
        {status ? <Alert variant="success" description={status} /> : null}
        <Field label="Email">
            <Input type="email" name="email" placeholder="you@example.com" />
        </Field>
        <Button type="submit" size="lg" loading={loading}>Send reset link</Button>
    </form>
}

#universal_test("field after empty conditional blocks is not merged into the button") {
    <ForgotLikeForm />
    <script>
        const input = byCss('input[name="email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
        expect(input.el.closest('button')).toBe(null)
    </script>
}

// The password show/hide control derives the input type from a plain local
// computed from state (`var kind = show ? "text" : "password"`). Toggling the
// button must update the input's type.
#universal ShowHideFixture(props) {
    state show = false
    var kind = show ? "text" : "password"
    return <div>
        <input data-testid="inp" type={kind} />
        <button data-testid="tog" type="button" onClick={() => show = !show}>Show</button>
    </div>
}

#universal_test("state-derived local updates a bound attribute on click") {
    <ShowHideFixture />
    <script>
        const inp = byTestId('inp')
        expect(inp.jsProp('type')).toBe('password')
        byTestId('tog').click()
        expect(inp.jsProp('type')).toBe('text')
    </script>
}

// Same as above, but the value is passed to a child component (mirrors
// PasswordInput: `type` derived from state, handed to `Input`). Tests whether a
// state-derived prop is re-evaluated when the parent re-renders.
#universal TypeSink(props) {
    return <input data-testid="inp" type={props.kind || "text"} />
}

#universal TypeHostViaComponent(props) {
    state show = false
    var kind = show ? "text" : "password"
    return <div>
        <TypeSink kind={kind} />
        <button data-testid="tog" type="button" onClick={() => show = !show}>Show</button>
    </div>
}

#universal_test("state-derived local passed as a component prop updates") {
    <TypeHostViaComponent />
    <script>
        const inp = byTestId('inp')
        expect(inp.jsProp('type')).toBe('password')
        byTestId('tog').click()
        expect(inp.jsProp('type')).toBe('text')
    </script>
}

// The show/hide control as it appears in the account app: a password input plus
// a toggle button inside an InputGroup, wrapped in a Field, preceded by an empty
// conditional block. If the Field root is mis-hydrated into a <button>, the
// toggle ends up nested in a button and cannot be clicked.
#universal PwFieldShowHide(props) {
    state err = ""
    state show = false
    var kind = show ? "text" : "password"
    return <form>
        {err ? <Alert variant="error" description={err} /> : null}
        <Field label="Password">
            <InputGroup>
                <input data-testid="pw" type={kind} />
                <button data-testid="tog" type="button" onClick={() => show = !show}>{show ? "Hide" : "Show"}</button>
            </InputGroup>
        </Field>
    </form>
}

#universal_test("password show/hide inside a Field after an empty conditional toggles") {
    <PwFieldShowHide />
    <script>
        const pw = byTestId('pw')
        expect(pw.jsProp('type')).toBe('password')
        byTestId('tog').click()
        expect(pw.jsProp('type')).toBe('text')
    </script>
}



