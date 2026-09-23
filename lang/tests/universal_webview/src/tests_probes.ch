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

// ===========================================================================
// Probe 20: merging a static class via `{...props}` with a reactive class must
// stay reactive. `$_um` concatenated with $__uni_value(), freezing the binding
// to its initial value, and a later falsy class wiped the earlier one.
// ===========================================================================

#universal ProbeClassMergeInner(props) {
    return <div data-testid="pcmi" {...props} class={props.on ? "on" : "off"}></div>
}

#universal ProbeClassMerge(props) {
    state on = false
    return <div data-testid="pcm">
        <button data-testid="pcm-btn" onClick={() => on = !on}>t</button>
        <ProbeClassMergeInner class="base" on={on} />
    </div>
}

#universal_test("probe: spread class merge keeps a reactive class live") {
    <ProbeClassMerge />
    <script>
        expect(byTestId('pcmi').hasClass('base')).toBe(true)
        expect(byTestId('pcmi').hasClass('off')).toBe(true)
        byTestId('pcm-btn').click()
        expect(byTestId('pcmi').hasClass('on')).toBe(true)
        expect(byTestId('pcmi').hasClass('off')).toBe(false)
        expect(byTestId('pcmi').hasClass('base')).toBe(true)
    </script>
}

// ===========================================================================
// Probe 21: a throwing event handler is contained and does not stop other
// handlers or a later interaction.
// ===========================================================================

#universal ProbeThrowHandler(props) {
    state ok = 0
    return <div data-testid="pth">
        <button data-testid="pth-throw" onClick={() => { throw new Error("boom") }}>t</button>
        <button data-testid="pth-ok" onClick={() => ok += 1}>{ok}</button>
    </div>
}

#universal_test("probe: throwing event handler is contained") {
    <ProbeThrowHandler />
    <script>
        byTestId('pth-throw').click()
        byTestId('pth-ok').click()
        expect(byTestId('pth-ok').text()).toBe('1')
        byTestId('pth-throw').click()
        byTestId('pth-ok').click()
        expect(byTestId('pth-ok').text()).toBe('2')
    </script>
}

// ===========================================================================
// Probe 22: a throwing effect body is contained (logged) and does not abort
// the flush or later state updates.
// ===========================================================================

#universal ProbeThrowEffect(props) {
    state n = 0
    useEffect(() => { throw new Error("effect boom") }, [])
    return <button data-testid="pte" onClick={() => n += 1}>{n}</button>
}

#universal_test("probe: throwing effect body is contained") {
    <ProbeThrowEffect />
    <script>
        await t.sleep(30)
        byTestId('pte').click()
        expect(byTestId('pte').text()).toBe('1')
    </script>
}

// ===========================================================================
// Probe 23: htmlFor associates a label with its control on both sides.
// ===========================================================================

#universal ProbeHtmlFor(props) {
    return <div data-testid="phf">
        <label data-testid="phf-label" htmlFor="phf-input">Name</label>
        <input data-testid="phf-input" id="phf-input" type="text" />
    </div>
}

#universal_test("probe: htmlFor renders and associates the label") {
    <ProbeHtmlFor />
    <script>
        expect(byTestId('phf-label')).toHaveAttribute('for', 'phf-input')
        const labelEl = byTestId('phf-label').el
        expect(labelEl.control ? labelEl.control.id : null).toBe('phf-input')
    </script>
}

// ===========================================================================
// Probe 24: two independent keyed lists in one component each keep their own
// reconciliation ranges.
// ===========================================================================

#universal ProbeTwoLists(props) {
    state orderA = ["a1", "a2"]
    state orderB = ["b1", "b2"]
    return <div data-testid="ptl">
        <ul data-testid="ptl-a">
            {orderA.map((x) => <li key={x} data-testid={"ptl-" + x}>{x}</li>)}
        </ul>
        <ul data-testid="ptl-b">
            {orderB.map((x) => <li key={x} data-testid={"ptl-" + x}>{x}</li>)}
        </ul>
        <button data-testid="ptl-rev-a" onClick={() => orderA = ["a2", "a1"]}>ra</button>
        <button data-testid="ptl-rev-b" onClick={() => orderB = ["b2", "b1"]}>rb</button>
    </div>
}

