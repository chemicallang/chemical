// Regression probes for the universal hydration/runtime bug classes:
//
//  * Hydration positional alignment when a reactive child renders nothing on
//    the server (null/false/true/"" / empty array). The following sibling's SSR
//    node must not be consumed by the empty reactive slot.
//  * Instance ownership when several components share one DOM host element
//    (a component whose root is another component).
//  * Adjacent reconciliation / effect / context / prop-binding behaviour.
//
// These are exploratory: they intentionally probe edge cases that may still be
// broken. Do not "fix" by weakening the assertions.

// ===========================================================================
// Class 1: empty / void reactive values during hydration
// ===========================================================================

// Empty-string state child. SSR renders nothing for ""; the following Field
// must stay inside its own <label>.
#universal RegEmptyStrThenField(props) {
    state msg = ""
    return <form>
        {msg}
        <Field label="Email">
            <Input type="email" name="reg-es-email" />
        </Field>
    </form>
}

#universal_test("regression: empty-string state child does not consume the next node") {
    <RegEmptyStrThenField />
    <script>
        const input = byCss('input[name="reg-es-email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
        expect(input.el.closest('button')).toBe(null)
    </script>
}

// Empty-array state child (not .map()): SSR renders nothing.
#universal RegEmptyArrChildThenTail(props) {
    state arr = []
    return <div data-testid="reg-eac">
        {arr}
        <span data-testid="reg-eac-tail">tail</span>
    </div>
}

#universal_test("regression: empty-array state child does not consume the next node") {
    <RegEmptyArrChildThenTail />
    <script>
        expect(byTestId('reg-eac-tail').exists()).toBeTruthy()
        expect(byTestId('reg-eac').getByTestIdAll('reg-eac-tail').count()).toBe(1)
    </script>
}

// Empty-array state `.map()` child followed by a Field.
#universal RegEmptyMapThenField(props) {
    state items = []
    return <form>
        {items.map((it) => <li>{it}</li>)}
        <Field label="Email">
            <Input type="email" name="reg-em-email" />
        </Field>
    </form>
}

#universal_test("regression: empty .map() list does not consume the next node") {
    <RegEmptyMapThenField />
    <script>
        const input = byCss('input[name="reg-em-email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
        expect(input.el.closest('button')).toBe(null)
    </script>
}

// Three empty conditionals in a row before a Field + Button.
#universal RegThreeNullThenField(props) {
    state a = ""
    state b = ""
    state c = ""
    return <form>
        {a ? <Alert variant="error" description={a} /> : null}
        {b ? <Alert variant="success" description={b} /> : null}
        {c ? <Alert variant="info" description={c} /> : null}
        <Field label="Email">
            <Input type="email" name="reg-tn-email" />
        </Field>
        <Button type="submit">Go</Button>
    </form>
}

#universal_test("regression: three empty conditionals before a Field keep it a label") {
    <RegThreeNullThenField />
    <script>
        const input = byCss('input[name="reg-tn-email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
        expect(input.el.closest('button')).toBe(null)
    </script>
}

// Empty conditional directly followed by a nested universal component.
#universal RegNullThenComponent(props) {
    state err = ""
    return <div data-testid="reg-ntc">
        {err ? <Alert variant="error" description={err} /> : null}
        <InputGroup data-testid="reg-ntc-group">
            <Input data-testid="reg-ntc-input" name="reg-ntc" />
        </InputGroup>
    </div>
}

#universal_test("regression: empty conditional before a nested component keeps the group") {
    <RegNullThenComponent />
    <script>
        expect(byTestId('reg-ntc-group').exists()).toBeTruthy()
        expect(byTestId('reg-ntc-input').el.parentElement.getAttribute('data-testid')).toBe('reg-ntc-group')
    </script>
}

// Empty conditional followed by a text sibling.
#universal RegNullThenText(props) {
    state on = false
    return <div data-testid="reg-ntt">
        <button data-testid="reg-ntt-toggle" onClick={() => on = !on}>t</button>
        {on ? <em data-testid="reg-ntt-em">x</em> : null}
        <span data-testid="reg-ntt-tail">tail</span>
    </div>
}

#universal_test("regression: empty conditional before a text sibling") {
    <RegNullThenText />
    <script>
        expect(byTestId('reg-ntt-tail').exists()).toBeTruthy()
        expect(byTestId('reg-ntt-em').exists()).toBe(false)
        byTestId('reg-ntt-toggle').click()
        expect(byTestId('reg-ntt-em').text()).toBe('x')
        expect(byTestId('reg-ntt-tail').exists()).toBeTruthy()
        byTestId('reg-ntt-toggle').click()
        expect(byTestId('reg-ntt-em').exists()).toBe(false)
        expect(byTestId('reg-ntt-tail').exists()).toBeTruthy()
    </script>
}

