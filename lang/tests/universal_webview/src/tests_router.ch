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
        // the nested child inherited the outer URL route's {id} param
        expect(byTestId('ut-nurl-child').text()).toBe('42')
        // switch the nested child independently; the layout stays in place
        nr.activateRoute('settings')
        await t.sleep(20)
        expect(byTestId('ut-nurl-settings').text()).toBe('Settings')
        expect(byTestId('ut-nurl-layout').exists()).toBe(true)
    </script>
}

// ── full URL nesting (Phase 6, flattened patterns + activation chain) ────────

#universal UtNestedUrlDeepChild(props) {
    return <div data-testid="ut-nud-child">{props.id}</div>
}

#universal UtRouterNestedUrlDeep(props) {
    router "ut-nud" {
        route default #"home" { <div data-testid="ut-nud-home">Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <UtNestedUrlDeepChild /> }
            route "/settings" { <div data-testid="ut-nud-settings">Settings</div> }
            <div data-testid="ut-nud-layout"><Outlet /></div>
        }
    }
}

#universal_test("router nested URL routes match the full path and activate the chain", isolate) {
    <UtRouterNestedUrlDeep />
    <script>
        const r = window.$__uni_routers['ut-nud']
        expect(r.activateRouteByUrl('/projects/42/settings')).toBe(true)
        await t.sleep(40)
        // the outer layout route is active and owns the full URL
        expect(r.current()).toBe('/projects/{id}')
        expect(r.currentUrl()).toBe('/projects/42/settings')
        expect(byTestId('ut-nud-layout').exists()).toBe(true)
        expect(byTestId('ut-nud-settings').text()).toBe('Settings')
        const nr = window.$__uni_routers['ut-nud#/projects/{id}']
        expect(nr.current()).toBe('/settings')
        // buildPath resolves both the root route and a nested URL route id
        expect(r.buildPath('/projects/{id}', { id: 42 })).toBe('/projects/42')
        expect(r.buildPath('/settings', { id: 42 })).toBe('/projects/42/settings')
        // navigating to the parent URL re-derives the nested default
        expect(r.activateRouteByUrl('/projects/7')).toBe(true)
        await t.sleep(40)
        expect(nr.current()).toBe('overview')
        expect(byTestId('ut-nud-layout').exists()).toBe(true)
        expect(byTestId('ut-nud-child').text()).toBe('7')
        // back to a nested child on a different param: the layout must survive
        expect(r.activateRouteByUrl('/projects/9/settings')).toBe(true)
        await t.sleep(40)
        expect(nr.current()).toBe('/settings')
        expect(byTestId('ut-nud-settings').text()).toBe('Settings')
    </script>
}

#universal_test("router nested URL renders the child from the URL on first load", isolate) {
    // The test harness mounts the fixture; drive the initial URL through the
    // client matcher the same way a deep link does.
    <UtRouterNestedUrlDeep />
    <script>
        const r = window.$__uni_routers['ut-nud']
        expect(r.activateRouteByUrl('/projects/11/settings')).toBe(true)
        await t.sleep(30)
        const nr = window.$__uni_routers['ut-nud#/projects/{id}']
        expect(nr.current()).toBe('/settings')
        expect(r.current()).toBe('/projects/{id}')
    </script>
}

// ── nested `route *` fallback (prefix match under a URL layout) ─────────────

#universal UtNestedFbChild(props) {
    return <div data-testid="ut-nfb-pane">Missing</div>
}

#universal UtRouterNestedFallback(props) {
    router "ut-nfb" {
        route default #"home" { <div data-testid="ut-nfb-home">Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <div data-testid="ut-nfb-overview">Overview</div> }
            route "/settings" { <div data-testid="ut-nfb-settings">Settings</div> }
            route * { <UtNestedFbChild /> }
            <div data-testid="ut-nfb-lay"><Outlet /></div>
        }
    }
}

#universal_test("router nested fallback catches an unknown remainder client-side", isolate) {
    <UtRouterNestedFallback />
    <script>
        const r = window.$__uni_routers['ut-nfb']
        expect(r.activateRouteByUrl('/projects/42/unknown/deeper')).toBe(true)
        await t.sleep(40)
        expect(r.current()).toBe('/projects/{id}')
        const nr = window.$__uni_routers['ut-nfb#/projects/{id}']
        expect(nr.current()).toBe('*')
        expect(byTestId('ut-nfb-pane').exists()).toBe(true)
        // an exact nested child still wins over the fallback
        expect(r.activateRouteByUrl('/projects/42/settings')).toBe(true)
        await t.sleep(40)
        expect(nr.current()).toBe('/settings')
        expect(byTestId('ut-nfb-settings').text()).toBe('Settings')
    </script>
}

// ── hydrated outer layout (native layout root gets a client function) ───────

#universal UtHydratedLayoutChild(props) {
    return <div data-testid="ut-hl-child">Child</div>
}

#universal UtRouterHydratedLayout(props) {
    router "ut-hl" {
        route default #"home" { <div data-testid="ut-hl-home">Home</div> }
        route "/projects/{id}" {
            route default #"overview" { <UtHydratedLayoutChild /> }
            route "/settings" { <div data-testid="ut-hl-settings">Settings</div> }
            <div data-testid="ut-hl-layout">
                <button data-testid="ut-hl-btn" onClick={() => { window.__utHlClicks = (window.__utHlClicks || 0) + 1 }}>inc</button>
                <Outlet />
            </div>
        }
    }
}

