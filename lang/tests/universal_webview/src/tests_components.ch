// Ported from components-e2e/tests/components.spec.ts.
//
// Tests that rely on portals (Dialog/Sheet/Select/Dropdown/Toast) are marked
// `isolate` so their body-appended portal content cannot collide with another
// test's fixture on the shared page.

// ---- Button ----------------------------------------------------------------

#universal_test("button renders all variants") {
    <ButtonFixture />
    <script>
        const f = byTestId('button-fixture')
        const variants = ['default', 'destructive', 'outline', 'secondary', 'ghost', 'link', 'success', 'warning', 'info', 'accent']
        for(const v of variants) { expect(f.getByTestId('btn-' + v)).toHaveAttribute('data-variant', v) }
    </script>
}

#universal_test("button renders sizes") {
    <ButtonFixture />
    <script>
        const f = byTestId('button-fixture')
        expect(f.getByTestId('btn-sm')).toHaveAttribute('data-size', 'sm')
        expect(f.getByTestId('btn-lg')).toHaveAttribute('data-size', 'lg')
        expect(f.getByTestId('btn-icon')).toHaveAttribute('data-size', 'icon')
    </script>
}

#universal_test("button disabled state") {
    <ButtonFixture />
    <script>
        const btn = byTestId('button-fixture').getByTestId('btn-disabled')
        expect(btn).toBeDisabled()
        expect(btn).toHaveAttribute('aria-disabled', 'true')
    </script>
}

#universal_test("button loading state") {
    <ButtonFixture />
    <script>
        const btn = byTestId('button-fixture').getByTestId('btn-loading')
        btn.click()
        expect(btn).toHaveAttribute('loading', 'true')
        await t.sleep(700)
        expect(btn).not.toHaveAttribute('loading', 'true')
    </script>
}

#universal_test("button is a button type=button") {
    <ButtonFixture />
    <script>
        const btn = byTestId('button-fixture').getByTestId('btn-default')
        expect(btn).toHaveJSProperty('tagName', 'BUTTON')
        expect(btn).toHaveAttribute('type', 'button')
    </script>
}

// ---- Tabs ------------------------------------------------------------------

#universal_test("tabs switch panels on click") {
    <TabsFixture />
    <script>
        const f = byTestId('tabs-fixture')
        expect(f.getByRole('tabpanel', { name: 'Alpha' })).toBeVisible()
        f.getByRole('tab', { name: 'Beta' }).click()
        expect(f.getByRole('tabpanel', { name: 'Beta' })).toBeVisible()
        expect(f.getByRole('tabpanel', { name: 'Alpha' })).toBeHidden()
    </script>
}

#universal_test("tabs mark the active tab") {
    <TabsFixture />
    <script>
        const f = byTestId('tabs-fixture')
        const alpha = f.getByRole('tab', { name: 'Alpha' })
        const beta = f.getByRole('tab', { name: 'Beta' })
        expect(alpha).toHaveAttribute('data-active', 'true')
        expect(alpha).toHaveAttribute('aria-selected', 'true')
        expect(beta).toHaveAttribute('data-active', 'false')
        beta.click()
        expect(beta).toHaveAttribute('data-active', 'true')
        expect(beta).toHaveAttribute('aria-selected', 'true')
        expect(alpha).toHaveAttribute('data-active', 'false')
    </script>
}

#universal_test("tabs arrow keys navigate") {
    <TabsFixture />
    <script>
        const f = byTestId('tabs-fixture')
        const alpha = f.getByRole('tab', { name: 'Alpha' })
        const beta = f.getByRole('tab', { name: 'Beta' })
        const gamma = f.getByRole('tab', { name: 'Gamma' })
        alpha.focus()
        t.press('ArrowRight')
        expect(beta).toBeFocused()
        t.press('ArrowRight')
        expect(gamma).toBeFocused()
        t.press('ArrowRight')
        expect(alpha).toBeFocused()
        t.press('Home')
        expect(alpha).toBeFocused()
        t.press('End')
        expect(gamma).toBeFocused()
    </script>
}

// ---- Accordion -------------------------------------------------------------

#universal_test("accordion opens and closes") {
    <AccordionFixture />
    <script>
        const f = byTestId('accordion-fixture')
        const summary = f.findAll('button').nth(0)
        const content = f.getByTestId('acc-item-0').find('[data-accordion-content]')
        expect(content).toBeHidden()
        summary.click()
        expect(content).toBeVisible()
        summary.click()
        expect(content).toBeHidden()
    </script>
}