// Empty conditional at the very end of a parent (no following sibling).
#universal RegNullAtEnd(props) {
    state on = false
    return <div data-testid="reg-nae">
        <button data-testid="reg-nae-toggle" onClick={() => on = !on}>t</button>
        <span data-testid="reg-nae-head">head</span>
        {on ? <em data-testid="reg-nae-em">x</em> : null}
    </div>
}

#universal_test("regression: empty conditional at the end of a parent") {
    <RegNullAtEnd />
    <script>
        expect(byTestId('reg-nae-head').text()).toBe('head')
        byTestId('reg-nae-toggle').click()
        expect(byTestId('reg-nae-em').text()).toBe('x')
        byTestId('reg-nae-toggle').click()
        expect(byTestId('reg-nae-em').exists()).toBe(false)
        expect(byTestId('reg-nae-head').text()).toBe('head')
    </script>
}

// false boolean state child (control: boolean renders nothing both sides).
#universal RegFalseChild(props) {
    state flag = false
    return <div data-testid="reg-false">
        {flag}
        <span data-testid="reg-false-tail">tail</span>
    </div>
}

#universal_test("regression: false boolean state child keeps the next node") {
    <RegFalseChild />
    <script>
        expect(byTestId('reg-false-tail').text()).toBe('tail')
    </script>
}

// zero number state child (control: SSR renders "0").
#universal RegZeroThenField(props) {
    state n = 0
    return <form>
        {n}
        <Field label="Email">
            <Input type="email" name="reg-z-email" />
        </Field>
    </form>
}

#universal_test("regression: zero number child keeps the next node") {
    <RegZeroThenField />
    <script>
        const input = byCss('input[name="reg-z-email"]')
        expect(input.exists()).toBeTruthy()
        expect(input.el.parentElement.tagName.toLowerCase()).toBe('label')
    </script>
}

// Conditional whose value is a FRAGMENT, truthy at SSR then toggled off.
#universal RegFragCond(props) {
    state on = true
    return <div data-testid="reg-fragcond">
        <button data-testid="reg-fragcond-toggle" onClick={() => on = !on}>t</button>
        {on ? <><span data-testid="reg-fragcond-a">a</span><span data-testid="reg-fragcond-b">b</span></> : null}
        <span data-testid="reg-fragcond-tail">tail</span>
    </div>
}

#universal_test("regression: fragment conditional toggled off removes all fragment nodes", isolate) {
    <RegFragCond />
    <script>
        expect(byTestId('reg-fragcond-a').text()).toBe('a')
        expect(byTestId('reg-fragcond-b').text()).toBe('b')
        byTestId('reg-fragcond-toggle').click()
        expect(byTestId('reg-fragcond-a').exists()).toBe(false)
        expect(byTestId('reg-fragcond-b').exists()).toBe(false)
        expect(byTestId('reg-fragcond-tail').text()).toBe('tail')
    </script>
}

// Conditional whose value is a universal COMPONENT, truthy at SSR.
#universal RegCondCompChild(props) {
    state n = 0
    return <button data-testid="reg-cc-child" onClick={() => n += 1}>{n}</button>
}

#universal RegCondComp(props) {
    state on = true
    return <div data-testid="reg-cc">
        <button data-testid="reg-cc-toggle" onClick={() => on = !on}>t</button>
        {on ? <RegCondCompChild /> : null}
        <span data-testid="reg-cc-tail">tail</span>
    </div>
}

#universal_test("regression: component conditional toggled off then on remounts") {
    <RegCondComp />
    <script>
        expect(byTestId('reg-cc-child').text()).toBe('0')
        byTestId('reg-cc-child').click()
        expect(byTestId('reg-cc-child').text()).toBe('1')
        byTestId('reg-cc-toggle').click()
        expect(byTestId('reg-cc-child').exists()).toBe(false)
        expect(byTestId('reg-cc-tail').text()).toBe('tail')
        byTestId('reg-cc-toggle').click()
        expect(byTestId('reg-cc-child').text()).toBe('0')
    </script>
}

// State list emptied then restored.
#universal RegListToggle(props) {
    state items = ["a", "b", "c"]
    return <ul data-testid="reg-list">
        {items.map((x) => <li data-testid={"reg-list-" + x}>{x}</li>)}
        <button data-testid="reg-list-empty" onClick={() => items = []}>empty</button>
        <button data-testid="reg-list-restore" onClick={() => items = ["a", "b", "c"]}>restore</button>
    </ul>
}

#universal_test("regression: list emptied then restored renders all items") {
    <RegListToggle />
    <script>
        expect(byTestId('reg-list').findAll('li').count()).toBe(3)
        byTestId('reg-list-empty').click()
        expect(byTestId('reg-list').findAll('li').count()).toBe(0)
        byTestId('reg-list-restore').click()
        expect(byTestId('reg-list').findAll('li').count()).toBe(3)
        expect(byTestId('reg-list-a').text()).toBe('a')
    </script>
}

// Conditional text value, truthy at SSR then toggled off.
#universal RegTextCond(props) {
    state on = true
    return <div data-testid="reg-tc">
        <button data-testid="reg-tc-toggle" onClick={() => on = !on}>t</button>
        {on ? "visible" : null}
        <span data-testid="reg-tc-tail">tail</span>
    </div>
}

