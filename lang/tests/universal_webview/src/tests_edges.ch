// Ported from components-e2e: edge-case fixtures (components.spec.ts) and the
// utility/controlled components. Portal-using tests are `isolate`.

// ---- Button edge -----------------------------------------------------------

#universal_test("button edge: submit type, aria-label, disabled, fab") {
    <ButtonEdgeFixture />
    <script>
        const f = byTestId('button-edge-fixture')
        expect(f.getByTestId('btn-submit')).toHaveAttribute('type', 'submit')
        expect(f.getByTestId('btn-aria')).toHaveAttribute('aria-label', 'Close dialog')
        expect(f.getByTestId('btn-disabled-interactive')).toBeDisabled()
        expect(f.find('[aria-label="Add item"]')).toBeVisible()
        expect(f.find('[aria-label="Disabled"]')).toBeDisabled()
        f.getByTestId('btn-submit').click()
        expect(f.getByTestId('btn-submit-state')).toHaveText('submitted')
    </script>
}

// ---- Input edge ------------------------------------------------------------

#universal_test("input edge: types and typed values") {
    <InputEdgeFixture />
    <script>
        const f = byTestId('input-edge-fixture')
        expect(f.getByTestId('input-email')).toHaveAttribute('type', 'email')
        expect(f.getByTestId('input-number')).toHaveAttribute('type', 'number')
        expect(f.getByTestId('input-password')).toHaveAttribute('type', 'password')
        expect(f.getByTestId('input-search')).toHaveAttribute('type', 'search')
        expect(f.getByTestId('textarea-rows')).toHaveAttribute('rows', '5')
        f.getByTestId('input-email').setValue('a@b.com')
        expect(f.getByTestId('input-email-value')).toHaveText('a@b.com')
    </script>
}

#universal_test("input edge: native select accepts selection") {
    <InputEdgeFixture />
    <script>
        const f = byTestId('input-edge-fixture')
        expect(f.getByTestId('native-select')).toBeVisible()
        f.getByTestId('native-select').setValue('b')
        expect(f.getByTestId('native-select-value')).toHaveText('b')
    </script>
}

// ---- Select edge (isolate: portal) -----------------------------------------

#universal_test("select edge: controlled value and disabled", isolate) {
    <SelectEdgeFixture />
    <script>
        const f = byTestId('select-edge-fixture')
        expect(f.getByTestId('select-controlled-value')).toHaveText('Apple')
        f.getByTestId('select-disabled').find('button').click()
        expect(byRole('listbox')).toBeHidden()
        f.getByTestId('select-empty').find('button').click()
    </script>
}

#universal_test("select edge: sizes render") {
    <SelectEdgeFixture />
    <script>
        const f = byTestId('select-edge-fixture')
        expect(f.getByTestId('select-sm').find('button')).toHaveAttribute('data-size', 'sm')
        expect(f.getByTestId('select-lg').find('button')).toHaveAttribute('data-size', 'lg')
    </script>
}

// ---- Slider edge -----------------------------------------------------------

#universal_test("slider edge: disabled, custom range, controlled") {
    <SliderEdgeFixture />
    <script>
        const f = byTestId('slider-edge-fixture')
        expect(f.findAll('[data-disabled="true"]').count()).toBeGreaterThanOrEqual(1)
        expect(f.getByRole('slider', { name: 'Custom range' })).toHaveAttribute('aria-valuemin', '10')
        expect(f.getByRole('slider', { name: 'Custom range' })).toHaveAttribute('aria-valuemax', '20')
        const controlled = f.getByRole('slider', { name: 'Controlled' })
        controlled.focus()
        controlled.press('ArrowRight')
        expect(f.getByTestId('slider-controlled-value')).toHaveText('55')
    </script>
}

// ---- Toast edge (isolate: portaled) ----------------------------------------

