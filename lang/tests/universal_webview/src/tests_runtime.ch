// Ported from components-e2e/tests/runtime.spec.ts (batching, unmount cleanup,
// effect deps, derived lists, keyed reconciliation, error boundaries,
// memoization, SVG namespace, ref forwarding, Suspense).

// ---- Batching --------------------------------------------------------------

#universal_test("batching: multiple state updates in one handler") {
    <BatchingFixture />
    <script>
        const f = byTestId('batching-fixture')
        expect(f.getByTestId('batch-a')).toHaveText('0')
        f.getByTestId('batch-update').click()
        expect(f.getByTestId('batch-a')).toHaveText('10')
        expect(f.getByTestId('batch-b')).toHaveText('20')
        expect(f.getByTestId('batch-c')).toHaveText('30')
    </script>
}

#universal_test("batching: rapid clicks do not lose state") {
    <BatchingFixture />
    <script>
        const f = byTestId('batching-fixture')
        for(let i = 0; i < 5; i++) { f.getByTestId('batch-update').click() }
        expect(f.getByTestId('batch-a')).toHaveText('10')
        expect(f.getByTestId('batch-b')).toHaveText('20')
        expect(f.getByTestId('batch-c')).toHaveText('30')
    </script>
}

#universal_test("batching: mutations are batched") {
    <BatchingFixture />
    <script>
        window.__mutationCount = 0
        const obs = new MutationObserver(() => { window.__mutationCount++ })
        obs.observe(document.body, { childList: true, subtree: true, characterData: true })
        byTestId('batching-fixture').getByTestId('batch-update').click()
        await t.sleep(50)
        obs.disconnect()
        expect(window.__mutationCount).toBeLessThanOrEqual(5)
    </script>
}

// ---- Unmount cleanup -------------------------------------------------------

#universal_test("unmount: toggle hides and shows the child") {
    <UnmountCleanupFixture />
    <script>
        const f = byTestId('unmount-fixture')
        expect(f.getByTestId('unmount-child')).toBeVisible()
        f.getByTestId('unmount-toggle').click()
        expect(f.getByTestId('unmount-child')).toBeHidden()
        expect(f.getByTestId('unmount-showing')).toHaveText('no')
        f.getByTestId('unmount-toggle').click()
        expect(f.getByTestId('unmount-child')).toBeVisible()
    </script>
}

#universal_test("unmount: useEffect cleanup runs when child is removed") {
    <UnmountCleanupFixture />
    <script>
        const f = byTestId('unmount-fixture')
        expect(f.getByTestId('unmount-child')).toBeVisible()
        expect(window.__childMounted).toBe(true)
        window.__cleanupRan = false
        f.getByTestId('unmount-toggle').click()
        expect(f.getByTestId('unmount-child')).toBeHidden()
        await t.sleep(60)
        expect(window.__cleanupRan).toBe(true)
    </script>
}

// ---- Effect deps -----------------------------------------------------------

#universal_test("effect deps: unrelated state change does not re-run effect") {
    <EffectDepsFixture />
    <script>
        const f = byTestId('effect-deps-fixture')
        expect(f.getByTestId('ed-runs')).toHaveText('1')
        f.getByTestId('ed-inc-unrelated').click()
        expect(f.getByTestId('ed-unrelated')).toHaveText('1')
        expect(f.getByTestId('ed-runs')).toHaveText('1')
        f.getByTestId('ed-inc-count').click()
        expect(f.getByTestId('ed-count')).toHaveText('1')
        expect(f.getByTestId('ed-runs')).toHaveText('2')
    </script>
}

// ---- Derived lists ---------------------------------------------------------

#universal_test("derived list: filtered array recomputes reactively") {
    <DerivedListProbe />
    <script>
        const f = byTestId('derived-list-probe')
        expect(f.getByTestId('probe-count')).toHaveText('3')
        f.getByTestId('probe-input').setValue('an')
        expect(f.getByTestId('probe-count')).toHaveText('1')
        expect(f.getByTestId('probe-Banana')).toBeVisible()
        expect(f.getByTestIdAll('probe-Apple').count()).toBe(0)
        f.getByTestId('probe-input').setValue('')
        expect(f.getByTestId('probe-count')).toHaveText('3')
    </script>
}

#universal_test("derived list from props: parent state updates propagate") {
    <PropsDerivedFixture />
    <script>
        const f = byTestId('props-derived-fixture')
        expect(f.findAll('li[data-testid^="pdl-"]').count()).toBe(2)
        f.getByTestId('pdl-input').setValue('an')
        expect(f.findAll('li[data-testid^="pdl-"]').count()).toBe(1)
        f.getByTestId('pdl-input').setValue('')
        f.getByTestId('pdl-add').click()
        expect(f.findAll('li[data-testid^="pdl-"]').count()).toBe(3)
    </script>
}

// ---- Keyed lists -----------------------------------------------------------

#universal_test("keyed list: SSR renders initial items") {
    <KeyedListFixture />
    <script>
        const list = byTestId('keyed-list')
        expect(list.getByTestId('item-a')).toHaveText('Alpha')
        expect(list.getByTestId('item-b')).toHaveText('Beta')
        expect(list.getByTestId('item-c')).toHaveText('Gamma')
    </script>
}

