// Exploratory probes for suspected universal runtime bugs. Each test asserts
// the *correct* behaviour; a failing test identifies a runtime bug to fix in
// lang/libs/page/src/page.ch. Do not weaken assertions to make a test pass.

// ===========================================================================
// Probe 1: writing a state signal the SAME value must not rebuild a reactive
// slot (React bails out on Object.is). If it rebuilds, the slot's DOM node is
// replaced and anything on it (focus, marker, input value) is lost.
// ===========================================================================

#universal ProbeSameValueSlot(props) {
    state n = 0
    return <div data-testid="psv">
        <button data-testid="psv-same" onClick={() => n = 0}>same</button>
        <input data-testid="psv-a" type="text" />
        {n >= 0 ? <input data-testid="psv-b" type="text" /> : null}
    </div>
}

#universal_test("probe: same-value state write keeps the reactive slot node") {
    <ProbeSameValueSlot />
    <script>
        const b = byTestId('psv-b')
        expect(b.exists()).toBeTruthy()
        b.el.__probe = 'kept'
        b.focus()
        expect(b).toBeFocused()
        byTestId('psv-same').click()
        expect(byTestId('psv-b').jsProp('__probe')).toBe('kept')
        expect(byTestId('psv-b')).toBeFocused()
    </script>
}

// ===========================================================================
// Probe 2: a layout effect must re-run (with cleanup) when its own state
// dependency changes.
// ===========================================================================

#universal ProbeLayoutOwnDeps(props) {
    state n = 0
    useLayoutEffect(() => {
        window.__pldRuns = (window.__pldRuns || 0) + 1
        return () => { window.__pldCleanups = (window.__pldCleanups || 0) + 1 }
    }, [n])
    return <button data-testid="pld" onClick={() => n += 1}>{n}</button>
}

#universal_test("probe: layout effect re-runs with cleanup on own dep change") {
    <ProbeLayoutOwnDeps />
    <script>
        await t.sleep(20)
        const runs0 = window.__pldRuns || 0
        const cleanups0 = window.__pldCleanups || 0
        expect(runs0).toBeGreaterThanOrEqual(1)
        byTestId('pld').click()
        await t.sleep(20)
        expect(window.__pldRuns).toBe(runs0 + 1)
        expect(window.__pldCleanups).toBe(cleanups0 + 1)
        expect(byTestId('pld').text()).toBe('1')
    </script>
}

// ===========================================================================
// Probe 3: a layout effect whose dependency is a PARENT-provided prop signal
// must re-run when the parent's state changes. useLayoutEffect currently never
// subscribes to its signal deps (only useEffect does), so this may be stale.
// ===========================================================================

#universal ProbeLayoutChild(props) {
    useLayoutEffect(() => {
        window.__plcRuns = (window.__plcRuns || 0) + 1
        return () => { window.__plcCleanups = (window.__plcCleanups || 0) + 1 }
    }, [props.tick])
    return <span data-testid="plc-out">{window.$__uni_value(props.tick)}</span>
}

#universal ProbeLayoutParent(props) {
    state tick = 0
    return <div data-testid="plp">
        <button data-testid="plp-btn" onClick={() => tick += 1}>b</button>
        <ProbeLayoutChild tick={tick} />
    </div>
}

#universal_test("probe: layout effect tracks a parent-provided prop dependency") {
    <ProbeLayoutParent />
    <script>
        await t.sleep(20)
        const runs0 = window.__plcRuns || 0
        expect(runs0).toBeGreaterThanOrEqual(1)
        expect(byTestId('plc-out').text()).toBe('0')
        byTestId('plp-btn').click()
        await t.sleep(20)
        expect(byTestId('plc-out').text()).toBe('1')
        expect(window.__plcRuns).toBe(runs0 + 1)
    </script>
}

// ===========================================================================
// Probe 4: CSS custom properties in a reactive style object must apply. The
// runtime sets `el.style["--x"] = v`, which is a no-op for custom properties
// (needs `setProperty`), so a `--var` binding silently vanishes.
// ===========================================================================

#universal ProbeCssVar(props) {
    state n = 1
    return <div data-testid="pcv" style={{ "--pcv-w": n + "px", width: n + "px" }}>
        <button data-testid="pcv-btn" onClick={() => n = 2}>b</button>
    </div>
}