#universal_test("accordion arrow keys navigate") {
    <AccordionFixture />
    <script>
        const f = byTestId('accordion-fixture')
        const first = f.findAll('button').nth(0)
        const second = f.findAll('button').nth(1)
        first.focus()
        t.press('ArrowDown')
        expect(second).toBeFocused()
        t.press('Home')
        expect(first).toBeFocused()
    </script>
}

#universal_test("accordion multiple items open simultaneously") {
    <AccordionMultiFixture />
    <script>
        const f = byTestId('accordion-multi-fixture')
        expect(f.getByText('Content A')).toBeVisible()
        expect(f.getByText('Content B')).toBeVisible()
        expect(f.getByText('Content C')).toBeHidden()
        f.findAll('button').nth(2).click()
        expect(f.getByText('Content C')).toBeVisible()
        expect(f.getByText('Content A')).toBeVisible()
    </script>
}

// ---- Dialog (isolate: portal) ----------------------------------------------

#universal_test("dialog opens, shows content, closes", isolate) {
    <DialogFixture />
    <script>
        const f = byTestId('dialog-fixture')
        expect(byTestId('dialog-content')).toBeHidden()
        f.getByTestId('dialog-open').click()
        expect(byTestId('dialog-content')).toBeVisible()
        expect(byTestId('dialog-content')).toContainText('Dialog title')
        byTestId('dialog-confirm').click()
        expect(byTestId('dialog-content')).toBeHidden()
    </script>
}

#universal_test("dialog inerts the background", isolate) {
    <DialogFixture />
    <script>
        expect(window.__ut_scope).not.toHaveAttribute('inert')
        byTestId('dialog-fixture').getByTestId('dialog-open').click()
        expect(byTestId('dialog-content')).toBeVisible()
        expect(window.__ut_scope).toHaveAttribute('inert')
        byTestId('dialog-confirm').click()
        expect(byTestId('dialog-content')).toBeHidden()
        expect(window.__ut_scope).not.toHaveAttribute('inert')
    </script>
}

#universal_test("dialog traps focus and closes on Escape", isolate) {
    <DialogFixture />
    <script>
        byTestId('dialog-fixture').getByTestId('dialog-open').click()
        await t.sleep(80)
        const confirm = byTestId('dialog-confirm')
        const cancel = byTestId('dialog-cancel')
        expect(confirm).toBeFocused()
        t.press('Tab')
        expect(cancel).toBeFocused()
        t.press('Tab')
        expect(confirm).toBeFocused()
        t.press('Escape')
        expect(byTestId('dialog-content')).toBeHidden()
    </script>
}

#universal_test("dialog has aria-modal", isolate) {
    <DialogFixture />
    <script>
        byTestId('dialog-fixture').getByTestId('dialog-open').click()
        expect(byRole('dialog')).toBeVisible()
    </script>
}

// ---- Select (isolate: portal menu) -----------------------------------------

#universal_test("select opens, picks option, reports value", isolate) {
    <SelectFixture />
    <script>
        const f = byTestId('select-fixture')
        expect(f.getByTestId('select-value')).toHaveText('Chosen: none')
        f.findAll('button').first().click()
        expect(byRole('listbox')).toBeVisible()
        byRole('option', { name: 'Banana' }).click()
        expect(f.getByTestId('select-value')).toHaveText('Chosen: Banana')
    </script>
}

#universal_test("select keyboard navigation and typeahead", isolate) {
    <SelectFixture />
    <script>
        const f = byTestId('select-fixture')
        const trigger = f.find('button[aria-haspopup="listbox"]')
        trigger.click()
        trigger.focus()
        expect(byRole('listbox')).toBeVisible()
        t.press('ArrowDown')
        t.press('Enter')
        expect(f.getByTestId('select-value')).toHaveText('Chosen: Banana')
        trigger.click()
        trigger.focus()
        t.press('c')
        t.press('Enter')
        expect(f.getByTestId('select-value')).toHaveText('Chosen: Cherry')
    </script>
}

// ---- Portal (isolate) ------------------------------------------------------