#universal_test("regression: conditional text toggled off leaves the tail intact") {
    <RegTextCond />
    <script>
        expect(byTestId('reg-tc').text()).toContain('visible')
        byTestId('reg-tc-toggle').click()
        expect(byTestId('reg-tc').text()).not.toContain('visible')
        expect(byTestId('reg-tc-tail').text()).toBe('tail')
    </script>
}

// ===========================================================================
// Class 2: instance ownership when components share a DOM host
// ===========================================================================

// Two levels of "root is another component": RegLvl2 -> RegLvl3 -> InputGroup.
#universal RegLvl3(props) {
    state n = 0
    return <InputGroup data-testid="reg-l3">
        <button data-testid="reg-l3-btn" onClick={() => n += 1}>{n}</button>
    </InputGroup>
}

#universal RegLvl2(props) {
    return <RegLvl3 />
}

#universal RegLvl1(props) {
    state n = 0
    return <div data-testid="reg-l1">
        <RegLvl2 />
        <button data-testid="reg-l1-btn" onClick={() => n += 1}>{n}</button>
    </div>
}

#universal_test("regression: two-level root-nested components keep their own state") {
    <RegLvl1 />
    <script>
        expect(byTestId('reg-l3-btn').text()).toBe('0')
        byTestId('reg-l1-btn').click()
        expect(byTestId('reg-l1-btn').text()).toBe('1')
        expect(byTestId('reg-l3-btn').text()).toBe('0')
        byTestId('reg-l3-btn').click()
        expect(byTestId('reg-l3-btn').text()).toBe('1')
        expect(byTestId('reg-l1-btn').text()).toBe('1')
    </script>
}

// Root-nested component remounted by a parent toggle.
#universal RegRemountInner(props) {
    state n = 0
    return <InputGroup data-testid="reg-rm-inner">
        <button data-testid="reg-rm-btn" onClick={() => n += 1}>{n}</button>
    </InputGroup>
}

#universal RegRemountHost(props) {
    state show = true
    return <div data-testid="reg-rm-host">
        <button data-testid="reg-rm-toggle" onClick={() => show = !show}>t</button>
        {show ? <RegRemountInner /> : null}
    </div>
}

#universal_test("regression: root-nested component remounts with fresh state") {
    <RegRemountHost />
    <script>
        byTestId('reg-rm-btn').click()
        expect(byTestId('reg-rm-btn').text()).toBe('1')
        byTestId('reg-rm-toggle').click()
        expect(byTestId('reg-rm-btn').exists()).toBe(false)
        byTestId('reg-rm-toggle').click()
        expect(byTestId('reg-rm-btn').text()).toBe('0')
        byTestId('reg-rm-btn').click()
        expect(byTestId('reg-rm-btn').text()).toBe('1')
    </script>
}

// Root-nested component with an effect: cleanup must run on removal.
#universal RegEffectInner(props) {
    useEffect(() => {
        window.__regEffectMounted = true
        return () => { window.__regEffectCleanup = true }
    }, [])
    return <InputGroup data-testid="reg-ei"><span data-testid="reg-ei-span">ei</span></InputGroup>
}

#universal RegEffectHost(props) {
    state show = true
    return <div data-testid="reg-ei-host">
        <button data-testid="reg-ei-toggle" onClick={() => show = !show}>t</button>
        {show ? <RegEffectInner /> : null}
    </div>
}

#universal_test("regression: root-nested component effect cleanup runs on removal") {
    <RegEffectHost />
    <script>
        window.__regEffectCleanup = false
        expect(byTestId('reg-ei-span').exists()).toBeTruthy()
        expect(window.__regEffectMounted).toBe(true)
        byTestId('reg-ei-toggle').click()
        expect(byTestId('reg-ei-span').exists()).toBe(false)
        await t.sleep(60)
        expect(window.__regEffectCleanup).toBe(true)
    </script>
}

// Two independent root-nested siblings must not share state.
#universal RegSibInner(props) {
    state n = props.start || 0
    return <InputGroup data-testid={props.tid}>
        <button data-testid={props.bid} onClick={() => n += 1}>{n}</button>
    </InputGroup>
}

#universal RegSibHost(props) {
    return <div data-testid="reg-sib">
        <RegSibInner tid="reg-sib-a" bid="reg-sib-a-btn" start={0} />
        <RegSibInner tid="reg-sib-b" bid="reg-sib-b-btn" start={10} />
    </div>
}

#universal_test("regression: two root-nested siblings keep independent state") {
    <RegSibHost />
    <script>
        byTestId('reg-sib-a-btn').click()
        byTestId('reg-sib-a-btn').click()
        expect(byTestId('reg-sib-a-btn').text()).toBe('2')
        expect(byTestId('reg-sib-b-btn').text()).toBe('10')
        byTestId('reg-sib-b-btn').click()
        expect(byTestId('reg-sib-b-btn').text()).toBe('11')
        expect(byTestId('reg-sib-a-btn').text()).toBe('2')
    </script>
}