#universal_test("router hydrates a native layout and preserves it across child switches", isolate) {
    <UtRouterHydratedLayout />
    <script>
        const r = window.$__uni_routers['ut-hl']
        window.__utHlClicks = 0
        expect(r.activateRouteByUrl('/projects/42')).toBe(true)
        await t.sleep(50)
        // the layout is hydrated: its button handler runs
        byTestId('ut-hl-btn').click()
        await t.sleep(10)
        expect(window.__utHlClicks).toBe(1)
        // the default child is visible and the outlet wrappers were adopted
        expect(byTestId('ut-hl-child').exists()).toBe(true)
        const nr = window.$__uni_routers['ut-hl#/projects/{id}']
        expect(nr.current()).toBe('overview')
        // switch the child; the layout DOM (and its handler) must survive
        nr.activateRoute('/settings')
        await t.sleep(20)
        expect(byTestId('ut-hl-settings').text()).toBe('Settings')
        byTestId('ut-hl-btn').click()
        await t.sleep(10)
        expect(window.__utHlClicks).toBe(2)
        expect(byTestId('ut-hl-layout').exists()).toBe(true)
    </script>
}

// ── route-root compile-time props reach the client component ───────────────

#universal UtRoutePropPane(props) {
    return <div data-testid="ut-rp-pane">{props.title}</div>
}

#universal UtRoutePropPaneB(props) {
    return <div data-testid="ut-rp-pane-b">{props.title}</div>
}

#universal UtRoutePropApp(props) {
    router "ut-rp" {
        route default #"a" { <UtRoutePropPane title="alpha" /> }
        route #"b" { <UtRoutePropPaneB title="beta" /> }
    }
}

#universal_test("router route-root attributes reach the client component", isolate) {
    <UtRoutePropApp />
    <script>
        const r = window.$__uni_routers['ut-rp']
        expect(byTestId('ut-rp-pane').text()).toBe('alpha')
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(30)
        expect(byTestId('ut-rp-pane-b').text()).toBe('beta')
    </script>
}

// ── component-rooted layout that forwards children ─────────────────────────

#universal UtForwardLayout(props) {
    return <div data-testid="ut-fw-layout">
        <span data-testid="ut-fw-chrome">chrome</span>
        {props.children}
    </div>
}

#universal UtRouterForwardLayout(props) {
    router "ut-fw" {
        route default #"home" { <div data-testid="ut-fw-home">Home</div> }
        route #"shell" {
            route default #"a" { <div data-testid="ut-fw-a">A</div> }
            route #"b" { <div data-testid="ut-fw-b">B</div> }
            <UtForwardLayout><Outlet /></UtForwardLayout>
        }
    }
}

#universal_test("router hydrates a component-rooted layout that forwards children", isolate) {
    <UtRouterForwardLayout />
    <script>
        const r = window.$__uni_routers['ut-fw']
        expect(r.activateRoute('shell')).toBe(true)
        await t.sleep(40)
        expect(byTestId('ut-fw-layout').exists()).toBe(true)
        expect(byTestId('ut-fw-chrome').text()).toBe('chrome')
        expect(byTestId('ut-fw-a').text()).toBe('A')
        const nr = window.$__uni_routers['ut-fw#shell']
        nr.activateRoute('b')
        await t.sleep(20)
        expect(byTestId('ut-fw-b').text()).toBe('B')
        // the layout survives the child switch
        expect(byTestId('ut-fw-chrome').text()).toBe('chrome')
    </script>
}

// ── Outlet inside a separate layout component's own body ───────────────────

#universal UtBodyOutletLayout(props) {
    return <div data-testid="ut-bo-layout">
        <span data-testid="ut-bo-chrome">chrome</span>
        <Outlet />
    </div>
}

#universal UtRouterBodyOutlet(props) {
    router "ut-bo" {
        route default #"home" { <div data-testid="ut-bo-home">Home</div> }
        route #"shell" {
            route default #"a" { <div data-testid="ut-bo-a">A</div> }
            route #"b" { <div data-testid="ut-bo-b">B</div> }
            <UtBodyOutletLayout />
        }
    }
}

#universal_test("router renders an Outlet inside a separate layout component", isolate) {
    <UtRouterBodyOutlet />
    <script>
        const r = window.$__uni_routers['ut-bo']
        expect(r.activateRoute('shell')).toBe(true)
        await t.sleep(50)
        expect(byTestId('ut-bo-layout').exists()).toBe(true)
        expect(byTestId('ut-bo-chrome').text()).toBe('chrome')
        expect(byTestId('ut-bo-a').text()).toBe('A')
        // the wrappers were relocated into the layout's own outlet slot
        const slot = byTestId('ut-bo-layout').find('[data-uni-outlet="true"]')
        expect(slot.exists()).toBe(true)
        expect(slot.find('[data-uni-route="ut-bo#shell#a"]').exists()).toBe(true)
        const nr = window.$__uni_routers['ut-bo#shell']
        nr.activateRoute('b')
        await t.sleep(20)
        expect(byTestId('ut-bo-b').text()).toBe('B')
        expect(byTestId('ut-bo-chrome').text()).toBe('chrome')
    </script>
}

