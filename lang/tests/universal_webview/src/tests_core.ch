// Ported from components-e2e: counter.spec.ts, ssr.spec.ts, root-shapes.spec.ts,
// static-children.spec.ts.
//
// SSR-only assertions (Playwright runs these with JavaScript disabled) are
// expressed here against the hydrated DOM; the content must be identical after
// hydration adoption.

// ---- counter.spec.ts -------------------------------------------------------

#universal_test("counter hydrates and responds to clicks") {
    <CounterFixture />
    <script>
        expect(byTestId('counter-value').text()).toBe('Count: 0')
        byTestId('counter-increment').click()
        expect(byTestId('counter-value').text()).toBe('Count: 1')
        byTestId('counter-increment').click()
        byTestId('counter-increment').click()
        expect(byTestId('counter-value').text()).toBe('Count: 3')
        byTestId('counter-reset').click()
        expect(byTestId('counter-value').text()).toBe('Count: 0')
    </script>
}

// ---- ssr.spec.ts -----------------------------------------------------------

#universal_test("ssr keyed list renders items") {
    <KeyedListFixture />
    <script>
        expect(byTestId('keyed-list').findAll('li').count()).toBe(3)
        expect(byTestId('keyed-list').containsText('Alpha')).toBe(true)
        expect(byTestId('keyed-list').containsText('Beta')).toBe(true)
        expect(byTestId('keyed-list').containsText('Gamma')).toBe(true)
    </script>
}

#universal_test("ssr props-derived filter list renders") {
    <PropsDerivedFixture />
    <script>
        expect(byTestId('props-derived-list').findAll('li').count()).toBe(2)
        expect(byTestId('props-derived-list').containsText('Apple')).toBe(true)
        expect(byTestId('props-derived-list').containsText('Banana')).toBe(true)
    </script>
}

#universal_test("ssr derived filter list renders") {
    <DerivedListProbe />
    <script>
        expect(byTestId('probe-list').findAll('li').count()).toBe(3)
        expect(byTestId('probe-list').containsText('Apple')).toBe(true)
        expect(byTestId('probe-count').text()).toBe('3')
    </script>
}

#universal_test("ssr keyed component list rows resolve props") {
    <KeyedComponentListFixture />
    <script>
        expect(byTestId('keyed-comp-list').findAll('li').count()).toBe(3)
        expect(byTestId('crow-label-a').text()).toBe('Alpha')
        expect(byTestId('crow-label-b').text()).toBe('Beta')
        expect(byTestId('crow-label-c').text()).toBe('Gamma')
    </script>
}

#universal_test("ssr static component content renders") {
    <div>
        <AccordionFixture />
        <TabsFixture />
    </div>
    <script>
        expect(byTestId('accordion-fixture').containsText('What is Chemical')).toBe(true)
        expect(byRole('tabpanel').text()).toContain('Panel A')
    </script>
}

// ---- root-shapes.spec.ts ---------------------------------------------------

#universal_test("root shapes: fragment-root component renders all nodes") {
    <RootShapesFixture />
    <script>
        expect(byTestId('root-shapes-fixture').getByTestId('frag-one').text()).toBe('one')
        expect(byTestId('root-shapes-fixture').getByTestId('frag-two').text()).toBe('two')
        expect(byTestId('root-shapes-fixture').getByTestId('root-shapes-next').text()).toBe('next')
        expect(byTestId('root-shapes-fixture').getByTestIdAll('frag-one').count()).toBe(1)
        expect(byTestId('root-shapes-fixture').getByTestIdAll('frag-two').count()).toBe(1)
    </script>
}

#universal_test("root shapes: fresh fragment-root mount after toggle") {
    <RootShapesFixture />
    <script>
        byTestId('root-shapes-toggle').click()
        expect(byTestId('root-shapes-fixture').getByTestIdAll('frag-one').count()).toBe(2)
        expect(byTestId('root-shapes-fixture').getByTestIdAll('frag-two').count()).toBe(2)
        expect(byTestId('root-shapes-fixture').getByTestId('root-shapes-next').text()).toBe('next')
    </script>
}

// ---- static-children.spec.ts ----------------------------------------------

#universal_test("static children render and are adopted without duplication") {
    <StaticChildrenHost>
        <span data-testid="sc-child">hello</span>
        <em data-testid="sc-child2" data-n="2">world</em>
    </StaticChildrenHost>
    <script>
        expect(byTestId('static-children-host').getByTestIdAll('sc-child').count()).toBe(1)
        expect(byTestId('static-children-host').getByTestIdAll('sc-child2').count()).toBe(1)
        expect(byTestId('sc-child').text()).toBe('hello')
        expect(byTestId('sc-child2').text()).toBe('world')
        expect(byTestId('sc-child2').attr('data-n')).toBe('2')
    </script>
}