#universal_test("probe: two keyed lists reconcile independently") {
    <ProbeTwoLists />
    <script>
        byTestId('ptl-a').find('li').el.__probeA = 'kept-a'
        byTestId('ptl-b').find('li').el.__probeB = 'kept-b'
        byTestId('ptl-rev-a').click()
        expect(byTestId('ptl-a').findAll('li').nth(0).text()).toBe('a2')
        expect(byTestId('ptl-b').findAll('li').nth(0).text()).toBe('b1')
        expect(byTestId('ptl-a1').jsProp('__probeA')).toBe('kept-a')
        expect(byTestId('ptl-b1').jsProp('__probeB')).toBe('kept-b')
    </script>
}

// ===========================================================================
// Probe 25: a portal container must be removed from <body> when the component
// that owns it unmounts. Portals append a container to document.body, which is
// outside the removed DOM subtree, so nothing else cleans it up.
// ===========================================================================

#universal ProbePortalHost(props) {
    return <div data-testid="p25-host">
        {createPortal(<span data-testid="p25-portal">portal</span>, {})}
    </div>
}

#universal ProbePortalGrand(props) {
    state on = true
    return <div data-testid="p25-grand">
        <button data-testid="p25-btn" onClick={() => on = !on}>t</button>
        {on ? <ProbePortalHost /> : null}
        <span data-testid="p25-tail">tail</span>
    </div>
}

#universal_test("probe: portal container is removed when its owner unmounts", isolate) {
    <ProbePortalGrand />
    <script>
        const count = () => document.querySelectorAll('[data-testid=p25-portal]').length
        expect(count()).toBe(1)
        expect(byTestId('p25-tail').text()).toBe('tail')
        byTestId('p25-btn').click()
        expect(byTestId('p25-tail').text()).toBe('tail')
        expect(count()).toBe(0)
    </script>
}

// ===========================================================================
// Probe 26: an effect with an empty deps array runs exactly once.
// ===========================================================================

#universal ProbeEffectOnce(props) {
    state n = 0
    useEffect(() => { window.__peoRuns = (window.__peoRuns || 0) + 1 }, [])
    return <button data-testid="peo" onClick={() => n += 1}>{n}</button>
}

#universal_test("probe: effect with empty deps runs once") {
    <ProbeEffectOnce />
    <script>
        await t.sleep(20)
        const runs0 = window.__peoRuns || 0
        expect(runs0).toBe(1)
        byTestId('peo').click()
        byTestId('peo').click()
        await t.sleep(20)
        expect(byTestId('peo').text()).toBe('2')
        expect(window.__peoRuns).toBe(1)
    </script>
}

// ===========================================================================
// Probe 27: a reactive style STRING updates (cssText path).
// ===========================================================================

#universal ProbeStyleString(props) {
    state on = false
    return <div data-testid="pss" style={on ? "color:blue" : "color:red"}>
        <button data-testid="pss-btn" onClick={() => on = !on}>t</button>
    </div>
}

#universal_test("probe: reactive style string updates") {
    <ProbeStyleString />
    <script>
        expect(byTestId('pss').css('color')).toBe('rgb(255, 0, 0)')
        byTestId('pss-btn').click()
        expect(byTestId('pss').css('color')).toBe('rgb(0, 0, 255)')
    </script>
}

// ===========================================================================
// Probe 28: a universal component inside table structure hydrates via its
// comment/table boundary.
// ===========================================================================

#universal ProbeTableRow(props) {
    state n = 0
    return <tr data-testid="ptr-row"><td>
        <button data-testid="ptr-btn" onClick={() => n += 1}>{n}</button>
    </td></tr>
}

#universal ProbeTable(props) {
    return <table data-testid="ptr-table"><tbody>
        <ProbeTableRow />
    </tbody></table>
}

#universal_test("probe: component inside table structure works") {
    <ProbeTable />
    <script>
        const btn = byTestId('ptr-btn')
        expect(btn.exists()).toBeTruthy()
        expect(btn.el.closest('tr') ? true : false).toBeTruthy()
        expect(btn.text()).toBe('0')
        btn.click()
        expect(btn.text()).toBe('1')
    </script>
}

// ===========================================================================
// Probe 29: when a modal portal is removed by unmounting its owner, the
// background must be un-inerted. The inert scan otherwise only re-runs on a
// style mutation of the (now removed) modal container.
// ===========================================================================

#universal ProbeModalContent(props) {
    return createPortal(
        <div data-testid="pmc-content" style="display:block">modal</div>,
        { modal: true }
    )
}

#universal ProbeModalHost(props) {
    state open = true
    return <div data-testid="pmh">
        <button data-testid="pmh-btn" onClick={() => open = !open}>t</button>
        {open ? <ProbeModalContent /> : null}
    </div>
}