// ═══════════════════════════════════════════════════════════════════════════
// Extended behavioural matrix (design §8.1/§8.2, invariant register §15.5).
// ═══════════════════════════════════════════════════════════════════════════

// ── navigation / state machine ──────────────────────────────────────────────

#universal UtNEffectPane(props) {
    useEffect(() => { window.__utEffOnce = (window.__utEffOnce || 0) + 1 }, [])
    return <div data-testid="ut-eff-pane">P</div>
}

#universal UtNHydPane(props) {
    useEffect(() => { window.__utHydRuns = (window.__utHydRuns || 0) + 1 }, [])
    return <div data-testid="ut-hyd-pane">P</div>
}

#universal UtNRouterHydration(props) {
    router "ut-hyd" {
        route default #"a" { <UtNHydPane /> }
        route #"b" { <UtNHydPane /> }
        route #"c" preload { <UtNHydPane /> }
    }
}

#universal UtNRouterEffects(props) {
    router "ut-eff" {
        route default #"a" { <UtNEffectPane /> }
        route #"b" { <UtNEffectPane /> }
    }
}

#universal UtNRouterPreloadThenActivate(props) {
    router "ut-plta" {
        route default #"a" { <div data-testid="ut-plta-a">A</div> }
        route #"admin" preload { <UtNEffectPane /> }
    }
}

#universal UtNReleasePane(props) {
    state n = 0
    useEffect(() => { window.__utRelR = (window.__utRelR || 0) + 1 }, [])
    return <div><span data-testid="ut-relr-out">N {n}</span></div>
}

#universal UtNRouterRelease(props) {
    router "ut-relr" {
        route default #"a" { <div data-testid="ut-relr-a">A</div> }
        route #"b" { <UtNReleasePane /> }
    }
}

#universal UtNRouterNoScroll(props) {
    router "ut-nsc" {
        route default #"a" { <div data-testid="ut-nsc-a">A</div> }
        route #"b" noscroll { <div data-testid="ut-nsc-b">B</div> }
    }
}

#universal UtNHookOrder(props) {
    router "ut-ho" {
        route default #"a" {
            onDeactivate(() => { window.__utHOLog = (window.__utHOLog || []).concat(['deact:a']) })
            <div data-testid="ut-ho-a">A</div>
        }
        route #"b" {
            onActivate(() => { window.__utHOLog = (window.__utHOLog || []).concat(['act:b']) })
            <div data-testid="ut-ho-b">B</div>
        }
    }
}

#universal UtNHookThrow(props) {
    router "ut-ht" {
        route default #"a" { <div data-testid="ut-ht-a">A</div> }
        route #"b" {
            onActivate(() => { throw new Error('boom') })
            <div data-testid="ut-ht-b">B</div>
        }
    }
}

#universal UtNReentrant(props) {
    router "ut-re" {
        route default #"a" { <div data-testid="ut-re-a">A</div> }
        route #"b" {
            onActivate(() => {
                if(!window.__utReentry) {
                    window.__utReentry = 1
                    window.__utReenterGo()
                }
            })
            <div data-testid="ut-re-b">B</div>
        }
        route #"c" { <div data-testid="ut-re-c">C</div> }
    }
}

#universal UtNGuardUrl(props) {
    router "ut-gu" {
        route default #"home" { <div data-testid="ut-gu-home">Home</div> }
        route "/ok" {
            onBeforeActivate(() => { return true })
            <div data-testid="ut-gu-ok">OK</div>
        }
        route "/no" {
            onBeforeActivate(() => { return false })
            <div data-testid="ut-gu-no">NO</div>
        }
    }
}

#universal UtNGuardBack(props) {
    router "ut-gb" {
        route default "/" { <div data-testid="ut-gb-home">Home</div> }
        route "/blocked" {
            onBeforeActivate(() => { return false })
            <div data-testid="ut-gb-blocked">Blocked</div>
        }
    }
}

#universal UtNMultiA(props) {
    router "ut-ma" {
        route default #"x" { <div data-testid="ut-ma-x">X</div> }
        route #"y" { <div data-testid="ut-ma-y">Y</div> }
    }
}

#universal UtNMultiB(props) {
    router "ut-mb" {
        route default #"p" { <div data-testid="ut-mb-p">P</div> }
        route #"q" { <div data-testid="ut-mb-q">Q</div> }
    }
}

#universal UtNRouterMulti(props) {
    return <div data-testid="ut-multi">
        <UtNMultiA />
        <UtNMultiB />
    </div>
}

#universal UtNRouterFocus(props) {
    router "ut-fo" {
        route default #"a" { <div>
            <input data-testid="ut-fo-in" />
            <span data-testid="ut-fo-mark">A</span>
        </div> }
        route #"b" { <div data-testid="ut-fo-b">B</div> }
    }
}

#universal UtNParamPane(props) {
    state n = 0
    useEffect(() => { window.__utPrMounts = (window.__utPrMounts || 0) + 1 }, [])
    return <div>
        <span data-testid="ut-pr-id">{props.id}</span>
        <button data-testid="ut-pr-btn" onClick={() => n += 1}>inc</button>
        <span data-testid="ut-pr-count">Count: {n}</span>
    </div>
}

#universal UtNRouterParamRemount(props) {
    router "ut-pr" {
        route default #"home" { <div data-testid="ut-pr-home">Home</div> }
        route "/p/{id}" { <UtNParamPane /> }
    }
}