#universal_test("keyed list: add to end preserves existing items") {
    <KeyedListFixture />
    <script>
        const list = byTestId('keyed-list')
        byTestId('keyed-add-delta').click()
        expect(list.getByTestId('item-a')).toHaveText('Alpha')
        expect(list.getByTestId('item-b')).toHaveText('Beta')
        expect(list.getByTestId('item-c')).toHaveText('Gamma')
        expect(list.getByTestId('item-d')).toHaveText('Delta')
    </script>
}

#universal_test("keyed list: remove from middle preserves remaining items") {
    <KeyedListFixture />
    <script>
        const list = byTestId('keyed-list')
        byTestId('keyed-remove-b').click()
        expect(list.getByTestId('item-a')).toHaveText('Alpha')
        expect(list.getByTestIdAll('item-b').count()).toBe(0)
        expect(list.getByTestId('item-c')).toHaveText('Gamma')
    </script>
}

#universal_test("keyed list: reverse reorders items correctly") {
    <KeyedListFixture />
    <script>
        byTestId('keyed-reverse').click()
        const items = byTestId('keyed-list').findAll('li')
        expect(items.nth(0)).toHaveText('Gamma')
        expect(items.nth(1)).toHaveText('Beta')
        expect(items.nth(2)).toHaveText('Alpha')
    </script>
}

#universal_test("keyed list: replace all clears and renders new items") {
    <KeyedListFixture />
    <script>
        const list = byTestId('keyed-list')
        byTestId('keyed-replace-all').click()
        expect(list.getByTestIdAll('item-a').count()).toBe(0)
        expect(list.getByTestIdAll('item-b').count()).toBe(0)
        expect(list.getByTestIdAll('item-c').count()).toBe(0)
        expect(list.getByTestId('item-x')).toHaveText('X-ray')
        expect(list.getByTestId('item-y')).toHaveText('Yankee')
    </script>
}

#universal_test("keyed component list: reorder preserves item state and order") {
    <KeyedComponentListFixture />
    <script>
        const f = byTestId('keyed-comp-fixture')
        const list = f.getByTestId('keyed-comp-list')
        f.getByTestId('crow-inc-a').click()
        f.getByTestId('crow-inc-a').click()
        expect(f.getByTestId('crow-hits-a')).toHaveText('2')
        f.getByTestId('keyed-comp-reverse').click()
        const rows = list.findAll('li')
        expect(rows.nth(0)).toHaveAttribute('data-testid', 'crow-c')
        expect(rows.nth(1)).toHaveAttribute('data-testid', 'crow-b')
        expect(rows.nth(2)).toHaveAttribute('data-testid', 'crow-a')
        expect(f.getByTestId('crow-hits-a')).toHaveText('2')
        f.getByTestId('keyed-comp-relabel-a').click()
        expect(f.getByTestId('crow-label-a')).toHaveText('Alpha2')
    </script>
}

// ---- Error boundary hierarchy ---------------------------------------------

#universal_test("error boundary: child error shows parent fallback") {
    <ErrorBoundaryChildFixture />
    <script>
        const eb = byTestId('eb-parent')
        expect(eb.getByTestId('parent-fallback')).toBeVisible()
        expect(eb.getByTestId('parent-fallback')).toContainText('Parent caught:')
        expect(eb.getByTestId('eb-parent-sibling')).toHaveText(' sibling content')
    </script>
}

// ---- Memoization -----------------------------------------------------------

#universal_test("memoization: initial value and updates") {
    <MemoFixture />
    <script>
        const m = byTestId('memo-fixture')
        expect(m.getByTestId('memo-label')).toHaveText('hello')
        m.getByTestId('memo-inc').click()
        m.getByTestId('memo-inc').click()
        expect(m.getByTestId('memo-counter')).toHaveText('2')
        expect(m.getByTestId('memo-label')).toHaveText('hello')
        m.getByTestId('memo-changelabel').click()
        expect(m.getByTestId('memo-label')).toHaveText('hello!')
    </script>
}

// ---- SVG namespace ---------------------------------------------------------

#universal_test("svg: elements render in the SVG namespace") {
    <SvgFixture />
    <script>
        expect(byTestId('svg-root')).toBeVisible()
        expect(byTestId('svg-root')).toHaveAttribute('viewBox', '0 0 100 100')
        expect(byTestId('svg-circle')).toHaveAttribute('cx', '50')
        expect(byTestId('svg-circle')).toHaveAttribute('r', '40')
        expect(byTestId('svg-rect')).toHaveAttribute('width', '30')
        expect(byTestId('svg-circle').jsProp('namespaceURI')).toBe('http://www.w3.org/2000/svg')
        expect(byTestId('svg-text')).toHaveText('SVG above')
    </script>
}

// ---- Ref forwarding --------------------------------------------------------

#universal_test("ref forwarding: callback receives the root element") {
    <RefForwardingFixture />
    <script>
        expect(byTestId('ref-captured-tag')).toHaveText('div')
        expect(byTestId('ref-captured-testid')).toHaveText('ref-child-root')
        expect(byTestId('ref-child-root')).toHaveText('child content')
    </script>
}

// ---- Suspense --------------------------------------------------------------

#universal_test("suspense: swaps fallback for content once loaded") {
    <SuspenseFixture />
    <script>
        await t.sleep(120)
        expect(byTestId('suspense-content')).toHaveText('Loaded data')
        expect(byTestId('suspense-fixture').findAll('.chx-suspense-fallback').count()).toBe(0)
    </script>
}
