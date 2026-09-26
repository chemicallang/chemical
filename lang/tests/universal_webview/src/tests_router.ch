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

// ═══════════════════════════════════════════════════════════════════════════
// Phase 2 — URL-layer hardening: history dispatch, query decoding parity,
// percent round-trips, remote prefetch, nested param inheritance, and probes
// for suspected bugs (design §6.1/§6.3/§6.5/§6.7).
// ═══════════════════════════════════════════════════════════════════════════

// ── fixtures ────────────────────────────────────────────────────────────────

#universal UtP2Pane(props) {
    return <div data-testid="ut-p2-pane">{props.id}</div>
}

#universal UtP2Url(props) {
    router "ut-p2-url" {
        route default #"home" { <div data-testid="ut-p2-home">Home</div> }
        route "/projects/{id}" { <UtP2Pane /> }
        route "/about" title "About Title" { <div data-testid="ut-p2-about">About</div> }
        route * { <div data-testid="ut-p2-404">404</div> }
    }
}

#universal UtP2GuardFlag(props) {
    router "ut-p2-guard" {
        route default #"home" { <div data-testid="ut-p2g-home">Home</div> }
        route "/admin" {
            onBeforeActivate(() => { return window.__utP2Allow })
            <div data-testid="ut-p2g-admin">Admin</div>
        }
    }
}

#universal UtP2Dyn(props) {
    router "ut-p2-dyn" {
        route default #"home" { <div data-testid="ut-p2d-home">Home</div> }
        route #"b" { <div data-testid="ut-p2d-b">B</div> }
    }
}

#universal UtP2NestId(props) {
    return <div data-testid="ut-p2-nid">{props.id}</div>
}

#universal UtP2NestTab(props) {
    return <div data-testid="ut-p2-ntab">{props.tab}</div>
}

#universal UtP2Nested(props) {
    router "ut-p2-nest" {
        route default #"home" { <div data-testid="ut-p2n-home">Home</div> }
        route "/p/{id}" {
            route default #"overview" { <UtP2NestId /> }
            route "/x/{tab}" { <UtP2NestTab /> }
            <div data-testid="ut-p2n-layout"><Outlet /></div>
        }
    }
}

#universal UtP2Remote(props) {
    router "ut-p2-rem" {
        route default #"home" { <div data-testid="ut-p2r-home">Home</div> }
        route "/heavy" remote { <div data-testid="ut-p2r-heavy">Heavy</div> }
    }
}

// An id layout whose only URL child is nested: the initial tail must fall back
// to the id default when the URL does not select the child.
#universal UtP2InitHost(props) {
    router "ut-p2-init" {
        route default #"shell" {
            route "/reports/{id}" { <UtP2Pane /> }
            <div data-testid="ut-p2i-layout"><Outlet /></div>
        }
    }
}

// ── interception policy ─────────────────────────────────────────────────────

#universal_test("router the intercept predicate rejects fragments backslashes and relative hrefs", isolate) {
    <UtP2Dyn />
    <script>
        const ev = (extra) => Object.assign({ button: 0, metaKey: false, ctrlKey: false, shiftKey: false, altKey: false,
            defaultPrevented: false, currentTarget: { target: null, hasAttribute: () => false } }, extra)
        // A bare fragment is never intercepted — the browser owns anchor scrolling.
        expect(window.$__uni_should_intercept(ev({}), '#section')).toBe(false)
        // Relative and empty hrefs are left to the browser.
        expect(window.$__uni_should_intercept(ev({}), 'relative/path')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '')).toBe(false)
        // Plain same-origin paths are intercepted.
        expect(window.$__uni_should_intercept(ev({}), '/')).toBe(true)
        expect(window.$__uni_should_intercept(ev({}), '/a/b')).toBe(true)
        // A same-origin path carrying a hash is still a plain anchor: hash-fragment
        // scrolling is a documented non-goal that must "work via plain anchors",
        // so intercepting it would suppress the scroll entirely.
        expect(window.$__uni_should_intercept(ev({}), '/page#section')).toBe(false)
        // WHATWG parses `\` as `/` in special schemes, so a path starting `\` is
        // protocol-relative and must not be intercepted as an internal route.
        const backslashPath = String.fromCharCode(47, 92) + 'evil.example.com'
        expect(window.$__uni_should_intercept(ev({}), backslashPath)).toBe(false)
    </script>
}