#universal_test("probe: removing a modal portal un-inerts the background", isolate) {
    <ProbeModalHost />
    <script>
        const host = byTestId('pmh').el
        expect(host.closest('[inert]') ? true : false).toBeTruthy()
        byTestId('pmh-btn').click()
        expect(document.querySelectorAll('[data-testid=pmc-content]').length).toBe(0)
        expect(host.closest('[inert]') ? true : false).toBe(false)
    </script>
}

// ===========================================================================
// Probe 30: a component with a JSX root whose conditional CHILD toggles
// null <-> element, driven by a parent prop signal.
// ===========================================================================

#universal ProbeNullContent(props) {
    return <div data-testid="pnc-root">{props.on ? <span data-testid="pnc-in">in</span> : null}</div>
}

#universal ProbeNullContentHost(props) {
    state show = false
    return <div data-testid="pnc-host">
        <button data-testid="pnc-btn" onClick={() => show = !show}>t</button>
        <ProbeNullContent on={show} />
        <span data-testid="pnc-tail">tail</span>
    </div>
}

#universal_test("probe: conditional child of a component toggles null and element") {
    <ProbeNullContentHost />
    <script>
        expect(byTestId('pnc-in').exists()).toBe(false)
        expect(byTestId('pnc-root').exists()).toBeTruthy()
        byTestId('pnc-btn').click()
        expect(byTestId('pnc-in').text()).toBe('in')
        expect(byTestId('pnc-tail').text()).toBe('tail')
        byTestId('pnc-btn').click()
        expect(byTestId('pnc-in').exists()).toBe(false)
        expect(byTestId('pnc-root').exists()).toBeTruthy()
        expect(byTestId('pnc-tail').text()).toBe('tail')
    </script>
}

// ===========================================================================
// Probe 31: `cond && <jsx/>` child toggling.
// ===========================================================================

#universal ProbeAndChild(props) {
    state on = false
    return <div data-testid="pac">
        <button data-testid="pac-btn" onClick={() => on = !on}>t</button>
        {on && <span data-testid="pac-child">c</span>}
        <span data-testid="pac-tail">tail</span>
    </div>
}

#universal_test("probe: and-shortcircuit child toggles") {
    <ProbeAndChild />
    <script>
        expect(byTestId('pac-child').exists()).toBe(false)
        expect(byTestId('pac-tail').text()).toBe('tail')
        byTestId('pac-btn').click()
        expect(byTestId('pac-child').text()).toBe('c')
        expect(byTestId('pac-tail').text()).toBe('tail')
        byTestId('pac-btn').click()
        expect(byTestId('pac-child').exists()).toBe(false)
        expect(byTestId('pac-tail').text()).toBe('tail')
    </script>
}

// ===========================================================================
// Probe 32: a reactive style object whose value is a state signal updates.
// ===========================================================================

#universal ProbeStyleSignal(props) {
    state color = "red"
    return <div data-testid="pss2" style={{ color: color }}>
        <button data-testid="pss2-btn" onClick={() => color = "blue"}>t</button>
    </div>
}

#universal_test("probe: signal-valued reactive style updates") {
    <ProbeStyleSignal />
    <script>
        expect(byTestId('pss2').css('color')).toBe('rgb(255, 0, 0)')
        byTestId('pss2-btn').click()
        expect(byTestId('pss2').css('color')).toBe('rgb(0, 0, 255)')
    </script>
}

// ===========================================================================
// Probe 33: a component with a fragment root containing text and elements.
// ===========================================================================

#universal ProbeFragText(props) {
    return <><span data-testid="pft-a">A</span> mid <span data-testid="pft-b">B</span></>
}

#universal ProbeFragTextHost(props) {
    return <div data-testid="pft"><ProbeFragText /><span data-testid="pft-tail"> tail</span></div>
}

#universal_test("probe: fragment root with mixed text hydrates") {
    <ProbeFragTextHost />
    <script>
        expect(byTestId('pft-a').text()).toBe('A')
        expect(byTestId('pft-b').text()).toBe('B')
        expect(byTestId('pft').text()).toContain('mid')
        expect(byTestId('pft-tail').text()).toBe(' tail')
    </script>
}

// ===========================================================================
// Probe 34: React's `onChange` on a text input fires while typing (the DOM
// `input` event), not only on blur/commit. Input/TextArea forward `onChange`,
// so without this mapping the standard controlled-input pattern is dead.
// ===========================================================================