// A component whose root is another component whose root is a fragment.
#universal RegFragInner2(props) {
    state n = 0
    return <>
        <button data-testid="reg-f2-btn" onClick={() => n += 1}>{n}</button>
        <span data-testid="reg-f2-span">s</span>
    </>
}

#universal RegFragOuter2(props) {
    return <RegFragInner2 />
}

#universal RegFragHost2(props) {
    return <div data-testid="reg-f2-host">
        <RegFragOuter2 />
    </div>
}

#universal_test("regression: root-nested component with fragment root keeps state") {
    <RegFragHost2 />
    <script>
        expect(byTestId('reg-f2-span').text()).toBe('s')
        byTestId('reg-f2-btn').click()
        expect(byTestId('reg-f2-btn').text()).toBe('1')
    </script>
}

// Context published by a component whose root is another component.
#universal RegCtxInner(props) {
    const ctx = useContext("reg-ctx")
    return <span data-testid="reg-ctx-out">{ctx.value}</span>
}

#universal RegCtxOuter(props) {
    state v = "init"
    const ctx = createContext("reg-ctx", "")
    ctx.value = v
    return <InputGroup data-testid="reg-ctx-grp">
        <RegCtxInner />
        <button data-testid="reg-ctx-btn" onClick={() => v = "changed"}>c</button>
    </InputGroup>
}

#universal_test("regression: context resolves through a root-nested provider") {
    <RegCtxOuter />
    <script>
        expect(byTestId('reg-ctx-out').text()).toBe('init')
        byTestId('reg-ctx-btn').click()
        expect(byTestId('reg-ctx-out').text()).toBe('changed')
    </script>
}

// ===========================================================================
// Class 3: reconciliation
// ===========================================================================

// Keyed component list: changing a prop on the same key must not reset state.
#universal RegKeyChild(props) {
    state n = 0
    return <li data-testid={props.tid}>
        <button data-testid={props.bid} onClick={() => n += 1}>{n}</button>
        <span data-testid={props.lid}>{props.label}</span>
    </li>
}

#universal RegKeyHost(props) {
    state tick = 0
    var items = ["a", "b"]
    return <ul data-testid="reg-key">
        {items.map((x) => <RegKeyChild key={x} tid={"reg-k-" + x} bid={"reg-k-btn-" + x} lid={"reg-k-label-" + x} label={x + ":" + tick} />)}
        <button data-testid="reg-k-bump" onClick={() => tick += 1}>b</button>
    </ul>
}

#universal_test("regression: keyed component prop change preserves item state") {
    <RegKeyHost />
    <script>
        byTestId('reg-k-btn-a').click()
        byTestId('reg-k-btn-a').click()
        expect(byTestId('reg-k-btn-a').text()).toBe('2')
        byTestId('reg-k-bump').click()
        expect(byTestId('reg-k-label-a').text()).toBe('a:1')
        expect(byTestId('reg-k-btn-a').text()).toBe('2')
    </script>
}

// Unkeyed component list: a parent re-render that changes child props must not
// reset the child's state (React preserves by position).
#universal RegUnkeyChild(props) {
    state n = 0
    return <li data-testid={props.tid}>
        <button data-testid={props.bid} onClick={() => n += 1}>{n}</button>
    </li>
}

#universal RegUnkeyHost(props) {
    state tick = 0
    var items = ["a", "b"]
    return <ul data-testid="reg-unkey">
        {items.map((x) => <RegUnkeyChild tid={"reg-u-" + x} bid={"reg-u-btn-" + x} bump={tick} />)}
        <button data-testid="reg-u-bump" onClick={() => tick += 1}>b</button>
    </ul>
}

#universal_test("regression: unkeyed component list preserves state on parent re-render") {
    <RegUnkeyHost />
    <script>
        byTestId('reg-u-btn-a').click()
        byTestId('reg-u-btn-a').click()
        expect(byTestId('reg-u-btn-a').text()).toBe('2')
        byTestId('reg-u-bump').click()
        expect(byTestId('reg-u-btn-a').text()).toBe('2')
    </script>
}

// Keyed element list: prop change patches the element in place.
#universal RegKeyEl(props) {
    state tick = 0
    var items = ["a", "b"]
    return <ul data-testid="reg-ke">
        {items.map((x) => <li key={x} data-testid={"reg-ke-" + x}>{x + ":" + tick}</li>)}
        <button data-testid="reg-ke-bump" onClick={() => tick += 1}>b</button>
    </ul>
}

#universal_test("regression: keyed element prop change patches text in place") {
    <RegKeyEl />
    <script>
        expect(byTestId('reg-ke-a').text()).toBe('a:0')
        byTestId('reg-ke-bump').click()
        expect(byTestId('reg-ke-a').text()).toBe('a:1')
        expect(byTestId('reg-ke-b').text()).toBe('b:1')
    </script>
}