// ── history dispatch ────────────────────────────────────────────────────────

#universal_test("router push replace and identical-activation dispatch to the right history call", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        const pushes = [], replaces = []
        const origPush = history.pushState
        const origReplace = history.replaceState
        const savedOk = window.$__uni_history_ok
        history.pushState = function() { pushes.push(arguments[2]) }
        history.replaceState = function() { replaces.push(arguments[2]) }
        window.$__uni_history_ok = true
        try {
            expect(r.activateRouteByUrl('/projects/1')).toBe(true)
            await t.sleep(10)
            // An identical activation must be an O(1) no-op and write no history.
            expect(r.activateRouteByUrl('/projects/1')).toBe(true)
            await t.sleep(10)
            expect(r.replaceRouteByUrl('/projects/2')).toBe(true)
            await t.sleep(10)
            expect(pushes.length).toBe(1)
            expect(pushes[0]).toBe('/projects/1')
            expect(replaces.length).toBe(1)
            expect(replaces[0]).toBe('/projects/2')
            expect(window.$__uni_url_mem.path).toBe('/projects/2')
        } finally {
            history.pushState = origPush
            history.replaceState = origReplace
            window.$__uni_history_ok = savedOk
        }
    </script>
}

// ── signals ─────────────────────────────────────────────────────────────────

#universal_test("router the $url and $query signals fire only on a real change", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        let urlFires = 0, queryFires = 0
        const uUrl = r.$url.subscribe(() => { urlFires++ })
        const uQuery = r.$query.subscribe(() => { queryFires++ })
        r.activateRouteByUrl('/projects/1?a=1')
        await t.sleep(10)
        const u1 = urlFires, q1 = queryFires
        expect(u1).toBeGreaterThan(0)
        expect(q1).toBeGreaterThan(0)
        // Identical re-activation is a no-op: no signal write.
        r.activateRouteByUrl('/projects/1?a=1')
        await t.sleep(10)
        expect(urlFires).toBe(u1)
        expect(queryFires).toBe(q1)
        // A real URL change fires $url but not $query (query unchanged).
        r.activateRouteByUrl('/projects/2?a=1')
        await t.sleep(10)
        expect(urlFires).toBe(u1 + 1)
        expect(queryFires).toBe(q1)
        uUrl(); uQuery()
    </script>
}

#universal_test("router $url is the normalized URL and $query is null on an id route", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        expect(r.$url.value).toBe(null)
        expect(r.query()).toBe(null)
        r.activateRouteByUrl('/projects/9?tab=1')
        await t.sleep(10)
        expect(r.$url.value).toBe('/projects/9')
        expect(r.currentUrl()).toBe('/projects/9')
        expect(r.query().tab).toBe('1')
        r.deactivate()
        expect(r.$url.value).toBe(null)
        expect(r.query()).toBe(null)
    </script>
}

// ── query decoding parity ───────────────────────────────────────────────────

#universal_test("router client query decoding keeps plus literal handles bare keys and malformed percent", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        r.activateRouteByUrl('/projects/1?q=a+b&flag&u=%E2%9C%93&bad=%E0%A4%A')
        await t.sleep(10)
        // decodeURIComponent never turns `+` into a space (parity with the server).
        expect(r.query().q).toBe('a+b')
        // A bare key is an empty string.
        expect(r.query().flag).toBe('')
        // Percent-encoded unicode is decoded.
        expect(r.query().u).toBe(String.fromCharCode(0x2713))
        // Malformed percent input degrades to the raw text, never throws.
        expect(r.query().bad).toBe('%E0%A4%A')
    </script>
}

