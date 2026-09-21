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