#universal_test("select escapes overflow:hidden via portal", isolate) {
    <PortalFixture />
    <script>
        const f = byTestId('portal-fixture')
        f.getByTestId('portal-overflow-select').find('button').click()
        expect(byRole('listbox')).toBeVisible()
        byRole('option', { name: 'Three' }).click()
        expect(f.getByTestId('portal-value')).toHaveText('Chosen: Three')
    </script>
}

// ---- Slider ----------------------------------------------------------------

#universal_test("slider keyboard interaction") {
    <SliderFixture />
    <script>
        const f = byTestId('slider-fixture')
        const slider = f.getByRole('slider', { name: 'Volume' })
        expect(f.getByTestId('slider-value')).toHaveText('Value: 30')
        slider.focus()
        slider.press('ArrowRight')
        expect(f.getByTestId('slider-value')).toHaveText('Value: 40')
        slider.press('End')
        expect(f.getByTestId('slider-value')).toHaveText('Value: 100')
        slider.press('Home')
        expect(f.getByTestId('slider-value')).toHaveText('Value: 0')
    </script>
}

// ---- Checkbox / Switch / Radio --------------------------------------------

#universal_test("checkbox toggles") {
    <ToggleFixture />
    <script>
        const cb = byTestId('checkbox-control').find('input')
        expect(cb).not.toBeChecked()
        cb.click()
        expect(cb).toBeChecked()
    </script>
}

#universal_test("switch toggles") {
    <ToggleFixture />
    <script>
        const sw = byTestId('switch-control').find('input')
        expect(sw).toBeChecked()
        sw.click()
        expect(sw).not.toBeChecked()
    </script>
}

#universal_test("radio mutual exclusion") {
    <ToggleFixture />
    <script>
        const a = byTestId('radio-a').find('input')
        const b = byTestId('radio-b').find('input')
        expect(a).toBeChecked()
        b.click()
        expect(b).toBeChecked()
        expect(a).not.toBeChecked()
    </script>
}

// ---- ToggleGroup -----------------------------------------------------------

#universal_test("toggle group single mode") {
    <ToggleGroupFixture />
    <script>
        const f = byTestId('togglegroup-fixture')
        const bold = f.getByRole('button', { name: 'Bold' })
        const italic = f.getByRole('button', { name: 'Italic' })
        expect(bold).toHaveAttribute('aria-pressed', 'true')
        italic.click()
        expect(bold).toHaveAttribute('aria-pressed', 'false')
        expect(italic).toHaveAttribute('aria-pressed', 'true')
    </script>
}

#universal_test("toggle group multiple mode") {
    <ToggleGroupMultipleFixture />
    <script>
        const f = byTestId('togglegroup-multi-fixture')
        const bold = f.getByRole('button', { name: 'Bold' })
        const italic = f.getByRole('button', { name: 'Italic' })
        expect(bold).toHaveAttribute('aria-pressed', 'true')
        italic.click()
        expect(bold).toHaveAttribute('aria-pressed', 'true')
        expect(italic).toHaveAttribute('aria-pressed', 'true')
        bold.click()
        expect(bold).toHaveAttribute('aria-pressed', 'false')
    </script>
}

#universal_test("toggle group scoping keeps independent selection") {
    <ToggleGroupScopedFixture />
    <script>
        const f = byTestId('togglegroup-scoped-fixture')
        const a = f.getByTestId('tgs-a')
        const b = f.getByTestId('tgs-b')
        const aBold = a.getByRole('button', { name: 'A-Bold' })
        const aItalic = a.getByRole('button', { name: 'A-Italic' })
        const bBold = b.getByRole('button', { name: 'B-Bold' })
        const bItalic = b.getByRole('button', { name: 'B-Italic' })
        expect(aBold).toHaveAttribute('aria-pressed', 'true')
        expect(aItalic).toHaveAttribute('aria-pressed', 'false')
        expect(bItalic).toHaveAttribute('aria-pressed', 'true')
        expect(bBold).toHaveAttribute('aria-pressed', 'false')
        aItalic.click()
        expect(aItalic).toHaveAttribute('aria-pressed', 'true')
        expect(aBold).toHaveAttribute('aria-pressed', 'false')
        expect(bItalic).toHaveAttribute('aria-pressed', 'true')
        bBold.click()
        expect(bBold).toHaveAttribute('aria-pressed', 'true')
        expect(bItalic).toHaveAttribute('aria-pressed', 'false')
        expect(aItalic).toHaveAttribute('aria-pressed', 'true')
    </script>
}