#universal UtNUrlIdPane(props) {
    return <div data-testid="ut-up-id">{props.id}</div>
}

#universal UtNRouterUrlParams(props) {
    router "ut-up" {
        route default #"home" { <div data-testid="ut-up-home">Home</div> }
        route "/projects/{id}" { <UtNUrlIdPane /> }
        route * { <div data-testid="ut-up-404">Not found</div> }
    }
}

#universal UtNNfPane(props) {
    return <div data-testid="ut-nf-id">{props.id}</div>
}

#universal UtNRouterNoFallback(props) {
    router "ut-unf" {
        route default #"home" { <div data-testid="ut-unf-home">Home</div> }
        route "/projects/{id}" { <UtNNfPane /> }
    }
}

#universal UtNNavHost(props) {
    return <div data-testid="ut-nnav-wrap"><NavLink href="/projects" router="ut-links">Go</NavLink></div>
}

#universal UtNUrlLinkRouter(props) {
    router "ut-nul" {
        route default #"home" { <div data-testid="ut-nul-home">Home</div> }
        route "/projects/{id}" { <UtProjectPane /> }
    }
}

#universal UtNUrlLinkHost(props) {
    return <div data-testid="ut-nul-wrap"><RouterLink href="/projects/1" router="ut-nul">P1</RouterLink></div>
}

#universal UtNPreloadRouter(props) {
    router "ut-npl" {
        route default #"home" { <div data-testid="ut-npl-home">Home</div> }
        route #"admin" { <UtPaneB /> }
    }
}

#universal UtNPreloadHost(props) {
    return <div data-testid="ut-npl-wrap"><RouterLink href="/admin" routeId="admin" router="ut-npl" preload>Admin</RouterLink></div>
}

#universal UtNNestedStateLayout(props) {
    state n = 0
    return <div data-testid="ut-ns-lay">
        <button data-testid="ut-ns-btn" onClick={() => n += 1}>inc</button>
        <span data-testid="ut-ns-count">Count: {n}</span>
        {props.children}
    </div>
}

#universal UtNRouterNestedState(props) {
    router "ut-nst" {
        route default #"home" { <div data-testid="ut-nst-home">Home</div> }
        route #"shell" {
            route default #"a" { <div data-testid="ut-nst-a">A</div> }
            route #"b" { <div data-testid="ut-nst-b">B</div> }
            <UtNNestedStateLayout><Outlet /></UtNNestedStateLayout>
        }
    }
}

#universal_test("router re-activating the current route is an O(1) no-op", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        let cur = 0, url = 0
        const u1 = r.$current.subscribe(() => { cur++ })
        const u2 = r.$url.subscribe(() => { url++ })
        expect(r.activateRoute('a')).toBe(true)
        await t.sleep(10)
        expect(cur).toBe(0)
        expect(url).toBe(0)
        expect(r.current()).toBe('a')
        u1(); u2()
    </script>
}

#universal_test("router exactly one route is visible after every navigation", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        function visibleIds() {
            const out = []
            for(const k in r.routes) { if(r.routes[k].visible) out.push(k) }
            return out.join(',')
        }
        expect(visibleIds()).toBe('a')
        r.activateRoute('b'); await t.sleep(20)
        expect(visibleIds()).toBe('b')
        r.activateRoute('a'); await t.sleep(20)
        expect(visibleIds()).toBe('a')
        expect(r.currentRoute.id).toBe('a')
    </script>
}

#universal_test("router deactivate hides the current route and clears signals", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        r.deactivate()
        await t.sleep(10)
        expect(r.current()).toBe(null)
        expect(r.$current.value).toBe(null)
        expect(r.$url.value).toBe(null)
        expect(r.routes['a'].visible).toBe(false)
        expect(r.currentRoute).toBe(null)
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('b')
        expect(r.routes['b'].visible).toBe(true)
    </script>
}

#universal_test("router unknown id reports exactly one contained error and keeps the previous route", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        const res = r.activateRoute('nope')
        window.$__uni_router_error = orig
        expect(res).toBe(false)
        expect(errs.length).toBe(1)
        expect(r.current()).toBe('a')
        expect(r.routes['a'].visible).toBe(true)
        expect(r.routes['b'].visible).toBe(false)
    </script>
}

#universal_test("router accessor returns a null object for an absent router", isolate) {
    <UtRouterBasic />
    <script>
        const n = window.$__uni_router('does-not-exist')
        expect(typeof n.current).toBe('function')
        expect(n.current()).toBe(null)
        expect(n.currentUrl()).toBe(null)
        expect(n.activateRoute('x')).toBe(false)
        expect(n.isActive('a')).toBe(false)
        expect(n.query()).toBe(null)
    </script>
}

#universal_test("router unknown-route methods report without throwing", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        expect(r.preload('nope')).toBe(false)
        expect(r.release('nope')).toBe(false)
        expect(r.activateRouteByUrl('/nope')).toBe(false)
        expect(r.current()).toBe('a')
    </script>
}

#universal_test("router mount host is the boundary span and never the wrapper", isolate) {
    <UtRouterBasic />
    <script>
        const rec = window.$__uni_routers['ut-basic'].routes['a']
        expect(rec.el !== rec.host).toBe(true)
        expect(rec.el.classList.contains('chx-route')).toBe(true)
        expect(rec.host.hasAttribute('data-chx-i')).toBe(true)
    </script>
}