#universal_test("router query stays empty until the URL actually carries a query", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        expect(r.query()).toBe(null)
        // A URL route with no query writes no signal (null == empty by signature).
        r.activateRouteByUrl('/about')
        await t.sleep(10)
        expect(r.query() === null || Object.keys(r.query()).length === 0).toBe(true)
        // A query-bearing URL sets the parsed object...
        r.activateRouteByUrl('/projects/1?a=1')
        await t.sleep(10)
        expect(r.query().a).toBe('1')
        // ...and moving to a query-less URL empties it again.
        r.activateRouteByUrl('/projects/2')
        await t.sleep(10)
        expect(Object.keys(r.query()).length).toBe(0)
    </script>
}

#universal_test("router setQuery encodes clears and preserves the current path", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        r.activateRouteByUrl('/projects/5')
        await t.sleep(10)
        r.setQuery({ q: 'a b/c', 'k&1': 'v=2' })
        await t.sleep(10)
        expect(r.query()['q']).toBe('a b/c')
        expect(r.query()['k&1']).toBe('v=2')
        expect(r.currentRoute.rawUrl).toBe('/projects/5?q=a%20b%2Fc&k%261=v%3D2')
        expect(r.currentUrl()).toBe('/projects/5')
        // An empty object removes the query and leaves the path intact.
        r.setQuery({})
        await t.sleep(10)
        expect(r.currentRoute.rawUrl).toBe('/projects/5')
        expect(r.currentUrl()).toBe('/projects/5')
    </script>
}

// ── path building / normalization ───────────────────────────────────────────

#universal_test("router buildPath percent-encodes and round-trips through the client matcher", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        const vals = ['a b', 'a/b', 'he%', 'caf' + String.fromCharCode(233), 'sym&=']
        let i = 0
        while(i < vals.length) {
            const v = vals[i]
            const p = r.buildPath('/projects/{id}', { id: v })
            expect(r.activateRouteByUrl(p)).toBe(true)
            await t.sleep(10)
            // Split-then-decode must recover the exact original param.
            expect(byTestId('ut-p2-pane').text()).toBe(v)
            i = i + 1
        }
    </script>
}

#universal_test("router duplicate and trailing slashes still match one route", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        expect(r.activateRouteByUrl('//projects//7//')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-p2-pane').text()).toBe('7')
    </script>
}

#universal_test("router normPath strips query and one trailing slash and never throws", isolate) {
    <UtP2Dyn />
    <script>
        const r = window.$__uni_routers['ut-p2-dyn']
        expect(r.normPath('/a/?x=1')).toBe('/a')
        expect(r.normPath('/a//')).toBe('/a/')     // exactly one trailing slash is removed
        expect(r.normPath('/')).toBe('/')
        expect(r.normPath('')).toBe('/')
        expect(r.normPath('a')).toBe('a')          // a leading slash is never invented
        // A fragment must not survive normalization: `normPath` is used for both
        // matching and link-active comparison, so a hash would poison both.
        expect(r.normPath('/a#f')).toBe('/a')
    </script>
}

#universal_test("router decode_segment degrades malformed percent input to the raw text", isolate) {
    <UtP2Dyn />
    <script>
        expect(window.$__uni_decode_segment('a%20b')).toBe('a b')
        expect(window.$__uni_decode_segment('%2F')).toBe('/')
        expect(window.$__uni_decode_segment('a+b')).toBe('a+b')
        expect(window.$__uni_decode_segment('%E0%A4%A')).toBe('%E0%A4%A')
    </script>
}