// ===========================================================================
// Class 4: effects / context
// ===========================================================================

#universal RegLayoutEffect(props) {
    state n = 0
    useLayoutEffect(() => {
        window.__regLayoutRuns = (window.__regLayoutRuns || 0) + 1
    }, [])
    return <button data-testid="reg-le" onClick={() => n += 1}>{n}</button>
}

#universal_test("regression: useLayoutEffect runs once on mount") {
    <RegLayoutEffect />
    <script>
        const runsAfterMount = window.__regLayoutRuns
        expect(runsAfterMount).toBeGreaterThanOrEqual(1)
        byTestId('reg-le').click()
        expect(byTestId('reg-le').text()).toBe('1')
        expect(window.__regLayoutRuns).toBe(runsAfterMount)
    </script>
}

#universal RegEffectDeps(props) {
    state n = 0
    useEffect(() => {
        window.__regEffRuns = (window.__regEffRuns || 0) + 1
        return () => { window.__regEffCleanups = (window.__regEffCleanups || 0) + 1 }
    }, [n])
    return <button data-testid="reg-ed" onClick={() => n += 1}>{n}</button>
}

#universal_test("regression: effect cleanup runs before a dependency re-run") {
    <RegEffectDeps />
    <script>
        await t.sleep(30)
        const runsBefore = window.__regEffRuns || 0
        const cleanupsBefore = window.__regEffCleanups || 0
        expect(runsBefore).toBeGreaterThanOrEqual(1)
        byTestId('reg-ed').click()
        await t.sleep(30)
        expect(window.__regEffRuns).toBe(runsBefore + 1)
        expect(window.__regEffCleanups).toBe(cleanupsBefore + 1)
    </script>
}

#universal RegProvInner(props) {
    const ctx = useContext("reg-prov")
    return <span data-testid="reg-prov-out">{ctx.value}</span>
}

#universal RegProvOuter(props) {
    state v = "one"
    const ctx = createContext("reg-prov", "")
    ctx.value = v
    return <div data-testid="reg-prov">
        <RegProvInner />
        <button data-testid="reg-prov-btn" onClick={() => v = "two"}>c</button>
    </div>
}

#universal_test("regression: context consumer updates when provider state changes") {
    <RegProvOuter />
    <script>
        expect(byTestId('reg-prov-out').text()).toBe('one')
        byTestId('reg-prov-btn').click()
        expect(byTestId('reg-prov-out').text()).toBe('two')
    </script>
}

#universal RegCtxDefault(props) {
    const ctx = useContext("reg-missing-ctx")
    return <span data-testid="reg-cd">{ctx.value || "none"}</span>
}

#universal_test("regression: useContext without a provider falls back safely") {
    <RegCtxDefault />
    <script>
        expect(byTestId('reg-cd').text()).toBe('none')
    </script>
}

// ===========================================================================
// Class 5: reactive prop bindings
// ===========================================================================

#universal RegClassBind(props) {
    state active = false
    return <div data-testid="reg-cls" class={active ? "on" : "off"}>
        <button data-testid="reg-cls-btn" onClick={() => active = !active}>t</button>
    </div>
}

#universal_test("regression: reactive class binding updates") {
    <RegClassBind />
    <script>
        expect(byTestId('reg-cls').attr('class')).toBe('off')
        byTestId('reg-cls-btn').click()
        expect(byTestId('reg-cls').attr('class')).toBe('on')
    </script>
}

#universal RegStyleBind(props) {
    state on = false
    return <div data-testid="reg-sty" style={{display: on ? "none" : "block"}}>
        <button data-testid="reg-sty-btn" onClick={() => on = !on}>t</button>
    </div>
}

#universal_test("regression: reactive style binding updates") {
    <RegStyleBind />
    <script>
        expect(byTestId('reg-sty').css('display')).toBe('block')
        byTestId('reg-sty-btn').click()
        expect(byTestId('reg-sty').css('display')).toBe('none')
    </script>
}

#universal RegCheckBind(props) {
    state checked = false
    return <input data-testid="reg-chk" type="checkbox" checked={checked} onChange={() => checked = !checked} />
}

#universal_test("regression: controlled checkbox toggles") {
    <RegCheckBind />
    <script>
        expect(byTestId('reg-chk').isChecked()).toBe(false)
        byTestId('reg-chk').click()
        expect(byTestId('reg-chk').isChecked()).toBe(true)
        byTestId('reg-chk').click()
        expect(byTestId('reg-chk').isChecked()).toBe(false)
    </script>
}

// Adjacent text + state expression hydration.
#universal RegTextMix(props) {
    state name = "x"
    return <div data-testid="reg-tm">Hello {name} world</div>
}

#universal_test("regression: adjacent text and expression hydrate correctly") {
    <RegTextMix />
    <script>
        expect(byTestId('reg-tm').text()).toBe('Hello x world')
    </script>
}

// ===========================================================================
// Class 1b: reactive slots that render MULTIPLE SSR nodes (fragments, arrays,
// adjacent text/expression slots). The server merges/renders several nodes for
// one client vnode, so the hydration cursor must span all of them.
// ===========================================================================