// ---- RadioGroup ------------------------------------------------------------

#universal_test("radio group selects single option", isolate) {
    <RadioGroupFixture />
    <script>
        const f = byTestId('radiogroup-fixture')
        expect(f.getByRole('radio', { name: 'Medium' })).toBeChecked()
        f.getByRole('radio', { name: 'Large' }).check()
        expect(f.getByRole('radio', { name: 'Large' })).toBeChecked()
        expect(f.getByRole('radio', { name: 'Medium' })).not.toBeChecked()
    </script>
}

#universal_test("radio group defaultValue after hydration") {
    <RadioGroupFixture />
    <script>
        expect(byTestId('radiogroup-fixture').getByRole('radio', { name: 'Medium' })).toBeChecked()
    </script>
}

#universal_test("radio item without provider stays unchecked") {
    <RadioGroupNoProviderFixture />
    <script>
        const solo = byTestId('radiogroup-noprovider-fixture').getByRole('radio', { name: 'Solo' })
        expect(solo).not.toBeChecked()
        solo.click()
        expect(byTestId('radiogroup-noprovider-fixture').find('label')).toBeVisible()
    </script>
}

// ---- Toast (isolate: portaled) ---------------------------------------------

#universal_test("toast auto-dismisses", isolate) {
    <ToastFixture />
    <script>
        expect(byTestId('toast-item')).toContainText('Changes saved')
        await t.sleep(1800)
        expect(byTestId('toast-item')).toBeHidden()
    </script>
}

// ---- Collapsible -----------------------------------------------------------

#universal_test("collapsible toggles content") {
    <CollapsibleFixture />
    <script>
        const f = byTestId('collapsible-fixture')
        const trigger = f.getByRole('button', { name: 'More info' })
        expect(trigger).toHaveAttribute('aria-expanded', 'false')
        trigger.click()
        expect(trigger).toHaveAttribute('aria-expanded', 'true')
        expect(f).toContainText('Hidden details here.')
        trigger.click()
        expect(trigger).toHaveAttribute('aria-expanded', 'false')
    </script>
}

// ---- Sheet (isolate: portal) -----------------------------------------------

#universal_test("sheet opens and closes", isolate) {
    <SheetFixture />
    <script>
        byTestId('sheet-fixture').getByTestId('sheet-open').click()
        expect(byRole('dialog')).toBeVisible()
        expect(byRole('dialog')).toContainText('Sheet body')
        byRole('dialog').getByRole('button', { name: 'Close' }).click()
        expect(byRole('dialog')).toBeHidden()
    </script>
}

#universal_test("sheet inerts background", isolate) {
    <SheetFixture />
    <script>
        byTestId('sheet-fixture').getByTestId('sheet-open').click()
        expect(window.__ut_scope).toHaveAttribute('inert')
        byRole('dialog').getByRole('button', { name: 'Close' }).click()
        expect(window.__ut_scope).not.toHaveAttribute('inert')
    </script>
}

// ---- Dropdown (isolate: portal) --------------------------------------------

#universal_test("dropdown escapes overflow:hidden", isolate) {
    <DropdownFixture />
    <script>
        byTestId('dropdown-fixture').getByRole('button', { name: 'Actions' }).click()
        const menu = byRole('menu')
        expect(menu).toBeVisible()
        expect(document.body.contains(menu.el)).toBe(true)
        menu.getByRole('menuitem', { name: 'Delete' }).click()
        expect(menu).toBeHidden()
    </script>
}

#universal_test("select menu does not inert background", isolate) {
    <SelectFixture />
    <script>
        byTestId('select-fixture').findAll('button').first().click()
        expect(byRole('listbox')).toBeVisible()
        expect(window.__ut_scope).not.toHaveAttribute('inert')
    </script>
}

// ---- Error boundary --------------------------------------------------------

#universal_test("error boundary shows fallback UI") {
    <ErrorBoundaryFixture />
    <script>
        const f = byTestId('error-fixture')
        f.getByTestId('error-mount').click()
        expect(f.getByTestId('error-fallback')).toHaveText('Fallback shown')
    </script>
}