#universal_test("router set_table_base affects matching path building and currentUrl", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        window.$__uni_set_table_base('ut-p2-url', '/app')
        expect(r.table.base).toBe('/app')
        expect(r.buildPath('/about', {})).toBe('/app/about')
        expect(r.activateRouteByUrl('/app/about')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/about')
        expect(r.currentUrl()).toBe('/app/about')
        expect(r.$url.value).toBe('/app/about')
        // A non-base-prefixed URL still matches the same table.
        expect(r.activateRouteByUrl('/about')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/about')
        expect(r.currentUrl()).toBe('/about')
        window.$__uni_set_table_base('ut-p2-url', '')
        expect(r.table.base).toBe('')
    </script>
}

// ── URL miss / fallback ─────────────────────────────────────────────────────

#universal_test("router a fragment on an activation URL does not corrupt the matched param", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        // A browser strips the hash before the router ever sees a real request,
        // and `activateRouteByUrl` is also fed hrefs straight from `<RouterLink>`,
        // which may carry a hash. Matching must ignore it.
        expect(r.activateRouteByUrl('/projects/42#top')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-p2-pane').text()).toBe('42')
    </script>
}

// ── title ───────────────────────────────────────────────────────────────────

#universal_test("router a URL route title applies on activation and resets on deactivate", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        r.activateRouteByUrl('/about')
        await t.sleep(10)
        expect(document.title).toBe('About Title')
        r.activateRouteByUrl('/projects/3')
        await t.sleep(10)
        expect(document.title).toBe(window.$__uni_base_title)
        r.deactivate()
        expect(document.title).toBe(window.$__uni_base_title)
    </script>
}

// ── guards ──────────────────────────────────────────────────────────────────

#universal_test("router a guard flag can deny then allow the same navigation", isolate) {
    <UtP2GuardFlag />
    <script>
        const r = window.$__uni_routers['ut-p2-guard']
        const orig = window.$__uni_router_error
        let errs = 0
        window.$__uni_router_error = () => { errs++ }
        window.__utP2Allow = false
        expect(r.activateRouteByUrl('/admin')).toBe(false)
        expect(errs).toBe(1)
        expect(r.current()).toBe('home')
        expect(r.routes['/admin'].visible).toBe(false)
        window.__utP2Allow = true
        expect(r.activateRouteByUrl('/admin')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/admin')
        expect(r.routes['/admin'].visible).toBe(true)
        window.$__uni_router_error = orig
    </script>
}

// ── deactivate ──────────────────────────────────────────────────────────────

#universal_test("router deactivate without an active route is a silent no-op", isolate) {
    <UtP2Dyn />
    <script>
        const r = window.$__uni_routers['ut-p2-dyn']
        r.deactivate()
        r.deactivate()
        expect(r.current()).toBe(null)
        expect(r.currentUrl()).toBe(null)
        expect(r.query()).toBe(null)
        r.activateRoute('b')
        await t.sleep(10)
        r.deactivate()
        expect(r.current()).toBe(null)
        expect(r.routes['b'].visible).toBe(false)
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('b')
    </script>
}

// ── nested params ───────────────────────────────────────────────────────────

#universal_test("router a nested URL child receives its own and the inherited params", isolate) {
    <UtP2Nested />
    <script>
        const r = window.$__uni_routers['ut-p2-nest']
        expect(r.activateRouteByUrl('/p/7/x/b')).toBe(true)
        await t.sleep(40)
        expect(r.current()).toBe('/p/{id}')
        expect(r.currentUrl()).toBe('/p/7/x/b')
        const nr = window.$__uni_routers['ut-p2-nest#/p/{id}']
        expect(nr !== undefined).toBe(true)
        expect(nr.current()).toBe('/x/{tab}')
        // The child renders its own {tab}...
        expect(byTestId('ut-p2-ntab').text()).toBe('b')
        // ...and inherits the ancestor {id} in its merged params.
        expect(nr.routes['/x/{tab}'].params.id).toBe('7')
        expect(nr.routes['/x/{tab}'].params.tab).toBe('b')
        // buildPath resolves both the root and the nested child to full paths.
        expect(r.buildPath('/p/{id}', { id: '7' })).toBe('/p/7')
        expect(r.buildPath('/x/{tab}', { id: '7', tab: 'b' })).toBe('/p/7/x/b')
    </script>
}