#universal_test("probe: reactive style CSS custom property applies and updates") {
    <ProbeCssVar />
    <script>
        const el = byTestId('pcv').el
        expect(el.style.getPropertyValue('--pcv-w')).toBe('1px')
        expect(el.style.width).toBe('1px')
        byTestId('pcv-btn').click()
        expect(el.style.getPropertyValue('--pcv-w')).toBe('2px')
        expect(el.style.width).toBe('2px')
    </script>
}

// ===========================================================================
// Probe 5: onDoubleClick must map to the DOM `dblclick` event. The runtime
// lowercases the suffix (`doubleclick`) which never fires.
// ===========================================================================

#universal ProbeDblClick(props) {
    state n = 0
    return <button data-testid="pdc" onDoubleClick={() => n += 1}>{n}</button>
}

#universal_test("probe: onDoubleClick handler fires") {
    <ProbeDblClick />
    <script>
        expect(byTestId('pdc').text()).toBe('0')
        byTestId('pdc').dblclick()
        expect(byTestId('pdc').text()).toBe('1')
    </script>
}

// ===========================================================================
// Probe 6: dangerouslySetInnerHTML rendered at SSR and then updated reactively.
// ===========================================================================

#universal ProbeDanger(props) {
    state html = "<b>one</b>"
    return <div data-testid="pdg">
        <button data-testid="pdg-btn" onClick={() => html = "<i>two</i>"}>b</button>
        <div data-testid="pdg-target" dangerouslySetInnerHTML={{ __html: html }}></div>
    </div>
}

#universal_test("probe: dangerouslySetInnerHTML updates after hydration") {
    <ProbeDanger />
    <script>
        expect(byTestId('pdg-target').el.innerHTML).toBe('<b>one</b>')
        byTestId('pdg-btn').click()
        expect(byTestId('pdg-target').el.innerHTML).toBe('<i>two</i>')
    </script>
}

// ===========================================================================
// Probe 7: nested providers that share a context name must not collide.
// ===========================================================================

#universal ProbeCtxLeaf(props) {
    const ctx = useContext("probe-nested")
    return <span data-testid={props.tid}>{ctx.value}</span>
}

#universal ProbeCtxInner(props) {
    const ctx = createContext("probe-nested", "")
    ctx.value = "inner"
    return <span data-testid="pci">
        <ProbeCtxLeaf tid="pci-in" />
    </span>
}

#universal ProbeCtxOuter(props) {
    const ctx = createContext("probe-nested", "")
    ctx.value = "outer"
    return <div data-testid="pco">
        <ProbeCtxLeaf tid="pco-out" />
        <ProbeCtxInner />
        <ProbeCtxLeaf tid="pco-out2" />
    </div>
}

#universal_test("probe: nested same-name providers keep separate values") {
    <ProbeCtxOuter />
    <script>
        expect(byTestId('pco-out').text()).toBe('outer')
        expect(byTestId('pci-in').text()).toBe('inner')
        expect(byTestId('pco-out2').text()).toBe('outer')
    </script>
}

// ===========================================================================
// Probe 8: reactive <textarea> value updates when state changes.
// ===========================================================================

#universal ProbeTextarea(props) {
    state text = "a"
    return <div data-testid="pta">
        <button data-testid="pta-btn" onClick={() => text = "b"}>b</button>
        <textarea data-testid="pta-ta" value={text}></textarea>
    </div>
}

#universal_test("probe: reactive textarea value updates") {
    <ProbeTextarea />
    <script>
        expect(byTestId('pta-ta').value()).toBe('a')
        byTestId('pta-btn').click()
        expect(byTestId('pta-ta').value()).toBe('b')
    </script>
}

// ===========================================================================
// Probe 9: keyed list with NUMERIC keys must reconcile by key (0 is a valid
// key and must not be treated as "no key").
// ===========================================================================

#universal ProbeNumKeyList(props) {
    state order = [0, 1, 2]
    return <ul data-testid="pnk">
        {order.map((i) => <li key={i} data-testid={"pnk-" + i}>{i}</li>)}
        <button data-testid="pnk-rev" onClick={() => order = [2, 1, 0]}>r</button>
    </ul>
}

#universal_test("probe: numeric keyed list reorders by key") {
    <ProbeNumKeyList />
    <script>
        const list = byTestId('pnk')
        const items = list.findAll('li')
        expect(items.count()).toBe(3)
        expect(items.nth(0)).toHaveText('0')
        byTestId('pnk-rev').click()
        const after = list.findAll('li')
        expect(after.nth(0)).toHaveText('2')
        expect(after.nth(1)).toHaveText('1')
        expect(after.nth(2)).toHaveText('0')
    </script>
}