// Conditional whose value is a component with a fragment (multi-node) root.
#universal RegFragRootChild(props) {
    return <>
        <span data-testid="reg-frc-a">a</span>
        <span data-testid="reg-frc-b">b</span>
    </>
}

#universal RegFragRootCond(props) {
    state on = true
    return <div data-testid="reg-frc">
        <button data-testid="reg-frc-toggle" onClick={() => on = !on}>t</button>
        {on ? <RegFragRootChild /> : null}
        <span data-testid="reg-frc-tail">tail</span>
    </div>
}

#universal_test("regression: conditional multi-node component toggled off removes all nodes", isolate) {
    <RegFragRootCond />
    <script>
        expect(byTestId('reg-frc-a').text()).toBe('a')
        expect(byTestId('reg-frc-b').text()).toBe('b')
        byTestId('reg-frc-toggle').click()
        expect(byTestId('reg-frc-a').exists()).toBe(false)
        expect(byTestId('reg-frc-b').exists()).toBe(false)
        expect(byTestId('reg-frc-tail').text()).toBe('tail')
    </script>
}

// Computed array rendered as a list, truthy at SSR then emptied.
#universal RegComputedList(props) {
    state on = true
    var list = on ? ["a", "b"] : []
    return <div data-testid="reg-cl">
        <button data-testid="reg-cl-toggle" onClick={() => on = !on}>t</button>
        {list.map((x) => <span data-testid={"reg-cl-" + x}>{x}</span>)}
        <span data-testid="reg-cl-tail">tail</span>
    </div>
}

#universal_test("regression: computed list emptied then restored") {
    <RegComputedList />
    <script>
        expect(byTestId('reg-cl-a').text()).toBe('a')
        expect(byTestId('reg-cl-b').text()).toBe('b')
        byTestId('reg-cl-toggle').click()
        expect(byTestId('reg-cl-a').exists()).toBe(false)
        expect(byTestId('reg-cl-b').exists()).toBe(false)
        expect(byTestId('reg-cl-tail').text()).toBe('tail')
        byTestId('reg-cl-toggle').click()
        expect(byTestId('reg-cl-a').text()).toBe('a')
        expect(byTestId('reg-cl-b').text()).toBe('b')
        expect(byTestId('reg-cl-tail').text()).toBe('tail')
    </script>
}

// Adjacent reactive text slots: the server merges them into one text node, the
// client has one vnode per slot.
#universal RegMultiText(props) {
    state a = "A"
    state b = "B"
    return <div data-testid="reg-mt">
        <button data-testid="reg-mt-btn" onClick={() => { a = "X"; b = "Y" }}>t</button>
        {a}{b}
        <span data-testid="reg-mt-tail">tail</span>
    </div>
}

#universal_test("regression: adjacent reactive text slots hydrate and update") {
    <RegMultiText />
    <script>
        expect(byTestId('reg-mt').text()).toContain('AB')
        expect(byTestId('reg-mt-tail').text()).toBe('tail')
        byTestId('reg-mt-btn').click()
        expect(byTestId('reg-mt').text()).toContain('XY')
        expect(byTestId('reg-mt-tail').text()).toBe('tail')
    </script>
}

// A `.map()` callback that reads state must re-run when that state changes.
#universal RegMapReactive(props) {
    state tick = 0
    var items = ["a", "b"]
    return <ul data-testid="reg-mr">
        {items.map((x) => <li data-testid={"reg-mr-" + x}>{x + ":" + tick}</li>)}
        <button data-testid="reg-mr-bump" onClick={() => tick += 1}>b</button>
    </ul>
}

#universal_test("regression: map callback reading state re-renders on change") {
    <RegMapReactive />
    <script>
        expect(byTestId('reg-mr-a').text()).toBe('a:0')
        byTestId('reg-mr-bump').click()
        expect(byTestId('reg-mr-a').text()).toBe('a:1')
        expect(byTestId('reg-mr-b').text()).toBe('b:1')
    </script>
}

// Dynamically-mounted root-nested chain: removing it must dispose both the
// outer and the inner component (their effect cleanups must run).
#universal RegDynNestedChild(props) {
    useEffect(() => {
        window.__regDncMounted = true
        return () => { window.__regDncCleanup = true }
    }, [])
    return <InputGroup data-testid="reg-dnc-inner"><span data-testid="reg-dnc-span">c</span></InputGroup>
}

#universal RegDynNestedParent(props) {
    useEffect(() => {
        window.__regDnpMounted = true
        return () => { window.__regDnpCleanup = true }
    }, [])
    return <InputGroup data-testid="reg-dnp-inner">
        <RegDynNestedChild />
    </InputGroup>
}

#universal RegDynNestedGrand(props) {
    state show = true
    return <div data-testid="reg-dng">
        <button data-testid="reg-dng-toggle" onClick={() => show = !show}>t</button>
        {show ? <RegDynNestedParent /> : null}
    </div>
}