#universal_test("router a nested param change remounts the child and keeps the layout", isolate) {
    <UtP2Nested />
    <script>
        const r = window.$__uni_routers['ut-p2-nest']
        r.activateRouteByUrl('/p/1/x/a')
        await t.sleep(40)
        const nr = window.$__uni_routers['ut-p2-nest#/p/{id}']
        expect(byTestId('ut-p2-ntab').text()).toBe('a')
        expect(byTestId('ut-p2n-layout').exists()).toBe(true)
        // Change the ancestor param: the layout DOM must survive and the child
        // must re-derive the new inherited param.
        r.activateRouteByUrl('/p/2/x/a')
        await t.sleep(40)
        expect(byTestId('ut-p2n-layout').exists()).toBe(true)
        expect(nr.current()).toBe('/x/{tab}')
        expect(nr.routes['/x/{tab}'].params.id).toBe('2')
        expect(byTestId('ut-p2-ntab').text()).toBe('a')
        // A change of the child's own param remounts it with the new value.
        r.activateRouteByUrl('/p/3/x/c')
        await t.sleep(40)
        expect(byTestId('ut-p2-ntab').text()).toBe('c')
        expect(nr.routes['/x/{tab}'].params.id).toBe('3')
        // Returning to the bare layout URL re-derives the default child.
        r.activateRouteByUrl('/p/4')
        await t.sleep(40)
        expect(nr.current()).toBe('overview')
        expect(byTestId('ut-p2-nid').text()).toBe('4')
    </script>
}

// ── initial activation tail ─────────────────────────────────────────────────

#universal_test("router activate_initial follows the URL chain and falls back to the id default", isolate) {
    <UtP2InitHost />
    <script>
        const r = window.$__uni_routers['ut-p2-init']
        expect(r.current()).toBe('shell')
        // A deep link selects the nested URL child through the emitted chain.
        expect(window.$__uni_activate_initial('ut-p2-init', 'shell', '/reports/w')).toBe(true)
        await t.sleep(40)
        expect(r.current()).toBe('shell')
        const nr = window.$__uni_routers['ut-p2-init#shell']
        expect(nr.current()).toBe('/reports/{id}')
        expect(byTestId('ut-p2-pane').text()).toBe('w')
        // A URL that does not select the child falls back to the id default.
        r.deactivate()
        expect(window.$__uni_activate_initial('ut-p2-init', 'shell', '/')).toBe(true)
        await t.sleep(40)
        expect(r.current()).toBe('shell')
        expect(r.routes['shell'].visible).toBe(true)
    </script>
}

// ── remote fetch-on-demand ──────────────────────────────────────────────────

#universal_test("router a remote prefetch stores the fragment without mounting it", isolate) {
    <UtP2Remote />
    <script>
        const r = window.$__uni_routers['ut-p2-rem']
        const rec = r.routes['/heavy']
        const origFetch = window.fetch
        window.fetch = function() {
            return Promise.resolve({ ok: true, text: function() { return Promise.resolve('<div data-chx-i></div>') } })
        }
        try {
            expect(r.preload('/heavy')).toBe(true)
            await t.sleep(40)
            expect(rec.fragment).toBe('<div data-chx-i></div>')
            expect(rec.inFlight).toBe(null)
            expect(rec.hydrated).toBe(false)
            expect(rec.visible).toBe(false)
        } finally { window.fetch = origFetch }
    </script>
}

#universal_test("router a remote prefetch in flight is not started twice and blocks release", isolate) {
    <UtP2Remote />
    <script>
        const r = window.$__uni_routers['ut-p2-rem']
        const origFetch = window.fetch
        let calls = 0
        window.fetch = function() { calls++; return new Promise(function() {}) }
        try {
            expect(r.preload('/heavy')).toBe(true)
            expect(r.preload('/heavy')).toBe(true)
            expect(calls).toBe(1)
            // An in-flight route cannot be released.
            expect(r.release('/heavy')).toBe(false)
        } finally { window.fetch = origFetch }
    </script>
}

