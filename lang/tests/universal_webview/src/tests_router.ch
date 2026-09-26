// Universal router WebView tests (design §8.1).
//
// Each fixture declares a router with a UNIQUE name (registries are global on
// the shared page). Assertions read `window.$__uni_routers` directly — the
// runtime maintains `visible`/`hydrated` fields for exactly this purpose.

// ── fixtures ────────────────────────────────────────────────────────────────

#universal UtPaneA(props) {
    state n = 0
    useEffect(() => { window.__utMounts = (window.__utMounts || 0) + 1 }, [])
    return <div>
        <button data-testid="ut-a-btn" onClick={() => n += 1}>inc</button>
        <span data-testid="ut-a-out">Count: {n}</span>
    </div>
}

#universal UtPaneB(props) {
    return <div data-testid="ut-b-pane">B</div>
}

#universal UtMountSection(props) {
    return <section data-testid="ut-sec">Section</section>
}

#universal UtRouterBasic(props) {
    router "ut-basic" {
        route default #"a" { <UtPaneA /> }
        route #"b" { <UtPaneB /> }
    }
}

#universal UtRouterMount(props) {
    router "ut-mount" {
        route default #"s" { <UtMountSection /> }
        route #"t" { <UtMountSection /> }
    }
}

#universal UtRouterHooks(props) {
    router "ut-hooks" {
        route default #"a" {
            onActivate(() => { window.__utHookLog = (window.__utHookLog || []).concat(['act:a']) })
            onDeactivate(() => { window.__utHookLog = (window.__utHookLog || []).concat(['deact:a']) })
            <UtPaneB />
        }
        route #"b" {
            onActivate(() => { window.__utHookLog = (window.__utHookLog || []).concat(['act:b']) })
            <UtPaneB />
        }
    }
}

#universal UtRouterGuard(props) {
    router "ut-guard" {
        route default #"a" { <UtPaneB /> }
        route #"b" {
            onBeforeActivate(() => { return false })
            <UtPaneB />
        }
    }
}

// ── tests ───────────────────────────────────────────────────────────────────

#universal_test("router activates the default route and hydrates it", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        expect(r.routes['a'].visible).toBe(true)
        expect(r.routes['a'].hydrated).toBe(true)
        expect(r.routes['b'].visible).toBe(false)
        expect(r.routes['b'].hydrated).toBe(false)
        byTestId('ut-a-btn').click()
        expect(byTestId('ut-a-out').text()).toBe('Count: 1')
    </script>
}

#universal_test("router navigation preserves state and is O(1) on re-activate", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        // mutate route a
        byTestId('ut-a-btn').click()
        byTestId('ut-a-btn').click()
        expect(byTestId('ut-a-out').text()).toBe('Count: 2')
        expect(window.__utMounts).toBe(1)  // a mounted once
        // activate b
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(r.routes['b'].visible).toBe(true)
        expect(r.routes['b'].hydrated).toBe(true)
        expect(r.routes['a'].visible).toBe(false)
        expect(r.routes['a'].hydrated).toBe(true)  // kept alive
        // return to a: state preserved, no remount
        expect(r.activateRoute('a')).toBe(true)
        await t.sleep(20)
        expect(byTestId('ut-a-out').text()).toBe('Count: 2')
        expect(window.__utMounts).toBe(1)
        expect(r.current()).toBe('a')
    </script>
}

#universal_test("router hides a non-div rooted route and toggles its wrapper", isolate) {
    <UtRouterMount />
    <script>
        const r = window.$__uni_routers['ut-mount']
        const wrap = $('[data-uni-route="ut-mount#s"]')
        expect(wrap.exists()).toBe(true)
        expect(wrap.attr('data-uni-route-active')).toBe('true')
        r.activateRoute('t')
        await t.sleep(20)
        expect($('[data-uni-route="ut-mount#s"]').attr('data-uni-route-active')).toBe('false')
        expect($('[data-uni-route="ut-mount#t"]').attr('data-uni-route-active')).toBe('true')
        // the wrapper is still in the document (never replaced by hydration)
        expect(wrap.exists()).toBe(true)
    </script>
}