// ===========================================================================
// Probe 10: conditional child driven by a parent-provided prop signal must
// switch branches when the parent state changes.
// ===========================================================================

#universal ProbePropCondChild(props) {
    return <div data-testid="ppc">
        {props.loading ? <span data-testid="ppc-load">L</span> : <span data-testid="ppc-done">D</span>}
    </div>
}

#universal ProbePropCondParent(props) {
    state loading = true
    return <div>
        <button data-testid="ppc-btn" onClick={() => loading = !loading}>b</button>
        <ProbePropCondChild loading={loading} />
    </div>
}

#universal_test("probe: conditional child tracks a parent prop signal") {
    <ProbePropCondParent />
    <script>
        expect(byTestId('ppc-load').exists()).toBeTruthy()
        byTestId('ppc-btn').click()
        expect(byTestId('ppc-done').exists()).toBeTruthy()
        expect(byTestId('ppc-load').exists()).toBe(false)
    </script>
}

// ===========================================================================
// Probe 11: useEffect with a parent-provided prop dependency re-runs (locks in
// the converter fix that keeps the raw signal in the deps array).
// ===========================================================================

#universal ProbeEffectPropChild(props) {
    useEffect(() => {
        window.__pepRuns = (window.__pepRuns || 0) + 1
        return () => { window.__pepCleanups = (window.__pepCleanups || 0) + 1 }
    }, [props.tick])
    return <span data-testid="pep-out">{window.$__uni_value(props.tick)}</span>
}

#universal ProbeEffectPropParent(props) {
    state tick = 0
    return <div data-testid="pep">
        <button data-testid="pep-btn" onClick={() => tick += 1}>b</button>
        <ProbeEffectPropChild tick={tick} />
    </div>
}

#universal_test("probe: effect tracks a parent-provided prop dependency") {
    <ProbeEffectPropParent />
    <script>
        await t.sleep(20)
        const runs0 = window.__pepRuns || 0
        const cleanups0 = window.__pepCleanups || 0
        expect(runs0).toBeGreaterThanOrEqual(1)
        expect(byTestId('pep-out').text()).toBe('0')
        byTestId('pep-btn').click()
        await t.sleep(20)
        expect(byTestId('pep-out').text()).toBe('1')
        expect(window.__pepRuns).toBe(runs0 + 1)
        expect(window.__pepCleanups).toBe(cleanups0 + 1)
    </script>
}

// ===========================================================================
// Probe 12: React-style functional useState updater (`setN(v => v + 1)`).
// ===========================================================================

#universal ProbeFnUpdater(props) {
    const [n, setN] = useState(0)
    return <button data-testid="pfu" onClick={() => setN((v) => v + 1)}>{n}</button>
}

#universal_test("probe: functional useState updater applies") {
    <ProbeFnUpdater />
    <script>
        expect(byTestId('pfu').text()).toBe('0')
        byTestId('pfu').click()
        expect(byTestId('pfu').text()).toBe('1')
        byTestId('pfu').click()
        expect(byTestId('pfu').text()).toBe('2')
    </script>
}

// ===========================================================================
// Probe 13: a reactive style object that removes a property must drop it.
// ===========================================================================

#universal ProbeStyleRemove(props) {
    state on = false
    return <div data-testid="psr" style={{ display: on ? "none" : null, color: "red" }}>
        <button data-testid="psr-btn" onClick={() => on = !on}>t</button>
    </div>
}

#universal_test("probe: reactive style removes a null property") {
    <ProbeStyleRemove />
    <script>
        expect(byTestId('psr').css('display')).toBe('block')
        expect(byTestId('psr').css('color')).toBe('rgb(255, 0, 0)')
        byTestId('psr-btn').click()
        expect(byTestId('psr').css('display')).toBe('none')
        byTestId('psr-btn').click()
        expect(byTestId('psr').css('display')).toBe('block')
        expect(byTestId('psr').css('color')).toBe('rgb(255, 0, 0)')
    </script>
}

// ===========================================================================
// Probe 14: reactive ARIA attribute updates.
// ===========================================================================

#universal ProbeAria(props) {
    state on = false
    return <div data-testid="par">
        <button data-testid="par-btn" onClick={() => on = !on}>t</button>
        <button data-testid="par-target" aria-expanded={on ? "true" : "false"}>x</button>
    </div>
}