#universal_test("router multiple routers on a page are independent", isolate) {
    <UtNRouterMulti />
    <script>
        const a = window.$__uni_routers['ut-ma']
        const b = window.$__uni_routers['ut-mb']
        expect(a.routes['x'].visible).toBe(true)
        expect(b.routes['p'].visible).toBe(true)
        a.activateRoute('y')
        await t.sleep(20)
        expect(a.current()).toBe('y')
        expect(b.current()).toBe('p')
        expect(b.routes['p'].visible).toBe(true)
        expect(b.routes['q'].visible).toBe(false)
        b.activateRoute('q')
        await t.sleep(20)
        expect(b.current()).toBe('q')
        expect(a.current()).toBe('y')
        expect(a.routes['x'].visible).toBe(false)
    </script>
}

#universal_test("router isActive and normPath reflect state and normalize paths", isolate) {
    <UtRouterBasic />
    <script>
        const r = window.$__uni_routers['ut-basic']
        expect(r.isActive('a')).toBe(true)
        expect(r.isActive('b')).toBe(false)
        expect(r.normPath('/a/?x=1')).toBe('/a')
        expect(r.normPath('/a/')).toBe('/a')
        expect(r.normPath('/')).toBe('/')
        expect(r.normPath('')).toBe('/')
        expect(r.currentUrl()).toBe(null)
    </script>
}

// ── hydration / modes / effects ─────────────────────────────────────────────

#universal_test("router only the default and preloaded routes hydrate at load", isolate) {
    <UtNRouterHydration />
    <script>
        const r = window.$__uni_routers['ut-hyd']
        expect(r.routes['a'].hydrated).toBe(true)
        expect(r.routes['b'].hydrated).toBe(false)
        expect(r.routes['b'].inst).toBe(null)
        expect(!!r.routes['b'].host.$__uni_instance).toBe(false)
        expect(r.routes['c'].hydrated).toBe(true)
        expect(r.routes['c'].visible).toBe(false)
        expect(window.__utHydRuns).toBe(2)
    </script>
}

#universal_test("router effects run exactly once across activate deactivate reactivate", isolate) {
    <UtNRouterEffects />
    <script>
        const r = window.$__uni_routers['ut-eff']
        expect(window.__utEffOnce).toBe(1)
        r.activateRoute('b')
        await t.sleep(20)
        expect(r.routes['b'].visible).toBe(true)
        expect(window.__utEffOnce).toBe(2)
        r.activateRoute('a')
        await t.sleep(20)
        expect(window.__utEffOnce).toBe(2)
        r.activateRoute('b')
        await t.sleep(20)
        expect(window.__utEffOnce).toBe(2)
    </script>
}

#universal_test("router preload then activate does not double mount", isolate) {
    <UtNRouterPreloadThenActivate />
    <script>
        const r = window.$__uni_routers['ut-plta']
        expect(r.routes['admin'].hydrated).toBe(true)
        expect(window.__utEffOnce).toBe(1)
        expect(r.activateRoute('admin')).toBe(true)
        await t.sleep(20)
        expect(r.routes['admin'].visible).toBe(true)
        expect(window.__utEffOnce).toBe(1)
        r.activateRoute('a')
        await t.sleep(20)
        expect(window.__utEffOnce).toBe(1)
    </script>
}

#universal_test("router release re-arms an inactive route for a fresh mount", isolate) {
    <UtNRouterRelease />
    <script>
        const r = window.$__uni_routers['ut-relr']
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(window.__utRelR).toBe(1)
        expect(r.activateRoute('a')).toBe(true)
        await t.sleep(20)
        expect(r.release('b')).toBe(true)
        expect(r.routes['b'].hydrated).toBe(false)
        expect(r.routes['b'].inst).toBe(null)
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(r.routes['b'].visible).toBe(true)
        expect(window.__utRelR).toBe(2)
        expect(r.release('b')).toBe(false)
    </script>
}

#universal_test("router noscroll route registers its flag and activates", isolate) {
    <UtNRouterNoScroll />
    <script>
        const r = window.$__uni_routers['ut-nsc']
        expect(r.routes['b'].noscroll).toBe(true)
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(20)
        expect(r.routes['b'].visible).toBe(true)
    </script>
}

#universal_test("router focus returns to the route's remembered element across a switch", isolate) {
    <UtNRouterFocus />
    <script>
        const r = window.$__uni_routers['ut-fo']
        const input = byTestId('ut-fo-in')
        input.focus()
        expect(input.isFocused()).toBe(true)
        r.activateRoute('b')
        await t.sleep(20)
        expect(r.routes['a'].visible).toBe(false)
        expect(!!r.routes['a'].focusEl).toBe(true)
        r.activateRoute('a')
        await t.sleep(20)
        expect(r.routes['a'].el.contains(document.activeElement)).toBe(true)
    </script>
}

// ── hooks ───────────────────────────────────────────────────────────────────

#universal_test("router hooks fire deactivate before activate and only on transitions", isolate) {
    <UtNHookOrder />
    <script>
        const r = window.$__uni_routers['ut-ho']
        r.activateRoute('b')
        await t.sleep(20)
        expect((window.__utHOLog || []).join(',')).toBe('deact:a,act:b')
    </script>
}