#universal_test("router unknown id is contained and keeps the current route", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        expect(r.activateRoute('does-not-exist')).toBe(false)
        await t.sleep(10)
        expect(r.current()).toBe('a')
        expect(r.routes['a'].visible).toBe(true)
        expect(typeof r.$current.value).toBe('string')
    </script>
}

#universal_test("router hooks fire on activate and deactivate", isolate) {
    <UtRouterHooks />
    <script>
        const r = window.$__uni_routers['ut-hooks']
        r.activateRoute('b')
        await t.sleep(20)
        // a deactivated, b activated
        expect(window.__utHookLog).toContain('deact:a')
        expect(window.__utHookLog).toContain('act:b')
    </script>
}

#universal_test("router guard cancels navigation before any DOM change", isolate) {
    <UtRouterGuard />
    <script>
        const r = window.$__uni_routers['ut-guard']
        expect(r.activateRoute('b')).toBe(false)
        await t.sleep(10)
        expect(r.current()).toBe('a')
        expect(r.routes['a'].visible).toBe(true)
        expect(r.routes['b'].visible).toBe(false)
    </script>
}

// ── URL layer ───────────────────────────────────────────────────────────────

#universal UtProjectPane(props) {
    return <div data-testid="ut-proj">Project {props.id}</div>
}

#universal UtRouterUrl(props) {
    router "ut-url" {
        route default #"home" { <div data-testid="ut-url-home">Home</div> }
        route "/projects/{id}" { <UtProjectPane /> }
        route * { <div data-testid="ut-url-404">Not found</div> }
    }
}

#universal_test("router url match delivers params fallback and buildPath", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        expect(r.activateRouteByUrl('/projects/42')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-proj').text()).toBe('Project 42')
        // percent-decoded param (client matcher)
        r.activateRouteByUrl('/projects/a%20b')
        await t.sleep(20)
        expect(byTestId('ut-proj').text()).toBe('Project a b')
        // fallback on a miss
        r.activateRouteByUrl('/nope')
        await t.sleep(20)
        expect(r.current()).toBe('*')
        expect(byTestId('ut-url-404').text()).toBe('Not found')
        // path reversal
        expect(r.buildPath('/projects/{id}', { id: '7' })).toBe('/projects/7')
        // query signal
        r.activateRouteByUrl('/projects/42?tab=1')
        await t.sleep(20)
        expect(r.query().tab).toBe('1')
        r.setQuery({ tab: '2' })
        expect(r.query().tab).toBe('2')
    </script>
}

// ── Link ────────────────────────────────────────────────────────────────────

#universal UtRouterLinks(props) {
    router "ut-links" {
        route default #"home" { <div data-testid="ut-links-home">Home</div> }
        route "/projects" { <div data-testid="ut-links-proj">Projects</div> }
    }
}

#universal UtLinkHost(props) {
    return <div data-testid="ut-link-wrap"><RouterLink href="/projects" router="ut-links">Go</RouterLink></div>
}

#universal_test("router RouterLink intercepts a plain click and the predicate gates modifiers", isolate) {
    <UtRouterLinks />
    <UtLinkHost />
    <script>
        const r = window.$__uni_routers['ut-links']
        const a = byTestId('ut-link-wrap').find('a')
        expect(a.exists()).toBe(true)
        a.click()
        await t.sleep(20)
        expect(r.current()).toBe('/projects')
        // the shared predicate: plain same-origin left-clicks only
        const ev = (extra) => Object.assign({ button: 0, metaKey: false, ctrlKey: false, shiftKey: false, altKey: false,
            defaultPrevented: false, currentTarget: { target: null, hasAttribute: () => false } }, extra)
        expect(window.$__uni_should_intercept(ev({}), '/projects')).toBe(true)
        expect(window.$__uni_should_intercept(ev({ ctrlKey: true }), '/projects')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ button: 1 }), '/projects')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), 'https://example.com/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '#frag')).toBe(false)
    </script>
}

// ── nested routes / Outlet ──────────────────────────────────────────────────

#universal UtNestedChildA(props) {
    return <div data-testid="ut-nested-a">A</div>
}

#universal UtNestedChildB(props) {
    return <div data-testid="ut-nested-b">B</div>
}