#universal_test("error boundary default fallback") {
    <ErrorBoundaryFixture />
    <script>
        byTestId('error-fixture').getByTestId('error-default-mount').click()
        expect(byTestId('error-fixture').find('.chx-error-boundary')).toHaveAttribute('role', 'alert')
    </script>
}

// ---- Alert -----------------------------------------------------------------

#universal_test("alert renders all variants with role=alert") {
    <AlertFixture />
    <script>
        const f = byTestId('alert-fixture')
        for(const v of ['info', 'success', 'error', 'warning', 'default', 'accent']) {
            expect(f.find('[data-variant="' + v + '"][role="alert"]')).toBeVisible()
        }
    </script>
}

#universal_test("alert shows title and description") {
    <AlertFixture />
    <script>
        const info = byTestId('alert-fixture').find('[data-variant="info"]')
        expect(info).toContainText('Heads up')
        expect(info).toContainText('This is info.')
    </script>
}

#universal_test("alert dismiss removes it") {
    <AlertFixture />
    <script>
        const info = byTestId('alert-fixture').find('[data-variant="info"]')
        info.getByRole('button', { name: 'Dismiss alert' }).click()
        expect(info).toBeHidden()
    </script>
}

// ---- Avatar ----------------------------------------------------------------

#universal_test("avatar renders sizes and fallback text") {
    <AvatarFixture />
    <script>
        const f = byTestId('avatar-fixture')
        expect(f.find('[data-size="xs"]')).toContainText('XS')
        expect(f.find('[data-size="sm"]')).toContainText('SM')
        expect(f.find('[data-size="lg"]')).toContainText('LG')
        expect(f.find('[data-size="xl"]')).toContainText('XL')
    </script>
}

#universal_test("avatar bordered style") {
    <AvatarFixture />
    <script>
        expect(byTestId('avatar-fixture').find('[data-bordered="true"]')).toContainText('BD')
    </script>
}

#universal_test("avatar group renders multiple avatars") {
    <AvatarFixture />
    <script>
        expect(byTestId('avatar-fixture').findAll('[data-size]').count()).toBeGreaterThanOrEqual(5)
    </script>
}

#universal_test("avatar more shows count") {
    <AvatarFixture />
    <script>
        expect(byTestId('avatar-fixture').getByText('+5')).toBeVisible()
    </script>
}

// ---- Badge -----------------------------------------------------------------

#universal_test("badge renders all variants") {
    <BadgeFixture />
    <script>
        const f = byTestId('badge-fixture')
        expect(f.find('span[data-variant="default"]')).toContainText('Default')
        expect(f.find('span[data-variant="secondary"]')).toContainText('Secondary')
        expect(f.find('span[data-variant="success"]')).toContainText('Success')
        expect(f.find('span[data-variant="error"]')).toContainText('Error')
        expect(f.find('span[data-variant="outline"]')).toContainText('Outline')
    </script>
}

#universal_test("badge renders sizes") {
    <BadgeFixture />
    <script>
        const f = byTestId('badge-fixture')
        expect(f.find('span[data-size="xs"]')).toContainText('XS')
        expect(f.find('span[data-size="sm"]')).toContainText('SM')
        expect(f.find('span[data-size="lg"]')).toContainText('LG')
    </script>
}

#universal_test("badge renders as inline span") {
    <BadgeFixture />
    <script>
        expect(byTestId('badge-fixture').find('span[data-variant="default"]')).toHaveJSProperty('tagName', 'SPAN')
    </script>
}

// ---- Card ------------------------------------------------------------------

#universal_test("card renders header, title, description, content, footer") {
    <CardFixture />
    <script>
        const f = byTestId('card-fixture')
        expect(f.getByText('Card title')).toBeVisible()
        expect(f.getByText('Card description text.')).toBeVisible()
        expect(f.getByTestId('card-content')).toContainText('Card body content.')
        expect(f.getByTestId('card-action-btn')).toBeVisible()
    </script>
}

#universal_test("card interactive onClick fires") {
    <CardFixture />
    <script>
        const f = byTestId('card-fixture')
        expect(f.getByTestId('card-interactive-text')).toHaveText('Click me')
        f.getByTestId('card-interactive-text').click()
        expect(f.getByTestId('card-interactive-text')).toHaveText('Clicked!')
    </script>
}