#universal_test("router guard allow navigates and deny keeps the current url", isolate) {
    <UtNGuardUrl />
    <script>
        const r = window.$__uni_routers['ut-gu']
        expect(r.activateRouteByUrl('/ok')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('/ok')
        expect(r.routes['/ok'].visible).toBe(true)
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        expect(r.activateRouteByUrl('/no')).toBe(false)
        window.$__uni_router_error = orig
        expect(errs.length).toBe(1)
        expect(r.current()).toBe('/ok')
        expect(r.routes['/ok'].visible).toBe(true)
        expect(r.routes['/no'].visible).toBe(false)
    </script>
}

#universal_test("router throwing hook is isolated and the activation still succeeds", isolate) {
    <UtNHookThrow />
    <script>
        const r = window.$__uni_routers['ut-ht']
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        const ok = r.activateRoute('b')
        window.$__uni_router_error = orig
        expect(ok).toBe(true)
        expect(r.current()).toBe('b')
        expect(r.routes['b'].visible).toBe(true)
        expect(errs.length).toBe(1)
    </script>
}

#universal_test("router hook-initiated navigation is queued and applied after the transition", isolate) {
    <UtNReentrant />
    <script>
        const r = window.$__uni_routers['ut-re']
        window.__utReenterGo = () => { r.activateRoute('c') }
        expect(r.activateRoute('b')).toBe(true)
        expect(window.__utReentry).toBe(1)
        expect(r.current()).toBe('c')
        expect(r.routes['c'].visible).toBe(true)
        expect(r.routes['b'].visible).toBe(false)
    </script>
}

#universal_test("router guard denial on popstate re-syncs the URL to the visible route", isolate) {
    <UtNGuardBack />
    <script>
        window.$__uni_history_ok = false
        const r = window.$__uni_routers['ut-gb']
        expect(r.activateRouteByUrl('/')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('/')
        window.$__uni_url_mem.path = '/blocked'
        window.dispatchEvent(new PopStateEvent('popstate'))
        await t.sleep(20)
        expect(r.current()).toBe('/')
        expect(window.$__uni_url_mem.path).toBe('/')
        expect(r.routes['/blocked'].visible).toBe(false)
    </script>
}

#universal_test("router popstate re-derives the route from the in-memory URL", isolate) {
    <UtRouterUrl />
    <script>
        window.$__uni_history_ok = false
        const r = window.$__uni_routers['ut-url']
        r.activateRouteByUrl('/projects/5')
        await t.sleep(20)
        expect(r.current()).toBe('/projects/{id}')
        window.$__uni_url_mem.path = '/nope'
        window.dispatchEvent(new PopStateEvent('popstate'))
        await t.sleep(20)
        expect(r.current()).toBe('*')
        expect(byTestId('ut-url-404').text()).toBe('Not found')
        window.$__uni_url_mem.path = '/projects/8'
        window.dispatchEvent(new PopStateEvent('popstate'))
        await t.sleep(20)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-proj').text()).toBe('Project 8')
    </script>
}

// ── URL layer ───────────────────────────────────────────────────────────────

#universal_test("router percent-decodes client params and splits before decoding", isolate) {
    <UtNRouterUrlParams />
    <script>
        const r = window.$__uni_routers['ut-up']
        r.activateRouteByUrl('/projects/a%20b')
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe('a b')
        r.activateRouteByUrl('/projects/a%2Fb')
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe('a/b')
        r.activateRouteByUrl('/projects/a+b')
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe('a+b')
        r.activateRouteByUrl('/projects/he%27s')
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe("he's")
    </script>
}

#universal_test("router trailing slash and query are normalized out of the match", isolate) {
    <UtNRouterUrlParams />
    <script>
        const r = window.$__uni_routers['ut-up']
        expect(r.activateRouteByUrl('/projects/42/')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('/projects/{id}')
        expect(r.currentUrl()).toBe('/projects/42')
        expect(byTestId('ut-up-id').text()).toBe('42')
    </script>
}

#universal_test("router client base stripping and buildPath include the mount base", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        r.table.base = '/app'
        expect(r.activateRouteByUrl('/app/projects/7')).toBe(true)
        await t.sleep(20)
        expect(byTestId('ut-proj').text()).toBe('Project 7')
        expect(r.buildPath('/projects/{id}', { id: '9' })).toBe('/app/projects/9')
        r.table.base = ''
    </script>
}

#universal_test("router query parses last-wins and setQuery updates the signal and URL", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        r.activateRouteByUrl('/projects/3?a=1&a=2&b=3')
        await t.sleep(20)
        expect(r.query().a).toBe('2')
        expect(r.query().b).toBe('3')
        r.setQuery({ q: 'x y' })
        expect(r.query().q).toBe('x y')
        expect(r.currentRoute.rawUrl).toBe('/projects/3?q=x%20y')
        expect(r.currentUrl()).toBe('/projects/3')
    </script>
}

#universal_test("router buildPath percent-encodes params and yields empty segments", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        expect(r.buildPath('/projects/{id}', { id: 'a/b' })).toBe('/projects/a%2Fb')
        expect(r.buildPath('/projects/{id}', {})).toBe('/projects/')
        expect(r.buildPath('/projects/{id}', { id: '7' })).toBe('/projects/7')
    </script>
}

#universal_test("router a buildPath for an unknown route reports and returns null", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        const p = r.buildPath('nope', {})
        window.$__uni_router_error = orig
        expect(p).toBe(null)
        expect(errs.length).toBe(1)
    </script>
}