#universal UtRouterNested(props) {
    router "ut-nested" {
        route default #"home" { <div data-testid="ut-nested-home">Home</div> }
        route #"projects" {
            route default #"list" { <UtNestedChildA /> }
            route #"detail" { <UtNestedChildB /> }
            <div data-testid="ut-nested-layout"><Outlet /></div>
        }
    }
}

#universal_test("router nested routes activate the default child and switch", isolate) {
    <UtRouterNested />
    <script>
        const r = window.$__uni_routers['ut-nested']
        expect(r.activateRoute('projects')).toBe(true)
        await t.sleep(30)
        const nr = window.$__uni_routers['ut-nested#projects']
        expect(nr !== undefined).toBe(true)
        expect(nr.routes['list'].visible).toBe(true)
        expect(byTestId('ut-nested-a').text()).toBe('A')
        expect(nr.activateRoute('detail')).toBe(true)
        await t.sleep(20)
        expect(nr.routes['detail'].visible).toBe(true)
        expect(nr.routes['list'].visible).toBe(false)
        expect(byTestId('ut-nested-b').text()).toBe('B')
    </script>
}

// ── title / release / preload ───────────────────────────────────────────────

#universal UtRouterTitle(props) {
    router "ut-title" {
        route default #"home" title "Home Title" { <div data-testid="ut-title-home">Home</div> }
        route #"about" title "About Title" { <div data-testid="ut-title-about">About</div> }
    }
}

#universal_test("router title updates document.title on activation", isolate) {
    <UtRouterTitle />
    <script>
        const r = window.$__uni_routers['ut-title']
        expect(document.title).toBe('Home Title')
        r.activateRoute('about')
        await t.sleep(20)
        expect(document.title).toBe('About Title')
        r.activateRoute('home')
        await t.sleep(20)
        expect(document.title).toBe('Home Title')
    </script>
}

#universal_test("router release disposes an inactive route and re-arms it", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(r.release('b')).toBe(false)  // active route cannot be released
        expect(r.activateRoute('a')).toBe(true)
        await t.sleep(20)
        expect(r.release('b')).toBe(true)
        expect(r.routes['b'].hydrated).toBe(false)
        expect(r.routes['b'].inst).toBe(null)
    </script>
}

#universal UtRouterPreload(props) {
    router "ut-preload" {
        route default #"a" { <UtPaneB /> }
        route #"admin" preload { <UtPaneB /> }
    }
}

#universal_test("router preload hydrates a hidden route at load", isolate) {
    <UtRouterPreload />
    <script>
        const r = window.$__uni_routers['ut-preload']
        expect(r.routes['admin'].hydrated).toBe(true)
        expect(r.routes['admin'].visible).toBe(false)
        expect(r.routes['a'].visible).toBe(true)
    </script>
}

// ── nested routes under a URL route (Phase 6) ───────────────────────────────

#universal UtNestedUrlChild(props) {
    return <div data-testid="ut-nurl-child">{props.id}</div>
}

#universal UtRouterNestedUrl(props) {
    router "ut-nurl" {
        route default #"home" { <div data-testid="ut-nurl-home">Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <UtNestedUrlChild /> }
            route #"settings" { <div data-testid="ut-nurl-settings">Settings</div> }
            <div data-testid="ut-nurl-layout"><Outlet /></div>
        }
    }
}

#universal_test("router nested routes under a URL route activate children", isolate) {
    <UtRouterNestedUrl />
    <script>
        const r = window.$__uni_routers['ut-nurl']
        expect(r.activateRouteByUrl('/projects/42')).toBe(true)
        await t.sleep(40)
        expect(r.current()).toBe('/projects/{id}')
        const nr = window.$__uni_routers['ut-nurl#/projects/{id}']
        expect(nr !== undefined).toBe(true)
        expect(nr.routes['overview'].visible).toBe(true)
        expect(byTestId('ut-nurl-child').exists()).toBe(true)
        // switch the nested child independently; the layout stays in place
        nr.activateRoute('settings')
        await t.sleep(20)
        expect(byTestId('ut-nurl-settings').text()).toBe('Settings')
        expect(byTestId('ut-nurl-layout').exists()).toBe(true)
    </script>
}