#universal_test("regression: removing a dynamically-mounted nested chain disposes all instances") {
    <RegDynNestedGrand />
    <script>
        window.__regDncCleanup = false
        window.__regDnpCleanup = false
        expect(byTestId('reg-dnc-span').exists()).toBeTruthy()
        expect(window.__regDncMounted).toBe(true)
        expect(window.__regDnpMounted).toBe(true)
        byTestId('reg-dng-toggle').click()
        expect(byTestId('reg-dnc-span').exists()).toBe(false)
        await t.sleep(60)
        expect(window.__regDnpCleanup).toBe(true)
        expect(window.__regDncCleanup).toBe(true)
    </script>
}

// Multi-node reactive slot at the END of a parent (no following sibling).
#universal RegFragCondEnd(props) {
    state on = true
    return <div data-testid="reg-fce">
        <button data-testid="reg-fce-toggle" onClick={() => on = !on}>t</button>
        <span data-testid="reg-fce-head">head</span>
        {on ? <><span data-testid="reg-fce-a">a</span><span data-testid="reg-fce-b">b</span></> : null}
    </div>
}

#universal_test("regression: multi-node slot at the end of a parent toggles cleanly", isolate) {
    <RegFragCondEnd />
    <script>
        expect(byTestId('reg-fce-a').text()).toBe('a')
        expect(byTestId('reg-fce-b').text()).toBe('b')
        byTestId('reg-fce-toggle').click()
        expect(byTestId('reg-fce-a').exists()).toBe(false)
        expect(byTestId('reg-fce-b').exists()).toBe(false)
        expect(byTestId('reg-fce-head').text()).toBe('head')
    </script>
}

// Multi-node reactive slot with three SSR nodes.
#universal RegFragCondThree(props) {
    state on = true
    return <div data-testid="reg-fc3">
        <button data-testid="reg-fc3-toggle" onClick={() => on = !on}>t</button>
        {on ? <><span data-testid="reg-fc3-a">a</span><span data-testid="reg-fc3-b">b</span><span data-testid="reg-fc3-c">c</span></> : null}
        <span data-testid="reg-fc3-tail">tail</span>
    </div>
}

#universal_test("regression: three-node fragment conditional toggles cleanly", isolate) {
    <RegFragCondThree />
    <script>
        expect(byTestId('reg-fc3-a').text()).toBe('a')
        expect(byTestId('reg-fc3-b').text()).toBe('b')
        expect(byTestId('reg-fc3-c').text()).toBe('c')
        byTestId('reg-fc3-toggle').click()
        expect(byTestId('reg-fc3-a').exists()).toBe(false)
        expect(byTestId('reg-fc3-b').exists()).toBe(false)
        expect(byTestId('reg-fc3-c').exists()).toBe(false)
        expect(byTestId('reg-fc3-tail').text()).toBe('tail')
    </script>
}

// Multi-node reactive slot inside each item of a list.
#universal RegFragInList(props) {
    state on = true
    var items = ["a", "b"]
    return <ul data-testid="reg-fil">
        {items.map((x) => <li data-testid={"reg-fil-" + x}>
            {on ? <><em data-testid={"reg-fil-em-" + x}>e</em><i data-testid={"reg-fil-i-" + x}>i</i></> : null}
            <span data-testid={"reg-fil-tail-" + x}>t</span>
        </li>)}
        <button data-testid="reg-fil-toggle" onClick={() => on = !on}>t</button>
    </ul>
}

#universal_test("regression: multi-node slot inside list items toggles cleanly", isolate) {
    <RegFragInList />
    <script>
        expect(byTestId('reg-fil-em-a').text()).toBe('e')
        expect(byTestId('reg-fil-i-a').text()).toBe('i')
        expect(byTestId('reg-fil-em-b').text()).toBe('e')
        expect(byTestId('reg-fil-i-b').text()).toBe('i')
        byTestId('reg-fil-toggle').click()
        expect(byTestId('reg-fil-em-a').exists()).toBe(false)
        expect(byTestId('reg-fil-i-a').exists()).toBe(false)
        expect(byTestId('reg-fil-em-b').exists()).toBe(false)
        expect(byTestId('reg-fil-i-b').exists()).toBe(false)
        expect(byTestId('reg-fil-tail-a').text()).toBe('t')
        expect(byTestId('reg-fil-tail-b').text()).toBe('t')
    </script>
}

// Guard: a single-node conditional rendering a root-nested component still
// toggles cleanly (the $__uni_mount shared-host fix).
#universal RegStateRootInner(props) {
    state n = 0
    return <InputGroup data-testid="reg-srn-inner">
        <button data-testid="reg-srn-btn" onClick={() => n += 1}>{n}</button>
    </InputGroup>
}

#universal RegStateRootNest(props) {
    state on = true
    return <div data-testid="reg-srn">
        <button data-testid="reg-srn-toggle" onClick={() => on = !on}>t</button>
        {on ? <RegStateRootInner /> : null}
        <span data-testid="reg-srn-tail">tail</span>
    </div>
}