#universal_test("router a URL miss without a fallback keeps the current route", isolate) {
    <UtNRouterNoFallback />
    <script>
        const r = window.$__uni_routers['ut-unf']
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        const res = r.activateRouteByUrl('/definitely/missing')
        window.$__uni_router_error = orig
        expect(res).toBe(false)
        expect(errs.length).toBe(1)
        expect(r.current()).toBe('home')
    </script>
}

#universal_test("router replaceRouteByUrl activates without an extra history entry", isolate) {
    <UtRouterUrl />
    <script>
        const r = window.$__uni_routers['ut-url']
        expect(r.activateRouteByUrl('/projects/1')).toBe(true)
        await t.sleep(20)
        expect(r.replaceRouteByUrl('/projects/2')).toBe(true)
        await t.sleep(20)
        expect(byTestId('ut-proj').text()).toBe('Project 2')
        expect(r.currentUrl()).toBe('/projects/2')
        expect(r.replaceRoute('home')).toBe(true)
        await t.sleep(20)
        expect(r.current()).toBe('home')
    </script>
}

#universal_test("router activateRoute with params and activateRouteByUrl both deliver params", isolate) {
    <UtNRouterUrlParams />
    <script>
        const r = window.$__uni_routers['ut-up']
        r.activateRoute('/projects/{id}', { id: '7' })
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe('7')
        expect(r.current()).toBe('/projects/{id}')
        r.activateRouteByUrl('/projects/42')
        await t.sleep(20)
        expect(byTestId('ut-up-id').text()).toBe('42')
        expect(r.currentUrl()).toBe('/projects/42')
    </script>
}

// ── param-change remount ────────────────────────────────────────────────────

#universal_test("router a param change remounts once and clears stale state", isolate) {
    <UtNRouterParamRemount />
    <script>
        const r = window.$__uni_routers['ut-pr']
        r.activateRouteByUrl('/p/1')
        await t.sleep(20)
        expect(byTestId('ut-pr-id').text()).toBe('1')
        expect(window.__utPrMounts).toBe(1)
        byTestId('ut-pr-btn').click()
        expect(byTestId('ut-pr-count').text()).toBe('Count: 1')
        r.activateRouteByUrl('/p/2')
        await t.sleep(20)
        expect(byTestId('ut-pr-id').text()).toBe('2')
        expect(window.__utPrMounts).toBe(2)
        expect(byTestId('ut-pr-count').text()).toBe('Count: 0')
        expect(r.routes['/p/{id}'].ssr).toBe(false)
        r.activateRouteByUrl('/p/1')
        await t.sleep(20)
        expect(byTestId('ut-pr-id').text()).toBe('1')
        expect(window.__utPrMounts).toBe(3)
        expect(r.routes['/p/{id}'].baseProps['id']).toBe(null)
    </script>
}

// ── links / reactive state ──────────────────────────────────────────────────

#universal_test("router RouterLink exposes aria-current from the $url signal", isolate) {
    <UtRouterLinks />
    <UtLinkHost />
    <script>
        const r = window.$__uni_routers['ut-links']
        const a = byTestId('ut-link-wrap').find('a')
        expect(a.attr('aria-current')).toBe(null)
        a.click()
        await t.sleep(20)
        expect(r.current()).toBe('/projects')
        expect(a.attr('aria-current')).toBe('page')
        r.activateRoute('home')
        await t.sleep(20)
        expect(a.attr('aria-current')).toBe(null)
    </script>
}

#universal_test("router NavLink adds the active class and aria-current", isolate) {
    <UtRouterLinks />
    <UtNNavHost />
    <script>
        const r = window.$__uni_routers['ut-links']
        const a = byTestId('ut-nnav-wrap').find('a')
        expect(a.hasClass('is-active')).toBe(false)
        a.click()
        await t.sleep(20)
        expect(a.hasClass('chx-navlink')).toBe(true)
        expect(a.hasClass('is-active')).toBe(true)
        expect(a.attr('aria-current')).toBe('page')
    </script>
}

#universal_test("router a URL link is param-aware via the $url signal", isolate) {
    <UtNUrlLinkRouter />
    <UtNUrlLinkHost />
    <script>
        const r = window.$__uni_routers['ut-nul']
        const a = byTestId('ut-nul-wrap').find('a')
        r.activateRouteByUrl('/projects/1')
        await t.sleep(20)
        expect(a.attr('aria-current')).toBe('page')
        r.activateRouteByUrl('/projects/2')
        await t.sleep(20)
        expect(a.attr('aria-current')).toBe(null)
    </script>
}

#universal_test("router RouterLink preload hydrates a hidden route on hover", isolate) {
    <UtNPreloadRouter />
    <UtNPreloadHost />
    <script>
        const r = window.$__uni_routers['ut-npl']
        expect(r.routes['admin'].hydrated).toBe(false)
        byTestId('ut-npl-wrap').find('a').hover()
        await t.sleep(20)
        expect(r.routes['admin'].hydrated).toBe(true)
        expect(r.routes['admin'].visible).toBe(false)
        expect(r.current()).toBe('home')
    </script>
}