#universal ProbeOnChangeInput(props) {
    state text = ""
    return <div data-testid="poci">
        <input data-testid="poci-in" type="text" value={text} onChange={(e) => text = e.target.value} />
        <span data-testid="poci-m">{text}</span>
    </div>
}

#universal_test("probe: onChange on a text input fires while typing") {
    <ProbeOnChangeInput />
    <script>
        const el = byTestId('poci-in').el
        el.value = 'abc'
        el.dispatchEvent(new Event('input', { bubbles: true }))
        expect(byTestId('poci-m').text()).toBe('abc')
    </script>
}

#universal ProbeOnChangeTextarea(props) {
    state text = ""
    return <div data-testid="poct">
        <textarea data-testid="poct-ta" value={text} onChange={(e) => text = e.target.value}></textarea>
        <span data-testid="poct-m">{text}</span>
    </div>
}

#universal_test("probe: onChange on a textarea fires while typing") {
    <ProbeOnChangeTextarea />
    <script>
        const el = byTestId('poct-ta').el
        el.value = 'xy'
        el.dispatchEvent(new Event('input', { bubbles: true }))
        expect(byTestId('poct-m').text()).toBe('xy')
    </script>
}

// Control: onChange on a checkbox still fires on click (change event).
#universal ProbeOnChangeCheckbox(props) {
    state c = false
    return <input data-testid="pocc" type="checkbox" checked={c} onChange={() => c = !c} />
}

#universal_test("probe: onChange on a checkbox still fires on click") {
    <ProbeOnChangeCheckbox />
    <script>
        expect(byTestId('pocc').isChecked()).toBe(false)
        byTestId('pocc').click()
        expect(byTestId('pocc').isChecked()).toBe(true)
        byTestId('pocc').click()
        expect(byTestId('pocc').isChecked()).toBe(false)
    </script>
}

// ===========================================================================
// Probe 35: the shipped Input component (which forwards onChange) drives a
// controlled value while typing.
// ===========================================================================

#universal ProbeInputComponent(props) {
    state v = ""
    return <div data-testid="pic">
        <Input data-testid="pic-in" value={v} onChange={(e) => v = e.target.value} />
        <span data-testid="pic-m">{v}</span>
    </div>
}

#universal_test("probe: Input component onChange drives a controlled value") {
    <ProbeInputComponent />
    <script>
        byTestId('pic-in').type('hi')
        expect(byTestId('pic-m').text()).toBe('hi')
        expect(byTestId('pic-in').value()).toBe('hi')
    </script>
}

// ===========================================================================
// Probe 36: props.children renders (single element and multiple children).
// ===========================================================================

#universal ProbeChildSlot(props) {
    return <div data-testid="pcs2">{props.children}</div>
}

#universal ProbeChildSlotHost(props) {
    return <div data-testid="pcs2h">
        <ProbeChildSlot><b data-testid="pcs2-b">bold</b></ProbeChildSlot>
        <ProbeChildSlot><i data-testid="pcs2-i1">1</i><i data-testid="pcs2-i2">2</i></ProbeChildSlot>
    </div>
}

#universal_test("probe: props.children renders single and multiple children") {
    <ProbeChildSlotHost />
    <script>
        expect(byTestId('pcs2-b').text()).toBe('bold')
        expect(byTestId('pcs2-i1').text()).toBe('1')
        expect(byTestId('pcs2-i2').text()).toBe('2')
        expect(byTestId('pcs2h').findAll('[data-testid=pcs2]').count()).toBe(2)
    </script>
}

// ===========================================================================
// Leak probes: repeated mount/unmount cycles must not retain instances,
// portal containers, or render-context stack frames.
// ===========================================================================

#universal LeakChild(props) {
    useEffect(() => {
        window.__leakMounts = (window.__leakMounts || 0) + 1
        return () => { window.__leakCleanups = (window.__leakCleanups || 0) + 1 }
    }, [])
    return <div data-testid="leak-child">child</div>
}

#universal LeakHost(props) {
    state on = false
    return <div data-testid="leak-host">
        <button data-testid="leak-btn" onClick={() => on = !on}>t</button>
        {on ? <LeakChild /> : null}
    </div>
}