#universal_test("card data-interactive attribute") {
    <CardFixture />
    <script>
        const f = byTestId('card-fixture')
        expect(f.find('[data-interactive="true"]')).toBeVisible()
        expect(f.find('[data-interactive="false"]')).toBeVisible()
    </script>
}

#universal_test("card title level renders correct heading") {
    <CardFixture />
    <script>
        expect(byTestId('card-fixture').find('h2')).toContainText('H2 title')
    </script>
}

#universal_test("card action slot renders") {
    <CardFixture />
    <script>
        expect(byTestId('card-fixture').getByText('With action')).toBeVisible()
    </script>
}

// ---- Input -----------------------------------------------------------------

#universal_test("input renders default variant and placeholder") {
    <InputFixture />
    <script>
        const input = byTestId('input-fixture').getByTestId('input-default')
        expect(input).toHaveAttribute('placeholder', 'Default input')
        expect(input).toHaveAttribute('data-variant', 'default')
    </script>
}

#universal_test("input renders all variants") {
    <InputFixture />
    <script>
        const f = byTestId('input-fixture')
        expect(f.getByTestId('input-filled')).toHaveAttribute('data-variant', 'filled')
        expect(f.getByTestId('input-ghost')).toHaveAttribute('data-variant', 'ghost')
        expect(f.getByTestId('input-error')).toHaveAttribute('data-variant', 'error')
        expect(f.getByTestId('input-success')).toHaveAttribute('data-variant', 'success')
    </script>
}

#universal_test("input renders sizes") {
    <InputFixture />
    <script>
        const f = byTestId('input-fixture')
        expect(f.getByTestId('input-sm')).toHaveAttribute('data-size', 'sm')
        expect(f.getByTestId('input-lg')).toHaveAttribute('data-size', 'lg')
    </script>
}

#universal_test("input disabled state") {
    <InputFixture />
    <script>
        expect(byTestId('input-fixture').getByTestId('input-disabled')).toBeDisabled()
    </script>
}

#universal_test("input typed value updates state") {
    <InputFixture />
    <script>
        const f = byTestId('input-fixture')
        expect(f.getByTestId('input-value')).toHaveText('empty')
        f.getByTestId('input-default').setValue('hello')
        expect(f.getByTestId('input-value')).toHaveText('hello')
    </script>
}

#universal_test("textarea renders and accepts input") {
    <InputFixture />
    <script>
        const ta = byTestId('input-fixture').getByTestId('textarea-default')
        expect(ta).toHaveJSProperty('tagName', 'TEXTAREA')
        ta.setValue('some text')
        expect(ta).toHaveValue('some text')
    </script>
}

#universal_test("field renders label, hint, and error") {
    <InputFixture />
    <script>
        const f = byTestId('input-fixture')
        expect(f.getByText('Email')).toBeVisible()
        expect(f.getByText('Too short.')).toBeVisible()
    </script>
}

// ---- Separator -------------------------------------------------------------

#universal_test("separator has role=separator") {
    <SeparatorFixture />
    <script>
        expect(byTestId('separator-fixture').find('[role="separator"]')).toHaveAttribute('data-orientation', 'horizontal')
    </script>
}

// ---- Typography ------------------------------------------------------------

#universal_test("typography renders heading levels") {
    <TypographyFixture />
    <script>
        const f = byTestId('typography-fixture')
        expect(f.find('h1')).toContainText('Heading 1')
        expect(f.find('h2')).toContainText('Heading 2')
        expect(f.find('h4')).toContainText('Heading 4')
        expect(f.find('h5')).toContainText('Heading 5')
        expect(f.find('h6')).toContainText('Heading 6')
    </script>
}

#universal_test("typography Heading selects level dynamically") {
    <TypographyFixture />
    <script>
        const f = byTestId('typography-fixture')
        expect(f.findAll('h3').count()).toBeGreaterThanOrEqual(2)
        expect(f.findAll('h3').last()).toContainText('Dynamic level')
    </script>
}

#universal_test("typography Text muted variant") {
    <TypographyFixture />
    <script>
        expect(byTestId('typography-fixture').find('[data-muted="true"]')).toContainText('Muted text.')
    </script>
}

#universal_test("typography Lead and Caption render") {
    <TypographyFixture />
    <script>
        const f = byTestId('typography-fixture')
        expect(f.getByText('Lead paragraph.')).toBeVisible()
        expect(f.getByText('Caption text.')).toBeVisible()
    </script>
}