#universal_test("regression: single-node conditional root-nested component toggles cleanly") {
    <RegStateRootNest />
    <script>
        expect(byTestId('reg-srn-btn').text()).toBe('0')
        byTestId('reg-srn-btn').click()
        expect(byTestId('reg-srn-btn').text()).toBe('1')
        byTestId('reg-srn-toggle').click()
        expect(byTestId('reg-srn-btn').exists()).toBe(false)
        expect(byTestId('reg-srn-tail').text()).toBe('tail')
        byTestId('reg-srn-toggle').click()
        expect(byTestId('reg-srn-btn').text()).toBe('0')
    </script>
}

// ===========================================================================
// Class 6: other runtime surfaces
// ===========================================================================

// Keyed list whose items have multi-node (fragment) roots, reordered.
#universal RegKeyFragItem(props) {
    return <>
        <li data-testid={props.tid}>{props.label}</li>
        <li data-testid={props.tid + "-x"}>x</li>
    </>
}

#universal RegKeyFragList(props) {
    state order = ["a", "b"]
    return <ul data-testid="reg-kfl">
        {order.map((x) => <RegKeyFragItem key={x} tid={"reg-kfl-" + x} label={x} />)}
        <button data-testid="reg-kfl-rev" onClick={() => order = ["b", "a"]}>r</button>
    </ul>
}

#universal_test("regression: keyed multi-node items reorder without duplication", isolate) {
    <RegKeyFragList />
    <script>
        const list = byTestId('reg-kfl')
        expect(list.findAll('li').count()).toBe(4)
        byTestId('reg-kfl-rev').click()
        expect(list.findAll('li').count()).toBe(4)
        const items = list.findAll('li')
        expect(items.nth(0)).toHaveAttribute('data-testid', 'reg-kfl-b')
        expect(items.nth(1)).toHaveAttribute('data-testid', 'reg-kfl-b-x')
        expect(items.nth(2)).toHaveAttribute('data-testid', 'reg-kfl-a')
        expect(items.nth(3)).toHaveAttribute('data-testid', 'reg-kfl-a-x')
    </script>
}

// ref callback on a conditionally-rendered element: fires on mount/unmount.
#universal RegRefCond(props) {
    state on = false
    return <div data-testid="reg-rfc">
        <button data-testid="reg-rfc-toggle" onClick={() => on = !on}>t</button>
        {on ? <span data-testid="reg-rfc-el" ref={(el) => { window.__regRefCond = el ? "mounted" : "null" }}>x</span> : null}
    </div>
}

#universal_test("regression: ref callback fires for a conditional element") {
    <RegRefCond />
    <script>
        expect(window.__regRefCond).toBe(undefined)
        byTestId('reg-rfc-toggle').click()
        expect(byTestId('reg-rfc-el').text()).toBe('x')
        expect(window.__regRefCond).toBe('mounted')
    </script>
}

// useReducer counter.
#universal RegReducer(props) {
    const [count, dispatch] = useReducer((s, a) => (a === "inc" ? s + 1 : s), 0)
    return <button data-testid="reg-red" onClick={() => dispatch("inc")}>{count}</button>
}

#universal_test("regression: useReducer dispatches update the view") {
    <RegReducer />
    <script>
        expect(byTestId('reg-red').text()).toBe('0')
        byTestId('reg-red').click()
        byTestId('reg-red').click()
        expect(byTestId('reg-red').text()).toBe('2')
    </script>
}

// Boolean attribute (disabled) reactive toggle.
#universal RegDisabledBind(props) {
    state off = false
    return <div data-testid="reg-db">
        <button data-testid="reg-db-toggle" onClick={() => off = !off}>t</button>
        <button data-testid="reg-db-target" disabled={off}>go</button>
    </div>
}

#universal_test("regression: reactive disabled binding toggles") {
    <RegDisabledBind />
    <script>
        expect(byTestId('reg-db-target').isDisabled()).toBe(false)
        byTestId('reg-db-toggle').click()
        expect(byTestId('reg-db-target').isDisabled()).toBe(true)
        byTestId('reg-db-toggle').click()
        expect(byTestId('reg-db-target').isDisabled()).toBe(false)
    </script>
}

// class merge of a static class and a reactive class.
#universal RegClassMerge(props) {
    state on = false
    return <div data-testid="reg-cm" class={"base " + (on ? "on" : "off")}>
        <button data-testid="reg-cm-toggle" onClick={() => on = !on}>t</button>
    </div>
}

#universal_test("regression: static + reactive class merge updates") {
    <RegClassMerge />
    <script>
        expect(byTestId('reg-cm').hasClass('base')).toBe(true)
        expect(byTestId('reg-cm').hasClass('off')).toBe(true)
        byTestId('reg-cm-toggle').click()
        expect(byTestId('reg-cm').hasClass('on')).toBe(true)
        expect(byTestId('reg-cm').hasClass('off')).toBe(false)
    </script>
}