#universal_test("router the shared intercept predicate gates modifiers targets and schemes", isolate) {
    <UtRouterBasic />
    <script>
        const ev = (extra) => Object.assign({ button: 0, metaKey: false, ctrlKey: false, shiftKey: false, altKey: false,
            defaultPrevented: false, currentTarget: { target: null, hasAttribute: () => false } }, extra)
        expect(window.$__uni_should_intercept(ev({}), '/x')).toBe(true)
        expect(window.$__uni_should_intercept(ev({ ctrlKey: true }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ shiftKey: true }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ altKey: true }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ button: 2 }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ defaultPrevented: true }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '//evil.example.com/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), 'https://example.com/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), 'mailto:a@b.c')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), 'tel:123')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '#frag')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ currentTarget: { target: '_blank', hasAttribute: () => false } }), '/x')).toBe(false)
        expect(window.$__uni_should_intercept(ev({ currentTarget: { target: null, hasAttribute: (n) => n === 'download' } }), '/x')).toBe(false)
    </script>
}

// ── nested router state ─────────────────────────────────────────────────────

#universal_test("router nested component layout keeps its own state across child switches", isolate) {
    <UtNRouterNestedState />
    <script>
        const r = window.$__uni_routers['ut-nst']
        expect(r.activateRoute('shell')).toBe(true)
        await t.sleep(40)
        expect(byTestId('ut-nst-a').text()).toBe('A')
        byTestId('ut-ns-btn').click()
        expect(byTestId('ut-ns-count').text()).toBe('Count: 1')
        const nr = window.$__uni_routers['ut-nst#shell']
        nr.activateRoute('b')
        await t.sleep(20)
        expect(byTestId('ut-nst-b').text()).toBe('B')
        expect(byTestId('ut-ns-count').text()).toBe('Count: 1')
        expect(byTestId('ut-ns-lay').exists()).toBe(true)
    </script>
}

#universal_test("router nested router owns independent signals", isolate) {
    <UtRouterNested />
    <script>
        const r = window.$__uni_routers['ut-nested']
        expect(r.activateRoute('projects')).toBe(true)
        await t.sleep(30)
        const nr = window.$__uni_routers['ut-nested#projects']
        expect(r.$current.value).toBe('projects')
        expect(nr.$current.value).toBe('list')
        nr.activateRoute('detail')
        await t.sleep(20)
        expect(nr.$current.value).toBe('detail')
        expect(r.$current.value).toBe('projects')
        expect(byTestId('ut-nested-b').text()).toBe('B')
    </script>
}

// ── title / base title ──────────────────────────────────────────────────────

#universal_test("router deactivate restores the base document title", isolate) {
    <UtRouterTitle />
    <script>
        const r = window.$__uni_routers['ut-title']
        expect(document.title).toBe('Home Title')
        r.activateRoute('about')
        await t.sleep(20)
        expect(document.title).toBe('About Title')
        r.deactivate()
        expect(document.title).toBe(window.$__uni_base_title)
    </script>
}

// ── remote routes / preload idempotency / hide rule ─────────────────────────

#universal UtNRemote(props) {
    router "ut-rem" {
        route default #"home" { <div data-testid="ut-rem-home">Home</div> }
        route "/heavy" remote { <div data-testid="ut-rem-heavy">Heavy</div> }
    }
}

#universal UtNRouterPreloadIdem(props) {
    router "ut-pli" {
        route default #"a" { <div data-testid="ut-pli-a">A</div> }
        route #"admin" { <UtNEffectPane /> }
    }
}

#universal_test("router remote route ships no body and a failed fetch keeps the previous route", isolate) {
    <UtNRemote />
    <script>
        const r = window.$__uni_routers['ut-rem']
        const rec = r.routes['/heavy']
        expect(rec.remote).toBe(true)
        expect(!!rec.fragment).toBe(false)
        expect(byTestId('ut-rem-heavy').exists()).toBe(false)
        const orig = window.$__uni_router_error
        let errs = []
        window.$__uni_router_error = (m) => { errs.push(m) }
        let threw = false
        try { r.activateRouteByUrl('/heavy') } catch(e) { threw = true }
        expect(threw).toBe(false)
        await t.sleep(250)
        expect(errs.length >= 1).toBe(true)
        window.$__uni_router_error = orig
        expect(r.current()).toBe('home')
        expect(r.routes['home'].visible).toBe(true)
        expect(rec.visible).toBe(false)
        expect(!!rec.fragment).toBe(false)
    </script>
}

#universal_test("router preload is idempotent and mounts at most once", isolate) {
    <UtNRouterPreloadIdem />
    <script>
        const r = window.$__uni_routers['ut-pli']
        expect(r.preload('admin')).toBe(true)
        expect(r.preload('admin')).toBe(true)
        expect(window.__utEffOnce).toBe(1)
        expect(r.routes['admin'].hydrated).toBe(true)
        expect(r.routes['admin'].visible).toBe(false)
    </script>
}

#universal_test("router the hide rule keeps a non-div rooted inactive route hidden", isolate) {
    <UtRouterMount />
    <script>
        const r = window.$__uni_routers['ut-mount']
        r.activateRoute('t')
        await t.sleep(20)
        const wrapS = $('[data-uni-route="ut-mount#s"]')
        expect(wrapS.attr('data-uni-route-active')).toBe('false')
        expect(wrapS.css('display')).toBe('none')
        expect(wrapS.find('[data-testid="ut-sec"]').isVisible()).toBe(false)
        expect($('[data-uni-route="ut-mount#t"]').css('display')).not.toBe('none')
    </script>
}
