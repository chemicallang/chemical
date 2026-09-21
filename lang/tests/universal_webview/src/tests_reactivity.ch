// Ported from components-e2e/tests/reactivity.spec.ts — rapid interactions,
// hydration duplication, and ARIA contracts.

// ---- Rapid interactions ----------------------------------------------------

#universal_test("tabs: clicking the same tab twice does not crash") {
    <TabsFixture />
    <script>
        const f = byTestId('tabs-fixture')
        const alpha = f.getByRole('tab', { name: 'Alpha' })
        alpha.click()
        alpha.click()
        expect(f.getByRole('tabpanel', { name: 'Alpha' })).toBeVisible()
    </script>
}

#universal_test("toggle group: rapid clicks keep a single selection") {
    <ToggleGroupFixture />
    <script>
        const f = byTestId('togglegroup-fixture')
        const bold = f.getByRole('button', { name: 'Bold' })
        const italic = f.getByRole('button', { name: 'Italic' })
        for(let i = 0; i < 5; i++) { italic.click() }
        expect(italic).toHaveAttribute('aria-pressed', 'true')
        expect(bold).toHaveAttribute('aria-pressed', 'false')
    </script>
}

#universal_test("dialog: multiple open/close cycles", isolate) {
    <DialogFixture />
    <script>
        const f = byTestId('dialog-fixture')
        for(let i = 0; i < 3; i++) {
            f.getByTestId('dialog-open').click()
            await t.sleep(20)
            expect(byTestId('dialog-content')).toBeVisible()
            byTestId('dialog-confirm').click()
            await t.sleep(20)
            expect(byTestId('dialog-content')).toBeHidden()
        }
    </script>
}

#universal_test("no runtime errors after an interaction sequence") {
    <div>
        <CounterFixture />
        <TabsFixture />
        <AccordionFixture />
        <ToggleFixture />
    </div>
    <script>
        byTestId('counter-increment').click()
        byTestId('counter-increment').click()
        expect(byTestId('counter-value')).toHaveText('Count: 2')
        byTestId('tabs-fixture').getByRole('tab', { name: 'Beta' }).click()
        byTestId('accordion-fixture').findAll('button').nth(0).click()
        byTestId('checkbox-control').find('input').click()
        await t.sleep(30)
        // The harness fails the test on any uncaught error; reaching here means none.
        expect(true).toBe(true)
    </script>
}

// ---- Hydration correctness -------------------------------------------------

#universal_test("hydration does not duplicate elements") {
    <div>
        <CounterFixture />
        <BadgeFixture />
    </div>
    <script>
        expect(byTestIdAll('counter-value').count()).toBe(1)
        expect(byTestIdAll('counter-increment').count()).toBe(1)
        expect(byTestId('counter-value')).toHaveText('Count: 0')
    </script>
}

#universal_test("hydration attaches click handlers") {
    <CounterFixture />
    <script>
        byTestId('counter-increment').click()
        expect(byTestId('counter-value')).toHaveText('Count: 1')
    </script>
}

// ---- ARIA contracts --------------------------------------------------------

#universal_test("tabs expose tablist/tab/tabpanel roles") {
    <TabsFixture />
    <script>
        const f = byTestId('tabs-fixture')
        expect(f.getByRoleAll('tab').count()).toBe(3)
        expect(f.getByRoleAll('tabpanel').count()).toBeGreaterThanOrEqual(1)
    </script>
}

#universal_test("radio group exposes the radiogroup role") {
    <RadioGroupFixture />
    <script>
        expect(byTestId('radiogroup-fixture').getByRole('radiogroup')).toBeVisible()
    </script>
}

#universal_test("accordion triggers expose aria-expanded") {
    <AccordionFixture />
    <script>
        const f = byTestId('accordion-fixture')
        const trigger = f.getByTestId('acc-item-0').find('button')
        expect(trigger).toHaveAttribute('aria-expanded', 'false')
        trigger.click()
        expect(trigger).toHaveAttribute('aria-expanded', 'true')
    </script>
}

#universal_test("toggle group exposes aria-pressed defaults") {
    <ToggleGroupFixture />
    <script>
        const f = byTestId('togglegroup-fixture')
        expect(f.getByRole('button', { name: 'Bold' })).toHaveAttribute('aria-pressed', 'true')
        expect(f.getByRole('button', { name: 'Italic' })).toHaveAttribute('aria-pressed', 'false')
        expect(f.getByRole('button', { name: 'Underline' })).toHaveAttribute('aria-pressed', 'false')
    </script>
}