#universal_test("leak: effect cleanup runs for every mount across cycles", isolate) {
    <LeakHost />
    <script>
        window.__leakMounts = 0
        window.__leakCleanups = 0
        for(let i = 0; i < 10; i++) {
            byTestId('leak-btn').click()
            byTestId('leak-btn').click()
        }
        await t.sleep(40)
        expect(byTestId('leak-child').exists()).toBe(false)
        expect(window.__leakMounts).toBe(10)
        expect(window.__leakCleanups).toBe(10)
    </script>
}

#universal_test("leak: instance map does not grow across mount/unmount cycles", isolate) {
    <LeakHost />
    <script>
        const observed = window.$__uni_cleanup_observer ? window.$__uni_cleanup_observer.observed : null
        expect(observed ? true : false).toBeTruthy()
        const base = observed.size
        for(let i = 0; i < 25; i++) {
            byTestId('leak-btn').click()
            byTestId('leak-btn').click()
        }
        await t.sleep(40)
        expect(observed.size).toBeLessThanOrEqual(base)
    </script>
}

#universal_test("leak: render context stack returns to baseline after cycles", isolate) {
    <LeakHost />
    <script>
        const base = window.$__uni_render_stack.length
        for(let i = 0; i < 25; i++) {
            byTestId('leak-btn').click()
            byTestId('leak-btn').click()
        }
        await t.sleep(40)
        expect(window.$__uni_render_stack.length).toBe(base)
    </script>
}

#universal LeakPortalChild(props) {
    return createPortal(<span data-testid="leak-portal-mark">p</span>, {})
}

#universal LeakPortalHost(props) {
    state on = false
    return <div data-testid="leak-portal-host">
        <button data-testid="leak-portal-btn" onClick={() => on = !on}>t</button>
        {on ? <LeakPortalChild /> : null}
    </div>
}

#universal_test("leak: repeated portal mount/unmount leaves no containers", isolate) {
    <LeakPortalHost />
    <script>
        for(let i = 0; i < 10; i++) {
            byTestId('leak-portal-btn').click()
            byTestId('leak-portal-btn').click()
        }
        await t.sleep(40)
        expect(document.querySelectorAll('[data-testid=leak-portal-mark]').length).toBe(0)
        expect(document.querySelectorAll('body > [data-uni-portal]').length).toBe(0)
    </script>
}

// A computed whose body throws must still unwind the render-context stack
// ($_ucs.recompute pushed a frame with no try/finally, so the frame leaked and
// every later context restore was off by one).
#universal LeakCompute(props) {
    state arr = ["a"]
    return <div data-testid="leak-compute">
        <button data-testid="leak-compute-empty" onClick={() => arr = []}>e</button>
        <span data-testid="leak-compute-out">{arr[0].toUpperCase()}</span>
    </div>
}

#universal LeakPropChild(props) {
    return <input data-testid="leak-prop-in" type="checkbox" checked={props.checked} />
}

#universal LeakPropHost(props) {
    state c = false
    state on = false
    return <div data-testid="leak-prop-host">
        <button data-testid="leak-prop-toggle" onClick={() => on = !on}>t</button>
        <button data-testid="leak-prop-set" onClick={() => c = !c}>s</button>
        {on ? <LeakPropChild checked={c} /> : null}
    </div>
}

#universal_test("leak: element prop subscriptions are released on unmount", isolate) {
    <LeakPropHost />
    <script>
        const orig = window.$__uni_set_prop
        window.__spCount = 0
        window.$__uni_set_prop = (el, key, value) => { window.__spCount = window.__spCount + 1; return orig(el, key, value) }
        for(let i = 0; i < 10; i++) {
            byTestId('leak-prop-toggle').click()
            byTestId('leak-prop-toggle').click()
        }
        await t.sleep(30)
        expect(byTestId('leak-prop-in').exists()).toBe(false)
        // With the child unmounted, updating the parent signal that used to feed
        // it must not touch any (detached) element.
        window.__spCount = 0
        byTestId('leak-prop-set').click()
        await t.sleep(30)
        expect(window.__spCount).toBe(0)
    </script>
}

#universal_test("leak: a throwing computed leaves the render context stack clean", isolate) {
    <LeakCompute />
    <script>
        expect(byTestId('leak-compute-out').text()).toBe('A')
        expect(window.$__uni_render_stack.length).toBe(0)
        byTestId('leak-compute-empty').click()
        await t.sleep(20)
        expect(window.$__uni_render_stack.length).toBe(0)
        // A later update must still render normally.
        byTestId('leak-compute-empty').click()
        await t.sleep(20)
        expect(window.$__uni_render_stack.length).toBe(0)
    </script>
}