#universal_test("router a remote route activates after a successful fragment fetch", isolate) {
    <UtP2Remote />
    <script>
        const r = window.$__uni_routers['ut-p2-rem']
        const rec = r.routes['/heavy']
        const origFetch = window.fetch
        // A remote route ships no client body: the fetched fragment IS the
        // pre-rendered route markup (the `[data-chx-i]` boundary host aside).
        const fragment = '<div data-chx-i><div data-testid="ut-p2r-heavy">Heavy</div></div>'
        window.fetch = function() {
            return Promise.resolve({ ok: true, text: function() { return Promise.resolve(fragment) } })
        }
        try {
            expect(r.activateRouteByUrl('/heavy')).toBe(true)
            await t.sleep(80)
            expect(rec.fragment).toBe(fragment)
            expect(rec.visible).toBe(true)
            expect(r.current()).toBe('/heavy')
            expect(byTestId('ut-p2r-heavy').text()).toBe('Heavy')
        } finally { window.fetch = origFetch }
    </script>
}

// ── traversal segments ──────────────────────────────────────────────────────

#universal_test("router dot and dotdot segments are never captured by a param", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        expect(r.activateRouteByUrl('/projects/42')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-p2-pane').text()).toBe('42')
        // `.`/`..` are not route data (D-6.1): the `{id}` param must refuse them,
        // so the `route *` fallback catches the path instead.
        expect(r.activateRouteByUrl('/projects/..')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('*')
        expect(byTestId('ut-p2-404').text()).toBe('404')
        expect(r.activateRouteByUrl('/projects/.')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('*')
        // The percent-encoded forms (`%2E`/`%2E%2E`, and a mixed `%2E.`) are also
        // rejected, matching the server matcher after it decodes the segment.
        expect(r.activateRouteByUrl('/projects/%2E%2E')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('*')
        expect(r.activateRouteByUrl('/projects/%2e')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('*')
        expect(r.activateRouteByUrl('/projects/%2E.')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('*')
        // A normal percent-encoded param is still captured (no regression).
        expect(r.activateRouteByUrl('/projects/a%2Eb')).toBe(true)
        await t.sleep(10)
        expect(r.current()).toBe('/projects/{id}')
        expect(byTestId('ut-p2-pane').text()).toBe('a.b')
    </script>
}

// ── buildPath edge cases ────────────────────────────────────────────────────

#universal_test("router buildPath for the fallback id reports and returns null on the client", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        const orig = window.$__uni_router_error
        let errs = 0
        window.$__uni_router_error = () => { errs++ }
        const p = r.buildPath('*', {})
        window.$__uni_router_error = orig
        // The client table skips fallback entries when reversing a path, so a
        // fallback id is an unknown target (null + one contained error).
        // NB: the server `build_path` returns "/" for the fallback id — see the
        // `--libs` test — so this is a deliberate, tested client/server split.
        expect(p).toBe(null)
        expect(errs).toBe(1)
        // A real route still reverses normally.
        expect(r.buildPath('/projects/{id}', { id: '5' })).toBe('/projects/5')
    </script>
}

// ── preload / release of a never-hydrated route ─────────────────────────────

#universal_test("router preload and release round-trip a route that was never hydrated", isolate) {
    <UtP2Dyn />
    <script>
        const r = window.$__uni_routers['ut-p2-dyn']
        const rec = r.routes['b']
        expect(rec.hydrated).toBe(false)
        expect(rec.inst).toBe(null)
        // preload hydrates a hidden route without showing it.
        expect(r.preload('b')).toBe(true)
        expect(rec.hydrated).toBe(true)
        expect(rec.visible).toBe(false)
        // release disposes it and re-arms it.
        expect(r.release('b')).toBe(true)
        expect(rec.hydrated).toBe(false)
        expect(rec.inst).toBe(null)
        // Releasing an already-released (never re-hydrated) route is a no-op success.
        expect(r.release('b')).toBe(true)
        expect(rec.hydrated).toBe(false)
        // It can still be activated fresh afterwards.
        expect(r.activateRoute('b')).toBe(true)
        await t.sleep(10)
        expect(rec.visible).toBe(true)
        expect(rec.hydrated).toBe(true)
    </script>
}

// ── relative hrefs on the control components ────────────────────────────────

#universal UtP2RelHost(props) {
    return <div data-testid="ut-p2rel-wrap"><RouterLink href="sub/page" router="ut-p2-dyn">Rel</RouterLink></div>
}

#universal_test("router a relative href is rendered verbatim and never intercepted", isolate) {
    <UtP2Dyn />
    <UtP2RelHost />
    <script>
        const ev = (extra) => Object.assign({ button: 0, metaKey: false, ctrlKey: false, shiftKey: false, altKey: false,
            defaultPrevented: false, currentTarget: { target: null, hasAttribute: () => false } }, extra)
        // The component preserves the relative href...
        const a = byTestId('ut-p2rel-wrap').find('a')
        expect(a.exists()).toBe(true)
        expect(a.attr('href')).toBe('sub/page')
        // ...and relative (non-rooted) paths are left to the browser, so the link
        // can never be turned into a router activation.
        expect(window.$__uni_should_intercept(ev({}), 'sub/page')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), './a')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '../a')).toBe(false)
        expect(window.$__uni_should_intercept(ev({}), '?q=1')).toBe(false)
        // Rooted same-origin paths remain interceptable.
        expect(window.$__uni_should_intercept(ev({}), '/projects')).toBe(true)
    </script>
}