#universal_test("probe: reactive aria attribute updates") {
    <ProbeAria />
    <script>
        expect(byTestId('par-target')).toHaveAttribute('aria-expanded', 'false')
        byTestId('par-btn').click()
        expect(byTestId('par-target')).toHaveAttribute('aria-expanded', 'true')
    </script>
}

// ===========================================================================
// Probe 15: a reactive event handler binding must swap the listener.
// ===========================================================================

#universal ProbeHandlerSwap(props) {
    state mode = 0
    return <div data-testid="phs">
        <button data-testid="phs-toggle" onClick={() => mode = 1 - mode}>t</button>
        <button data-testid="phs-target" onClick={mode == 0 ? () => window.__phs = "a" : () => window.__phs = "b"}>x</button>
    </div>
}

#universal_test("probe: reactive event handler binding swaps the listener") {
    <ProbeHandlerSwap />
    <script>
        byTestId('phs-target').click()
        expect(window.__phs).toBe('a')
        byTestId('phs-toggle').click()
        byTestId('phs-target').click()
        expect(window.__phs).toBe('b')
    </script>
}

// ===========================================================================
// Probe 16: a ref callback must be invoked with null when its element is
// removed (React contract).
// ===========================================================================

#universal ProbeRefNull(props) {
    state on = true
    return <div data-testid="prn">
        <button data-testid="prn-btn" onClick={() => on = !on}>t</button>
        {on ? <span data-testid="prn-el" ref={(el) => { window.__prnRef = el ? "el" : "null" }}>x</span> : null}
    </div>
}

#universal_test("probe: ref callback receives null when the element unmounts") {
    <ProbeRefNull />
    <script>
        expect(window.__prnRef).toBe('el')
        byTestId('prn-btn').click()
        expect(byTestId('prn-el').exists()).toBe(false)
        expect(window.__prnRef).toBe('null')
    </script>
}

// ===========================================================================
// Probe 17: a controlled text input must preserve the caret while typing.
// Reactive value bindings assign `el.value = state`, which resets the caret to
// the end on every keystroke if the assignment is unconditional.
// ===========================================================================

#universal ProbeCaret(props) {
    state text = "hello"
    return <input data-testid="pcar" value={text} onInput={(e) => text = e.target.value} />
}

#universal_test("probe: controlled input preserves the caret while typing") {
    <ProbeCaret />
    <script>
        const el = byTestId('pcar').el
        expect(el.value).toBe('hello')
        el.focus()
        el.value = 'heXllo'
        el.setSelectionRange(2, 2)
        el.dispatchEvent(new Event('input', { bubbles: true }))
        expect(el.value).toBe('heXllo')
        expect(el.selectionStart).toBe(2)
    </script>
}

// ===========================================================================
// Probe 18: reactive <select> value updates when state changes externally.
// ===========================================================================

#universal ProbeSelectValue(props) {
    state v = "b"
    return <div data-testid="psv2">
        <button data-testid="psv2-btn" onClick={() => v = "a"}>t</button>
        <select data-testid="psv2-sel" value={v}>
            <option value="a">A</option>
            <option value="b">B</option>
        </select>
    </div>
}

#universal_test("probe: reactive select value updates from state") {
    <ProbeSelectValue />
    <script>
        expect(byTestId('psv2-sel').value()).toBe('b')
        byTestId('psv2-btn').click()
        expect(byTestId('psv2-sel').value()).toBe('a')
    </script>
}

// ===========================================================================
// Probe 19: a keyed list item whose key is unchanged but whose position moves
// keeps its DOM node identity (focus/marker survive a reorder).
// ===========================================================================

#universal ProbeKeyIdentity(props) {
    state order = ["a", "b", "c"]
    return <ul data-testid="pki">
        {order.map((x) => <li key={x} data-testid={"pki-" + x}>{x}</li>)}
        <button data-testid="pki-rev" onClick={() => order = ["c", "b", "a"]}>r</button>
        <button data-testid="pki-normal" onClick={() => order = ["a", "b", "c"]}>n</button>
    </ul>
}

#universal_test("probe: keyed item identity survives a reorder") {
    <ProbeKeyIdentity />
    <script>
        const a = byTestId('pki-a')
        a.el.__probe = 'kept'
        byTestId('pki-rev').click()
        expect(byTestId('pki-a').jsProp('__probe')).toBe('kept')
        byTestId('pki-normal').click()
        expect(byTestId('pki-a').jsProp('__probe')).toBe('kept')
    </script>
}