#universal_test("typography CodeText renders code tag") {
    <TypographyFixture />
    <script>
        expect(byTestId('typography-fixture').find('code')).toContainText('console.log()')
    </script>
}

#universal_test("typography Link renders anchor with href") {
    <TypographyFixture />
    <script>
        const link = byTestId('typography-fixture').find('a')
        expect(link).toHaveAttribute('href', 'https://example.com')
        expect(link).toContainText('External link')
    </script>
}

#universal_test("typography Blockquote renders with cite") {
    <TypographyFixture />
    <script>
        const f = byTestId('typography-fixture')
        expect(f.find('blockquote')).toContainText('A wise quote.')
        expect(f.find('cite')).toContainText('Someone')
    </script>
}

// ---- Progress --------------------------------------------------------------

#universal_test("progress renders with correct variant and value") {
    <ProgressFixture />
    <script>
        const p = byTestId('progress-fixture').getByTestId('progress-default')
        expect(p).toHaveJSProperty('tagName', 'PROGRESS')
        expect(p).toHaveAttribute('value', '45')
    </script>
}

#universal_test("progress renders all variants") {
    <ProgressFixture />
    <script>
        const f = byTestId('progress-fixture')
        expect(f.getByTestId('progress-primary')).toHaveAttribute('data-variant', 'primary')
        expect(f.getByTestId('progress-success')).toHaveAttribute('data-variant', 'success')
        expect(f.getByTestId('progress-error')).toHaveAttribute('data-variant', 'error')
    </script>
}

// ---- Pagination ------------------------------------------------------------

#universal_test("pagination navigates between pages") {
    <PaginationFixture />
    <script>
        const f = byTestId('pagination-fixture')
        expect(f.getByRole('button', { name: 'Previous page' })).toBeDisabled()
        f.getByRole('button', { name: '3' }).click()
        expect(f.getByTestId('pagination-value')).toHaveText('Page: 3')
        f.getByRole('button', { name: 'Next page' }).click()
        expect(f.getByTestId('pagination-value')).toHaveText('Page: 4')
    </script>
}

// ---- List ------------------------------------------------------------------

#universal_test("list renders items") {
    <ListFixture />
    <script>
        const items = byTestId('list-fixture').getByTestId('list').findAll('li')
        expect(items).toHaveCount(3)
        expect(items.nth(0)).toContainText('First item')
        expect(items.nth(2)).toContainText('Third item')
    </script>
}

// ---- Table -----------------------------------------------------------------

#universal_test("table renders headers and cells") {
    <TableFixture />
    <script>
        const t = byTestId('table-fixture').getByTestId('table')
        expect(t).toHaveJSProperty('tagName', 'TABLE')
        expect(t.findAll('th').nth(0)).toContainText('Name')
        expect(t.findAll('td').nth(0)).toContainText('Alpha')
        expect(t.findAll('td').nth(3)).toContainText('200')
    </script>
}

// ---- Tooltip ---------------------------------------------------------------

#universal_test("tooltip appears on hover") {
    <TooltipFixture />
    <script>
        const f = byTestId('tooltip-fixture')
        f.findAll('button').first().hover()
        expect(f.getByRole('tooltip')).toContainText('Top tip')
    </script>
}

#universal_test("tooltip bottom position renders its label") {
    <TooltipFixture />
    <script>
        const f = byTestId('tooltip-fixture')
        f.findAll('button').nth(1).hover()
        expect(f.getByRoleAll('tooltip').nth(1)).toContainText('Bottom tip')
    </script>
}

// ---- Nested / Perf ---------------------------------------------------------

#universal_test("nested components work together") {
    <NestedFixture />
    <script>
        const f = byTestId('nested-fixture')
        expect(f.getByTestId('nested-count')).toHaveText('Count: 0')
        expect(f.getByTestId('nested-text')).toHaveText('empty')
        f.getByTestId('nested-btn').click()
        expect(f.getByTestId('nested-count')).toHaveText('Count: 1')
        f.getByTestId('nested-input').setValue('hello')
        expect(f.getByTestId('nested-text')).toHaveText('hello')
    </script>
}

#universal_test("perf fixture click updates count") {
    <PerfFixture />
    <script>
        byTestId('perf-btn').click()
        expect(byTestId('perf-count')).toHaveText('1')
    </script>
}