#universal_test("toast edge: variants, persistent, manual close", isolate) {
    <ToastEdgeFixture />
    <script>
        expect(byTestId('toast-success')).toContainText('Success toast')
        expect(byTestId('toast-destructive')).toContainText('Error toast')
        await t.sleep(1100)
        expect(byTestId('toast-success')).toBeHidden()
        expect(byTestId('toast-destructive')).toBeHidden()
        // duration=0 must not auto-dismiss
        expect(byTestId('toast-persistent')).toBeVisible()
    </script>
}

// ---- Toggle edge -----------------------------------------------------------

#universal_test("toggle edge: disabled, sizes, aria-label") {
    <ToggleEdgeFixture />
    <script>
        const f = byTestId('toggle-edge-fixture')
        expect(f.getByTestId('cb-disabled').find('input')).toBeDisabled()
        expect(f.getByTestId('cb-sm')).toHaveAttribute('data-size', 'sm')
        expect(f.getByTestId('cb-aria').find('input')).toHaveAttribute('aria-label', 'Accept terms')
        expect(f.getByTestId('sw-disabled').find('input')).toBeDisabled()
    </script>
}

// ---- Collapsible edge ------------------------------------------------------

#universal_test("collapsible edge: defaultOpen, disabled, callback") {
    <CollapsibleEdgeFixture />
    <script>
        const f = byTestId('collapsible-edge-fixture')
        expect(f.findAll('button').nth(0)).toHaveAttribute('aria-expanded', 'true')
        expect(f).toContainText('Always visible content.')
        expect(f.getByTestId('collapsible-callback-state')).toHaveText('closed')
        f.findAll('button').nth(2).click()
        expect(f.getByTestId('collapsible-callback-state')).toHaveText('open')
    </script>
}

// ---- Dialog edge (isolate: portal) -----------------------------------------

#universal_test("dialog edge: controlled open/close and aria-label", isolate) {
    <DialogEdgeFixture />
    <script>
        const f = byTestId('dialog-edge-fixture')
        expect(f.getByTestId('dialog-edge-state')).toHaveText('closed')
        f.getByTestId('dialog-edge-open').click()
        expect(byTestId('dialog-edge-content')).toBeVisible()
        expect(byRole('dialog')).toHaveAttribute('aria-label', 'Edge dialog')
        byTestId('dialog-edge-close').click()
        expect(byTestId('dialog-edge-content')).toBeHidden()
        expect(f.getByTestId('dialog-edge-state')).toHaveText('closed')
    </script>
}

// ---- Accordion edge --------------------------------------------------------

#universal_test("accordion edge: disabled item cannot open") {
    <AccordionEdgeFixture />
    <script>
        const f = byTestId('accordion-edge-fixture')
        const disabledTrigger = f.getByTestId('acc-disabled').find('button')
        expect(disabledTrigger).toBeDisabled()
    </script>
}

// ---- Tabs edge -------------------------------------------------------------

#universal_test("tabs edge: onChange fires") {
    <TabsEdgeFixture />
    <script>
        const f = byTestId('tabs-edge-fixture')
        expect(f.getByTestId('tabs-edge-last')).toHaveText('0')
        f.getByRole('tab', { name: 'Two' }).click()
        expect(f.getByTestId('tabs-edge-last')).toHaveText('1')
    </script>
}

// ---- Pagination edge -------------------------------------------------------

#universal_test("pagination edge: last page disables next") {
    <PaginationEdgeFixture />
    <script>
        const f = byTestId('pagination-edge-fixture')
        expect(f.getByRole('button', { name: 'Next page' })).toBeDisabled()
        expect(f.getByRole('button', { name: 'Previous page' })).toBeEnabled()
    </script>
}

// ---- Controlled components -------------------------------------------------

#universal_test("toggle group controlled: parent drives selection") {
    <ToggleGroupControlledFixture />
    <script>
        const f = byTestId('togglegroup-controlled-fixture')
        expect(f.getByTestId('tg-ctrl-value')).toHaveText('Bold')
        f.getByRole('button', { name: 'Italic' }).click()
        expect(f.getByTestId('tg-ctrl-value')).toHaveText('Italic')
    </script>
}