// ── signal / state semantics ────────────────────────────────────────────────

#universal_test("router isActive accepts a URL pattern id and an id activation nulls $url", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        expect(r.isActive('home')).toBe(true)
        r.activateRouteByUrl('/projects/3')
        await t.sleep(10)
        expect(r.isActive('/projects/{id}')).toBe(true)
        expect(r.isActive('home')).toBe(false)
        expect(r.$url.value).toBe('/projects/3')
        // Switching back to the id route clears the URL signal.
        expect(r.activateRoute('home')).toBe(true)
        await t.sleep(10)
        expect(r.isActive('home')).toBe(true)
        expect(r.$url.value).toBe(null)
        expect(r.currentUrl()).toBe(null)
    </script>
}

#universal_test("router setQuery then a same-path different-query activation is a real change", isolate) {
    <UtP2Url />
    <script>
        const r = window.$__uni_routers['ut-p2-url']
        r.activateRouteByUrl('/projects/1?a=1')
        await t.sleep(10)
        expect(r.query().a).toBe('1')
        // Same normalized path but a different raw URL: the O(1) no-op compare
        // must include rawUrl, so this re-runs and updates the query signal.
        expect(r.activateRouteByUrl('/projects/1?a=2')).toBe(true)
        await t.sleep(10)
        expect(r.query().a).toBe('2')
        expect(r.currentRoute.rawUrl).toBe('/projects/1?a=2')
        expect(r.currentUrl()).toBe('/projects/1')
    </script>
}

#universal_test("router deactivating a nested layout preserves nested selection for reactivation", isolate) {
    <UtP2Nested />
    <script>
        const r = window.$__uni_routers['ut-p2-nest']
        r.activateRouteByUrl('/p/9/x/z')
        await t.sleep(40)
        const nr = window.$__uni_routers['ut-p2-nest#/p/{id}']
        expect(nr.current()).toBe('/x/{tab}')
        r.deactivate()
        await t.sleep(10)
        expect(r.current()).toBe(null)
        expect(r.routes['/p/{id}'].visible).toBe(false)
        // Deactivation hides the outer route but never disposes nested state.
        expect(nr.current()).toBe('/x/{tab}')
        expect(nr.routes['/x/{tab}'].visible).toBe(true)
        // Reactivating by id (no URL chain) re-derives the nested default.
        expect(r.activateRoute('/p/{id}')).toBe(true)
        await t.sleep(40)
        expect(r.routes['/p/{id}'].visible).toBe(true)
        expect(nr.current()).toBe('overview')
    </script>
}