#universal_test("radio group controlled: parent drives selection") {
    <RadioGroupControlledFixture />
    <script>
        const f = byTestId('radiogroup-controlled-fixture')
        expect(f.getByTestId('rg-ctrl-value')).toHaveText('small')
        f.getByRole('radio', { name: 'Large' }).check()
        expect(f.getByTestId('rg-ctrl-value')).toHaveText('large')
    </script>
}

#universal_test("collapsible controlled: external toggle drives it") {
    <CollapsibleControlledFixture />
    <script>
        const f = byTestId('collapsible-controlled-fixture')
        expect(f.getByTestId('collapsible-ctrl-state')).toHaveText('closed')
        f.getByTestId('collapsible-ctrl-toggle').click()
        expect(f.getByTestId('collapsible-ctrl-state')).toHaveText('open')
    </script>
}

#universal_test("toggle sizes render data-size") {
    <ToggleSizesFixture />
    <script>
        const f = byTestId('toggle-sizes-fixture')
        expect(f.getByTestId('cb-sm')).toHaveAttribute('data-size', 'sm')
        expect(f.getByTestId('cb-lg')).toHaveAttribute('data-size', 'lg')
    </script>
}

#universal_test("text polymorphic renders the requested tag") {
    <TextPolymorphicFixture />
    <script>
        const f = byTestId('text-polymorphic-fixture')
        expect(f.getByText('Paragraph')).toHaveJSProperty('tagName', 'P')
        expect(f.getByText('Span text')).toHaveJSProperty('tagName', 'SPAN')
        expect(f.getByText('Div text')).toHaveJSProperty('tagName', 'DIV')
    </script>
}

// ---- Utility / layout ------------------------------------------------------

#universal_test("container variants render") {
    <ContainerFixture />
    <script>
        const f = byTestId('container-fixture')
        expect(f.getByTestId('container-sm')).toBeVisible()
        expect(f.getByTestId('container-md')).toBeVisible()
        expect(f.getByTestId('container-default')).toBeVisible()
        expect(f.getByTestId('container-full')).toBeVisible()
    </script>
}

#universal_test("stack and grid render children") {
    <div>
        <StackFixture />
        <GridFixture />
    </div>
    <script>
        expect(byTestId('stack-fixture')).toContainText('Row A')
        expect(byTestId('stack-fixture')).toContainText('Col A')
        expect(byTestId('grid-fixture')).toContainText('Cell 1')
    </script>
}

#universal_test("breadcrumbs, divider and kbd render") {
    <div>
        <BreadcrumbsFixture />
        <DividerFixture />
        <KbdFixture />
    </div>
    <script>
        expect(byTestId('breadcrumbs-fixture')).toContainText('Home')
        expect(byTestId('breadcrumbs-fixture')).toContainText('Components')
        expect(byTestId('divider-fixture')).toContainText('Above')
        expect(byTestId('kbd-ctrl')).toHaveText('Ctrl')
    </script>
}

#universal_test("skeleton and spinner render") {
    <div>
        <SkeletonFixture />
        <SpinnerFixture />
    </div>
    <script>
        expect(byTestId('skeleton-rect')).toBeVisible()
        expect(byRole('status', { name: 'Loading data' })).toBeVisible()
    </script>
}

#universal_test("surface components render") {
    <div>
        <PaperFixture />
        <AppBarFixture />
        <DrawerFixture />
        <SnackbarFixture />
        <IconFixture />
        <BottomBarFixture />
        <EmptyStateFixture />
        <StatCardFixture />
    </div>
    <script>
        expect(byTestId('paper')).toContainText('Paper content')
        expect(byTestId('appbar')).toContainText('Logo')
        expect(byTestId('drawer')).toContainText('Drawer content')
        expect(byTestId('snackbar')).toContainText('Saved!')
        expect(byTestId('icon')).toHaveText('A')
        expect(byTestId('bottombar')).toContainText('Left')
        expect(byTestId('emptystate')).toContainText('No data found')
        expect(byTestId('statcard')).toContainText('Revenue')
    </script>
}
