# Universal Router Design

**Status:** Design (pre-implementation). Audited for production readiness (§11),
pre-mortemed for silent failure modes (§12), and re-audited mechanism-by-mechanism
against the runtime source (§13). This document is the implementability-reviewed
plan for the universal component router: a server-rendered, lazy-hydrating,
id-based route activation system with an opt-in URL layer.

**Implementation decision record (§14):** the Phase-0 decisions — lexer/grammar,
AST shape, converter emission points, package graph, URL matching rules, runtime
state machine, and a frozen diagnostics catalogue — are in §14. Start there before
writing code; three of those decisions were found because the design as written
would not have compiled or would have pulled `net`/`tls` into every routed app.

**Final implementation blueprint (§15):** the build-ready section — exact files,
symbols, tests and commands per phase (§15.1), the normative runtime source
(§15.2), byte-exact emission templates (§15.3), compiler-side signatures (§15.4),
the 20-row invariant register with the test that enforces each one (§15.5),
measurement methods for every performance budget (§15.6), the per-phase merge gate
(§15.7), and the quality fixes this pass applied to the earlier sections (§15.9).
**Where §15 and an earlier section disagree, §15 wins.**

**Second-pass note (§13):** the design was re-checked line-by-line against
`lang/libs/page/src/page.ch` and the `#html` converter. Four mechanism claims were
factually wrong (the error path throws; root-mode mount adopts/replaces the host;
`$__uni_dispose` leaves DOM behind; the server match assumed a pattern table that
cannot exist before render) and one server integration was impossible as written.
Those are corrected in place and catalogued in §13.1; the rest of §13 is the
deployment/perf/SEO/accessibility work that follows from them.

**Scope:** `lang/libs/page`, `lang/libs/router` (new), `lang/libs/universal_cbi`,
`lang/libs/universal_parser`, `lang/libs/js_syntax`, `lang/libs/html_cbi`, and the
universal test suites.

Companion reading:
- `.agents/skills/router/SKILL.md` — the app-author quick reference (route
  syntax, control API, components, server setup, deployment modes) **and** the
  build/maintenance map; kept in sync with every router change. Library sources
  ship **comment docs above the code**, not Markdown, so there is no
  `lang/libs/router/README.md`.
- `conversation/RouterFast.txt` — server-parameter ideation (basis for §3).
- `conversation/Hydrate Routes On Demand.pdf` — lifecycle ideation (basis for §2).
- `lang/docs/universal-runtime-professionalization-plan.md` — the pipeline plan this
  router builds on (phased hydration, manifest protocol, diagnostics).

---

## 0. Principles (from the ideation docs, sharpened)

1. **HTML exists independently of interactivity.** Server-rendered DOM is inert until
   its component hydrates. The router decides *when* that happens.
2. **Routing and hydration are separate concerns.** The router only declares which
   route is active; a hydration manager decides what must become live. Route change
   is never "destroy + create", it is "deactivate + activate".
3. **The primitive is imperative and cheap:** `router.activateRoute("id")`. URLs,
   query parameters, history, and guards are layers above, never inside the core.
4. **Everything the user did not write never happens.** A route whose page was never
   generated 404s at the server. A route id the compiler never saw is a compile
   diagnostic, not a runtime string-miss.
5. **The importer pays the cost.** Everything lives in a `router` library as
   extension functions and extension macros; `page` gains only the generic parameter
   store (§3), so apps that never route don't ship one byte of router code.
6. **No string lookups in hot paths.** Route ids are matched on interned pointers /
   integers client-side; the compiler proves every id the runtime can be asked for.

---

## 1. Lifecycle model

The runtime distinguishes five states per route component (per the hydration ideation):

```
SERVER_RENDERED  ->  UNHYDRATED  ->  HYDRATING  ->  HYDRATED
                                     (effects run once, here)
active flag (visibility) is orthogonal and changes often.
```

Invariants:

- **Hydration happens at most once per component instance** (the runtime already
  guarantees instance disposal on re-mount; the router only mounts once, so effects
  never re-run when a route becomes visible again).
- **`activate()` = `ensureHydrated()` + show.** `preload(id)` (§5.2) = hydrate without
  showing. Visibility (`display:none` toggling) is an implementation detail of
  `activate`/`deactivate`, not the abstraction.
- **Deactivation never disposes.** Navigating away keeps state, DOM, and listeners
  alive so returning is a pure visibility switch. Unmount happens only when the
  owning subtree is disposed (existing `$__uni_dispose` machinery).

Why not destroy-on-navigate: activation of an already-hydrated route must be O(1) —
an attribute write. Destroy/create would re-run every effect, re-fetch, and lose
scroll and input state; the ideation explicitly rejects this and it also matches the
runtime's disposal model.

---

## 2. Core primitive (client)

### 2.1 The router registry

Generated per router, emitted once per page:

```js
// emitted into the segmented component-definitions section
window.$__uni_routers = window.$__uni_routers || Object.create(null);
window.$__uni_routers["main-router"] = {
    currentRoute: null,          // active route record (`r.currentRoute`, NOT `r.current`)
    $current: null,              // $_us signal: active route **id** (§5.4)
    $url: null,                  // $_us signal: active **resolved url** (null for id routes)
    routes: {                    // filled by the route activation stubs
        // "projects": { el, host, comp, baseProps, params, props, hydrated: false,
        //               visible: false, ssr: true, url: null, inst: null }
    }
};

// See §15.2 for the authoritative, complete runtime source (this block is the
// same code; §15.2 is the single frozen copy that gets emitted).
```

Record fields:

| Field | Meaning |
|---|---|
| `el` | the router-owned **wrapper** element (hide/show, focus, scroll target) |
| `host` | the `[data-chx-i]` boundary element **inside** the wrapper — the actual mount host |
| `comp` | the generated route client function (or `null`, §2.4) |
| `baseProps` | the route's compile-time serialized props — **never mutated** |
| `params` | current URL params for this record (`null` for id routes), merged over `baseProps` (§6.4) |
| `props` | derived: `baseProps` with `params` merged in; recomputed at each mount, never stored mutated |
| `hydrated` | client function has been mounted at least once |
| `visible` | mirrors `data-uni-route-active` (assertion aid, §12.4) |
| `ssr` | the **host currently holds SSR markup for this record's `url`**; `false` → next mount is fresh (clears the host first) |
| `url` | for URL routes, the resolved path this record serves (record identity for param routes is the *pattern*; `url` is the resolved value, §6.4) |
| `inst` | the mounted instance handle (`host.$__uni_instance`) for dispose-on-param-change |
| `failed` | mount/host failure; contained and never retried in a loop (D-7.6) |
| `scrollY` / `focusEl` | remembered position and focus target for restore (§6.7) |
| `beforeActivate` / `onActivate` / `onDeactivate` | hook callbacks (§5.3) |

`activateRoute` is a small, monomorphic function:

```js
window.$__uni_router_error = ((message, details = "") => {
    // Recoverable router faults must NOT throw — a failed navigation has to
    // leave the previous route active and the page usable. ($__uni_error THROWS
    // by design, for programming errors; $__uni_dispatch likewise logs a missing
    // target and continues instead of aborting the hydration flush loop.)
    console.error("[router] " + message + (details ? ": " + details : ""));
})

window.$__uni_activate = ((routerName, routeId, url, params) => {
    if(window.$__uni_router_busy) {              // re-entrant (from a hook): FIFO, never inline
        window.$__uni_router_queue.push([routerName, routeId, url, params]);
        return true;
    }
    window.$__uni_router_busy = true;
    var ok = window.$__uni_activate_now(routerName, routeId, url, params);
    window.$__uni_router_busy = false;
    while(window.$__uni_router_queue.length) {   // drain in order; last wins via fast path
        var q = window.$__uni_router_queue.shift();
        window.$__uni_router_busy = true;
        window.$__uni_activate_now(q[0], q[1], q[2], q[3]);
        window.$__uni_router_busy = false;
    }
    return ok;
});

window.$__uni_activate_now = ((routerName, routeId, url, params) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) {
        window.$__uni_router_error("activateRoute: unknown route", routerName + "#" + routeId);
        return false;                            // previous route stays active
    }
    if(route.failed) {                           // D-7.6: contained, never retried in a loop
        window.$__uni_router_error("activateRoute: route failed earlier", routerName + "#" + routeId);
        return false;
    }
    url = (url === undefined) ? route.url : url;
    if(r.currentRoute === route && route.url === url) return true;   // exact no-op — O(1)
    if(r.currentRoute === route) {               // param change (§6.4): SSR range is stale
        if(route.inst) window.$__uni_dispose(route.inst);
        route.inst = null;
        route.hydrated = false; route.ssr = false;
    }
    if(route.beforeActivate && route.beforeActivate(url) === false) {
        window.$__uni_router_error("navigation cancelled", routerName + "#" + routeId);
        return false;                            // nothing mutated; current route intact
    }
    route.url = url;                             // set BEFORE mount: props derive from it
    route.params = params || null;
    // Hydrate BEFORE touching the visible route, so a mount failure cannot blank the
    // page (order corrected in §15.9-Q3) and so there is no empty frame between routes.
    if(!route.hydrated) {
        if(route.comp) {                         // §2.4: static routes have none
            if(!route.ssr) {                     // $__uni_dispose leaves DOM in place
                while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
            }
            try {
                window.$__uni_mount(route.host, route.comp,
                                    window.$__uni_route_props(route), "children");
                route.inst = route.host.$__uni_instance;
            } catch(err) {
                route.hydrated = true;           // mark done: do NOT retry on every switch
                window.$__uni_router_error("route mount failed", routerName + "#" + routeId + " " + err);
                return false;                    // previous route is still visible
            }
        }
        route.hydrated = true;
    }
    if(r.currentRoute) {                         // deactivate: hide, keep alive
        if(r.currentRoute.onDeactivate) r.currentRoute.onDeactivate();
        window.$__uni_route_visible(r.currentRoute, false);
    }
    window.$__uni_route_visible(route, true);    // show
    r.currentRoute = route;
    if(r.$current.value !== routeId) r.$current.value = routeId;    // $_us bails on ===
    if(r.$url && r.$url.value !== url) r.$url.value = url;           // D-7.8
    if(route.onActivate) {
        try { route.onActivate(url) }
        catch(err) { window.$__uni_router_error("onActivate failed", routerName + "#" + routeId + " " + err) }
    }
    return true;
});

// Merges params over the immutable compile-time props at each mount (§6.4).
window.$__uni_route_props = ((route) => {
    if(!route.params) return route.baseProps;
    var out = Object.create(null);
    for(var k in route.baseProps) out[k] = route.baseProps[k];
    for(var k in route.params)    out[k] = route.params[k];   // values pre-escaped (§13.3)
    return out;
});

// The only visibility mutator, and it is total: never throws, never queries the DOM.
window.$__uni_route_visible = ((route, visible) => {
    route.visible = visible;
    if(!route.el) {
        window.$__uni_router_error("route wrapper element missing", route.key);
        return;                                  // D-7.7: state stays consistent
    }
    route.el.setAttribute("data-uni-route-active", visible ? "true" : "false");
});
```

> **This block is an excerpt, not the emitted source.** The normative, complete
> runtime is §15.2; it additionally contains the decode/query/title helpers, the
> `rawUrl` handling, the remote two-step and the URL layer, and it is the copy that
> gets emitted. Any change lands in §15.2 first.

**Five corrections to the earlier draft of this function, each verified against
`lang/libs/page/src/page.ch`:**

1. **Errors do not throw.** `window.$__uni_error` (page.ch:469) is
   `((message, details, cause) => { … throw err; })`. An unknown route id or a
   cancelled guard is a *recoverable* condition, so it uses a router-owned
   `$__uni_router_error` that logs and returns — the same discipline
   `$__uni_dispatch` uses for a missing mount target. Writing `$__uni_error` here
   would abort the caller (event handler, hydration flush) and, because
   `activateRoute` is called from `Link` click handlers, propagate out of the
   click — exactly the crash the design forbids.
2. **Mount in `"children"` mode, not `"root"`.** `$__uni_mount(host, comp,
   props, "root")` hydrates *starting at `host`* and adopts the host element as
   the component's own root element — it applies the component's root props to
   it and, when the SSR tag does not match the vnode tag,
   `$__uni_hydrate_node` **replaces the host element**
   (`e.parentNode.replaceChild(fresh, e)`, page.ch:2127). If the wrapper were the
   mount host, a route whose root is not a `<div>` (or any tag mismatch) would
   destroy the wrapper — losing `data-uni-route`/`data-uni-route-active`/
   `.chx-route` — and the hide rule would stop matching (a route stuck visible,
   or never hideable). `"root"` mode is correct only when the host *is* the
   component's own SSR'd root element (the nested `__uni_uc` case). Routes mount
   exactly like a normal `#html`-embedded component: the `[data-chx-i]` boundary
   span inside the wrapper is the host, `"children"` mode, the span untouched.
3. **Disposal does not remove DOM.** `$__uni_dispose` (page.ch:2289) tears down
   effects, signals and portals but leaves the component's DOM in the document.
   A fresh (non-`ssr`) mount therefore **must clear the host's children first**,
   otherwise `$__uni_hydrate_children` adopts the previous route's stale nodes as
   if they were the new render.
4. **Hydrate before hiding the current route.** The first draft hid the outgoing
   route, then mounted the incoming one. That makes every mount failure (a
   throwing component body, a missing host) blank the page: the old route is
   already hidden and the new one never appeared. Mounting first is strictly
   better — a failure returns early with the previous route still visible, and
   there is no empty frame between routes (see the `try/catch` above and
   §15.9-Q3).
5. **The record's serialized props are never mutated.** `baseProps` is the
   compile-time props object; `params` holds the current URL params; `props` is
   *derived* by `$__uni_route_props` at mount time. The earlier draft overwrote
   `route.props` with the merged params, which (a) loses the original props on
   the second param change and (b) makes the merge order depend on navigation
   history. Deriving per mount is the only version that is stable under
   `1 → 2 → 1` navigation.

`r.$current.value = routeId` is the reactive write from §5.4, and
`r.$url.value = url` is the param-route companion signal (D-7.8). Both are
guarded by an explicit `!==` compare before the write: the `$_us` setter bails
out when the value is unchanged (page.ch:588), so re-activating the same route
notifies neither id-keyed nor url-keyed subscribers. Two signals instead of one
is deliberate — an id-keyed nav link must **not** re-render when only the param
changes (`/p/1 → /p/2`), while a URL-keyed one must; a single token signal cannot
express both without making every param change re-render every link.

`route_visible` toggles the wrapper's `data-uni-route-active` attribute between
`"true"` and `"false"`; pageCss carries one rule, appended once:

```css
.chx-route[data-uni-route-active="false"] { display: none !important; }
```

(The `.chx-route` class is part of the selector, per §12.6 — a bare attribute
selector is beaten by any user `element`+`!important` rule, and the wrapper always
carries the router-owned class. §2.3/template 4 emit it.)

**Why an attribute + CSS rule and not `el.style.display = "none"` (pre-mortem,
§12.3):** the runtime's reactive `style` *object* support writes `cssText` — a
full rewrite of the inline style. A route root with any reactive style binding
would wipe an inline `display:none` the moment its style effect re-runs while
deactivated, silently revealing a hidden route. The `!important` attribute rule
survives `cssText` rewrites, costs one attribute write, and is FOUC-safe (the
rule lives in `<head>` CSS before any markup renders). The ideation's warning
against making `display:none` the *abstraction* still holds: the public contract
is `activate`/`deactivate` (§5.3 hooks); the mechanism behind `route_visible` can
later change (detachment, `content-visibility`) without touching user code.

**Cost profile:** first activation of a route = one mount (normal hydration cost).
Every later switch = two attribute writes, one identity compare, and one signal
write that bails out when the id is unchanged. No DOM queries, no re-render, no
effect re-runs. (URL routes compare the resolved `url` string instead of the
identity alone — a short path compare, §6.4.)

**Re-entrancy rule (must be defined before implementing, §12.4/D-7.1):** hooks
(`onActivate`/`onDeactivate`) run *after* `r.currentRoute` is set, and a call to
`activateRoute` from inside a hook does not run inline — it is pushed onto a FIFO
(`$__uni_router_queue`) that is drained after the in-flight activation completes.
Without this, an `onDeactivate` that navigates would interleave two activations
over the same `r.currentRoute` field and corrupt the visibility pairing (a route
left visible with `currentRoute` pointing elsewhere). Both the queue and the
`busy` flag are initialized next to the registry (§15.2) — never lazily inside
the activate call, because a hook can fire during the very first activation. Note that the hooks in the code above
run while the transition is finishing, so the guards are: check `busy` on entry,
push if set; drain in a loop at the end. A few lines, decided here so it is not
discovered as a heisenbug.

### 2.2 Route activation stubs

Each route emits a two-line stub into the page JS (component-definitions section):

```js
// server renders:
// <div class="chx-route" id="r17" data-uni-route="main-router#projects"
//      data-uni-route-active="false" tabindex="-1">
//   <span data-chx-i id="u17">…SSR of <Projects/>…</span>
// </div>
window.$__uni_route_register("main-router", "projects", {
    key: "main-router#projects", id: "projects", comp: Projects, baseProps: {},
    wrapperId: "r17", hostId: "u17",                     // both source-location ids
    remote: false, isUrl: false, fetchUrl: null,
    noscroll: false, title: null,
    beforeActivate: null, onActivate: null, onDeactivate: null
});
```

Points that make this correct:

- `host` is the route's **hydration boundary** resolved through the existing
  `$__uni_boundary(id)` mechanism (element id, or comment marker for table
  contexts). No repeated attribute scanning; the id is known at compile time.
  `el` is the wrapper the router owns for hide/show — resolved once at bootstrap
  by its own id (`getElementById(spec.wrapperId)`), **not** by a constructed
  `[data-uni-route="…"]` attribute selector: a URL pattern is user text and can
  contain characters a CSS selector would require escaping (§15.9-Q32). One
  `getElementById` per route, at bootstrap only, never in the activation path.
- The wrapper must contain a **real `[data-chx-i]` boundary span** around the SSR
  body (not bare markup) so the host is an element with no leading-text hazard,
  and so the route mount is byte-for-byte the same hydration path a normal
  `#html`-embedded component takes (§2.1 note 2).
- The stub only runs when the component's client function has been emitted (stubs
  are emitted after component definitions in the segmented JS output, so `Projects`
  is defined; the professionalization plan's segmented sections make this ordering
  deterministic instead of relying on `move_js_range`).
- `baseProps` are the route's compile-time serialized props (§4.3) plus a `null`
  placeholder per pattern param (D-2.7), passed to the component on mount — so a
  route can be `<Projects filter="active" />` *and* read `props.id`. The two are
  merged at mount time; `baseProps` is never mutated.
- **The route body is NOT emitted through `$__uni_dispatch` / the hydration queue.**
  Today `#html { <Comp/> }` emits `window.$__uni_dispatch(...)`
  (`emit_universal_queue`, `lang/libs/html_cbi/src/converter/language/main.ch:417`),
  which pushes onto `$__uni_hydration_queue` and is drained by
  `$__universal_flush()` at the end of the page script. If a route body went
  through that path, *every* route would hydrate at load (defeating `lazy`) and
  the default route would then be mounted a second time by `$__uni_activate`,
  and `$__uni_mount`'s ownership check would dispose the first instance —
  observable as effects running twice in opposite orders. A route body is
  instead registered as a route record and mounted **only** by the router.

### 2.3 SSR side: the wrapper element

The route body is server-rendered inside a wrapper that is hidden by attribute:

- The wrapper carries only router-owned attributes and nothing else: `id="rN"`
  (source-location id, used by registration), `class="chx-route"` (the hide rule's
  hook), `data-uni-route="name#id"`, `data-uni-route-active`, and `tabindex="-1"`
  (focus target). The route's own markup lives inside the boundary span, never on
  the wrapper, so no user/component prop can clobber the router's attributes.
- The wrapper is **not** the mount host; the `[data-chx-i]` span inside it is
  (§2.1 note 2). The wrapper remains in the document for the lifetime of the
  page and is only ever attribute-toggled.
- The default route (§3.4) renders with `data-uni-route-active="true"` (or no
  attribute — visible is the default); other routes render `"false"`. Hiding is
  CSS-rule-driven (§2.1), so no inline style is involved and reactive style
  bindings on the route root cannot unhide a deactivated route.
- SSR renders **every** route body eagerly (subject to mode, §5.1). This is what
  makes navigation instant. The ideation's cost warning (500 KB of Admin HTML for a
  Dashboard visitor) is answered by lazy routes (§5.1) and the URL layer (§6), not by
  making eager rendering implicit.

### 2.4 Routes with no client function (`comp: null`)

A route body with no client-observable behaviour (no `state`, no event handlers,
no refs, no reactive bindings) has no client function worth mounting. The
converter already classifies each SSR statement as static vs dynamic (the same
classification the §7.5 snapshot cache uses), so such a route registers
`comp: null` and activation is purely `route_visible(route, true)` — no mount, no
instance, no listeners, no JS heap. Hide/show, focus, scroll and the URL layer are
unchanged. The rule is **conservative**: anything the classifier is unsure about
is treated as interactive (a route emits a client function), because the cost of a
spurious mount is small while the cost of a missing one is a dead route. This also
means a fully static app can use the router as a pure controller over zero client
components — the router is useful even for a page that never hydrates anything.

---

## 3. Server parameter store (HtmlPage parameters)

The server-side half of the design: when a request lands, the page must be able to
select which route is the default and read request-scoped values inside components.

### 3.1 `PageParameter` variant

```chemical
// lang/libs/page/src/params.ch   — module `page`, NOT the router library
public variant PageParameter {
    Text(view : std::string_view)
    Object(ptr : *mut void)
}
```

- **`PageParameter` must live in `page`, and this is a hard constraint, not a
  preference.** `HtmlPage` holds the map, and `lang/libs/router` imports `page`
  (D-4.1) — so a `PageParameter` defined in `router` would give the dependency
  cycle `page → router → page`, which the module system does not allow. The router
  library adds only *extension functions* over the type (§3.2, §15.4). An earlier
  draft had this commented as `router/src/params.ch`; that cannot compile.
- `Text` covers strings without forcing an allocation (a `string_view` over the
  request buffer or a literal).
- `Object` is a typed-erased pointer for context passing (the ideation's
  `addParameterGen` / `getParameterGen`).
- Two pointers + discriminant ≈ the ideation's 24 bytes per parameter; stored in an
  `std::unordered_map<std::string_view, PageParameter>` (or `ordered_map` if
  deterministic iteration is needed for tests).

### 3.2 `HtmlPage` additions

Added to `HtmlPage` (the only change to `page` itself — deliberately minimal so the
router stays an extension library):

```chemical
// in HtmlPage
var parameters : std::unordered_map<std::string_view, PageParameter>

public func add_parameter(&mut self, key : std::string_view, value : std::string_view) {
    parameters.insert(key, PageParameter.Text(value))
}

public func add_parameter_object(&mut self, key : std::string_view, ptr : *mut void) {
    parameters.insert(key, PageParameter.Object(ptr))
}

public func get_parameter(&self, key : std::string_view) : std::string_view {
    const p = parameters.get_ptr(key)
    if(p == null) { return std::string_view() }
    if(p is PageParameter.Text) {
        var Text(view) = *p else unreachable
        return view
    }
    return std::string_view()
}

public func has_parameter(&self, key : std::string_view) : bool {
    return parameters.contains(key)          // by value: &key would be a *mut key view
}

// Set by the generated router code when a request URL matched no route and the
// router declared no fallback (§6.1); read by the handler *after* rendering
// (#html) to choose the status code. Server-only; the client has no equivalent.
var route_missing_flag : bool = false

public func mark_route_missing(&mut self) { route_missing_flag = true }
public func route_missing(&self) : bool { return route_missing_flag }
```

Methods are `public` because the router library (a different module) calls them —
internal functions are invisible across packages (AGENTS.md, library gotchas).

Naming note: the ideation writes `addParameter` / `getParameterGen<T>`. The codebase
convention is snake_case (`append_html`, `get_html_size`), so the API is
`add_parameter` / `get_parameter` / `add_parameter_object`. Chemical *does* have
generic functions (`func <T> name(...)` — see `lang/tests/src/generic/basic.ch`),
but the method-call form `page.get_parameter_gen<T>("k")` on an extension function
is unverified, so typed retrieval is a **free generic function** taking the page
explicitly:

```chemical
// lang/libs/router/src/params.ch — importer pays; null on miss/type mismatch
public func <T> get_parameter_object(page : &mut HtmlPage, key : std::string_view) : *mut T {
    const p = page.parameters.get_ptr(key)
    if(p == null) { return null }
    if(p is PageParameter.Object) {
        var Object(ptr) = *p else return null
        return ptr as *mut T
    }
    return null
}
```

Pattern matching on a `*mut PageParameter` from `get_ptr` follows the established
`top is JsonValue.Array` / `var Array(values) = *top` form used throughout
`lang/libs/json/src/handler.ch`.

> **Type-safety contract:** this keeps the ideation's fast `*mut void` path while
> the honest answer to "if user types Something wrong -> memory error" is that the
> cast is unchecked (like C). The type-safe layer is the *macro-generated*
> accessors (§4.4): when a router macro sees `get_request<UserCtx>(page)` it can
> validate the key/type pair at compile time against what was stored. The raw
> pointer path exists for code the macro cannot see; it is documented, not hidden.

**Lifetime contract (must be documented in the library):** keys and `Text` views
must outlive the page render (the ideation's "server time" — they typically point
into the request buffer, which outlives `render()`). `Object` pointers must remain
valid until the page is written. Violating this is the same class as any dangling
view in the codebase; the store intentionally does not copy for performance (the
ideation explicitly counts bytes per parameter).

### 3.3 Request context (the "request-like object")

The ideation wants complex routes to access the request. Rather than inventing a
new request type, the router library defines one:

```chemical
// lang/libs/router/src/request.ch
@make @direct_init
public struct RouteRequest {
    var method : std::string_view       // "GET"
    var path : std::string_view         // "/projects/42"
    var query : std::string_view        // raw query string, "" if none
    var body : std::string_view         // "" unless read by the caller
}

public func (page : &mut HtmlPage) set_request(req : &RouteRequest) {
    page.add_parameter_object("__request", req as *mut void)
}

public func get_request(page : &mut HtmlPage) : *mut RouteRequest {
    return get_parameter_object<RouteRequest>(page, "__request")
}
```

Extension functions with receiver syntax are an established pattern — the
receiver is written before the name and used directly in the body
(`public func (page : &mut HtmlPage) injectDefaultComponentsTheme()` in
`lang/libs/components/src/theme.ch:10`) — so `page.set_request(...)` composes
without polluting `page`. `@make @direct_init` is required so both
`RouteRequest.make(...)` and `RouteRequest{...}` work (the `@make`-without-
`@direct_init` interaction from AGENTS.md).

### 3.4 Default-route selection

`page.add_parameter("main-router", "dashboard")` tells the router which route id is
active for **this page instance**. SSR then:

1. Renders all routes (eager mode), wrapper hidden.
2. Renders the selected route's wrapper **without** the hidden attribute (the
   declared `route default` applies when no parameter is present, §4.5).
3. Emits its activation stub with `hydrated: false`; the client hydrates it through
   the **router's** path, not the hydration queue (§2.2 bullet 5: route bodies are
   deliberately never pushed onto `$__uni_hydration_queue`). Keeping one client path
   is what makes the initial route behave exactly like a click — uniform code path,
   no SSR-side "already active" branch.
4. Emits `window.$__uni_activate("main-router", "<id>")` at the end of `pageJsEnd`
   so the initial route activates (and hydrates) exactly like a click does.
   **Ordering invariant:** `defaultUniversalSetup` appends
   `window.$__universal_flush();` to `pageJsEnd` at setup time, and the final page
   script is `getFinalizedPageJs()` = `pageJs` + `pageJsEnd` (page.ch:2503). So
   the emission order is: component definitions and route stubs (`pageJs`) →
   `$__universal_flush()` (all non-route components hydrate) → the initial
   `$__uni_activate` appended after it. Appending the activation to `pageJs`
   instead would run it *before* the flush and before every other component's
   hydration — a real ordering bug, so the router emits into `pageJsEnd`
   explicitly.

If the parameter is absent or names an unknown route: render all routes hidden and
emit no activation — the page is inert but valid (the compile-time validation in
§4.4 makes this case impossible in practice for declared routers; the runtime check
covers dynamically composed pages).

---

## 4. Declarative router syntax (the `router` macro)

### 4.1 Surface syntax

```chemical
#universal App() {
    router "main-router" {
        route default #"dashboard" {
            <Dashboard />
        }
        route #"projects" {
            <Projects />
        }
        route * {
            <NotFound />
        }
    }
}
```

- `router "name" { ... }` — declaration inside a `#universal` body. The name is a
  string literal; two routers on one page must not share a name (compile diagnostic).
- `route #"id" { ... }` — the `#` marks an **id route** (no URL semantics, the
  ideation's exact token). The id is a string literal, unique within the router
  (compile diagnostic otherwise).
- `route * { ... }` — the fallback route: activates when the requested default is
  unknown (server-side; the client fallback only applies to the URL layer, §6.2).
  Optional; at most one per router (compile diagnostic otherwise).
- `route default #"id"` — the declared default route, used when no server
  parameter selects one (§3.4, §4.5). Optional; at most one per router.
- `route "/projects/{id}" { ... }` — URL route with params (§6). URL and id routes
  can coexist; a URL route *also* gets its id (its normalized path) so
  `activateRoute` works uniformly.

### 4.2 Where it hooks into the parser

`parseStatement` in `lang/libs/universal_parser/src/parser/parser_stmt.ch` is a
token-dispatch chain (`var/const/let/state`, `if`, `return`, ...). Add:

- `Router` / `Route` / `Star` keywords to `lang/libs/js_syntax/src/TokenType.ch`
  (append at the **end** of the enum — the CBI enum-sync rule from AGENTS.md; any
  insertion in the middle shifts `lang/libs/compiler/src/ChemicalTokenType.ch`
  values and SIGSEGVs every plugin).
- `JsRouterDecl { name, routes }` / `JsRouteDecl { id_or_path, is_fallback, body }`
  nodes to `JsNodeKind` (again append-only) and the AST structs in `js_syntax`.
- Parse them in `parseStatement`'s dispatch chain; the route body is parsed with the
  existing statement parser, so anything legal in a component body is legal in a
  route body. A JSX element is required at the top level of the body (single root,
  same rule as component return), enforced at conversion time with a diagnostic.

Error handling follows the parser's existing recovery (`parser.error(...)` +
`return null`), so a malformed router declaration produces a diagnostic, not a
crash. Every construct the converter cannot yet handle must be a **diagnostic**
(`ASTDiagnoser` is threaded through the converter) — never a silent drop (the
professionalization plan's Phase 0 rule; `emit_ssr_body_statements` silently
dropping statements is the cautionary example).

### 4.3 What the macro generates

For each route, three artifacts (all existing machinery, no new backend work):

1. **SSR**: the route body is rendered inside the wrapper div with
   `data-uni-route="name#id"` + `data-uni-route-active` (§2.3) — **no inline style**
   (hiding is attribute-CSS, §2.1). Concretely the generated server function emits
   `page.append_html('<div class="chx-route" data-uni-route="main-router#projects" data-uni-route-active="false"><span data-chx-i id="u17">')`,
   then the body's SSR, then `</span></div>`. The default route (§3.4) emits
   `data-uni-route-active="true"`.
2. **Client JS**: the route body is compiled like a nested universal child — an
   anonymous client function + `$_uc_c` style function-reference vnode — and the
   route's activation stub registers `{ el, host, comp, props }` in
   `$__uni_routers["name"].routes[id]` (§2.2). It is **emitted as a registration,
   not a dispatch**: the converter must not route a route body through
   `emit_universal_queue`, or `$__universal_flush()` would hydrate every route at
   load (§2.2 bullet 5). Concretely this is a new emission path beside
   `emit_universal_queue` in
   `lang/libs/html_cbi/src/converter/language/main.ch` — the boundary id is still
   allocated through the existing id machinery, only the queue push is replaced by
   the record assignment.
3. **Props**: attributes on the route's JSX root become the route's props, same as
   `#html { <Comp prop={value} /> }` does today (prop serialization rules apply,
   including the single-quote/backslash escaping contract from AGENTS.md;
   unsupported prop types are a compile diagnostic, never silently serialized).

Nested routers (a router inside a route body) are supported naturally: the inner
router is part of the outer route's SSR and client function, with its own name and
registry entry. Names must be unique per page (diagnostic if not).

### 4.4 Compile-time validation (the "error out cleanly" guarantees)

All of these are diagnostics with source locations, produced during symres/conversion
of the macro — the compiler never crashes, and bad code never reaches the browser:

| User error | Diagnostic |
|---|---|
| `route` outside a `router` | "'route' declaration is only valid inside a router block" |
| `router` at top level of `#html` (not inside a component) | "router must be declared inside a #universal component" |
| duplicate route id in one router | "route '#projects' is declared twice in router \"main-router\"" |
| duplicate router name on a page | "router \"main-router\" is declared twice" |
| route body with no / multiple JSX roots | "route body must render exactly one root element" |
| `route *` declared twice, or after it another route | "fallback route must be the last route" |
| `activateRoute("typo")` in a Chemical expression | "no route '#typo' in router \"main-router\"" (see below) |
| route body referencing `$` runtime globals directly | "route bodies cannot call runtime internals; use component APIs" |

The `activateRoute` check works because the macro knows the set of legal ids for
each router name in the same module; cross-module routers validate the ids that the
importing module statically references. Dynamically composed ids (a variable) skip
compile validation and are covered by the runtime `unknown route` error path
(§2.1) — logged, non-fatal, page keeps working.

The runtime error path is also clean: recoverable failures go through
`$__uni_router_error` (log-only, non-throwing — §2.1 note 1) and return `false`;
the previous route stays active. No throw, no broken page. `$__uni_error` is
intentionally *not* used here: it throws by design, and a router fault must never
propagate out of a click handler or the hydration flush.

### 4.5 Syntax ergonomics review (assessed; amendments adopted)

Assessing the surface for easeness — the goal is that the 90% case is one
declaration and one call:

| Aspect | Assessment |
|---|---|
| `router "name" { route #"id" { <X /> } }` | Good — one block, JSX bodies are already-familiar. |
| `route *` fallback | Good — terse, matches user intuition. |
| `onActivate`/`onDeactivate` next to the JSX root | Good — visible where the route is declared. |
| `router("main-router").activateRoute("projects")` | Verbose for the single-router page (the common case). Amended: `router()` with no name resolves to the page's **only** declared router — compile diagnostic if zero or multiple routers are declared. `.go("id")` as an alias for `.activateRoute(id)` is deliberately *not* added; one verb keeps greppability. |
| `route preload #"admin"` (mode before id) | Reads oddly — the mode is a property *of the route*. Amended to **mode after id**: `route #"admin" preload { ... }`. Parser is unaffected (after `route`, parse `#id`/path/`*`/`default`, then optional mode keyword, then body). §5.1 examples updated to the canonical form. |
| Default route only via server parameter | A page with no parameter renders inert — surprising for the simple case. Amended: `route default #"dashboard"` declares the fallback default (§3.4). The request parameter still **overrides** it; the declared default only applies when no parameter is present. |

Not adopted (considered, rejected): `router.activateRoute(Projects)`
(component-as-id — rejected in §5.2 for good reason), implicit single-route
routers (`router { ... }` without a name — hurts §4.4's duplicate-name
diagnostics and cross-module validation), URL-template strings in `activateRoute`
("#projects" vs "/projects" confusion — ids and paths stay visually distinct:
`#` prefix = id, `/` prefix = path).

---

## 5. Modes, preload, and control API

### 5.1 Route modes

```chemical
router "main-router" {
    route #"admin" preload { <Admin /> }     // SSR + hydrate at page load
    route #"reports" lazy { <Reports /> }    // SSR only; hydrate on first activate
}
```

- Default (no keyword) = `lazy` for hidden routes; the *default route* (§3.4) is
  always hydrated at load via the initial `activate` call.
- `preload`: the activation stub for that route runs at bootstrap (hydrated while
  hidden). Costs its effects immediately — that is the point; document it.
- `lazy` is what makes the "500 KB Admin" case acceptable: the HTML is in the
  response (instant switch), but the JS effects only run when needed.
- URL-layer routes (§6) additionally get **fetch-on-demand** (no SSR HTML at all for
  unvisited lazy URL routes when the page is served per-request).
- A third mode, `remote`, exists only on URL routes (§6.7): no SSR HTML ships at
  all; the route's markup is fetched on first activation (or on hover prefetch).
  `route remote "/reports" { ... }`.

### 5.2 `router` control API (component-visible)

Inside a component body (or from a Chemical-embedded expression in a route body),
the compiler exposes a typed handle:

```chemical
#universal Sidebar(props) {
    // single-router page: router() with no name resolves it (§4.5)
    <button onClick={() => { router().activateRoute("projects") }}>
        Projects
    </button>
}
```

`router("name")` is emitted as a call to the runtime accessor
`window.$__uni_router("name")` (D-3.1) — not an inline `$__uni_routers[...]` index,
so a router that is absent from the page yields the null-object instead of a
`TypeError` — with a compile diagnostic when a **literal** name is not declared on
the page (§4.4). The handle's methods:

| Method | Behavior |
|---|---|
| `activateRoute(id)` | §2.1; idempotent, returns bool |
| `replaceRoute(id)` | like `activateRoute`, but history `replaceState` (redirects, login flows) |
| `deactivate()` | hide current, keep state; `current = null` |
| `isActive(id)` | bool; also available **reactively** (§5.4) for JSX bindings |
| `preload(id)` | hydrate without showing (§5.1 `preload` keyword does this at load) |
| `release(id)` | dispose an **inactive** route's instance and drop its DOM, re-arming it for a fresh mount (§13.3.1) |
| `current()` | active id or `null` |
| `currentUrl()` | active normalized path or `null` (URL routes) |
| `currentRoute` | the active route record — a **field**, not a method (D-7.9) |
| `replaceRouteByUrl(path)` | `activateRouteByUrl` with `replaceState` (§6.7 redirects) |
| `normPath(href)` | the router's own path normalization, for comparing a `href` with `$url.value` in JSX (§6.6) |
| `buildPath(id, params)` | URL routes: reverse-map the emitted match table to a path string (§6.6) |
| `query()` | URL routes: parsed query params of the current URL (§6.5) |
| `setQuery(obj)` | URL routes: replace the query string, no navigation, no scroll reset (§6.5) |

The ideation's `router.activateRoute(Projects)` (component-as-id) is intentionally
**not** the surface: components are not unique per route (two routes may render the
same component), and ids are what the server parameter store uses (§3.4). The
compiler-interned id still gives the no-string-lookup property at runtime.

### 5.3 Lifecycle hooks

Route bodies can declare hooks next to the JSX root (parsed like `state`, optional):

```chemical
route #"projects" {
    onActivate(() => { refresh() })
    onDeactivate(() => { saveScroll() })
    <Projects />
}
```

Compiled into the route's client function: `onActivate` registers a callback
invoked by `route_visible(route, true)`; `onDeactivate` by the deactivate path.
They are effect-like (cleanup-safe, error-isolated: a throwing hook is caught and
reported via `$__uni_router_error`, §2.1, so one bad hook cannot abort the
activation) but **not** effects — they fire on activation transitions, not on
state changes. First activation also runs the component's
`useEffect`s once (hydration); later activations only fire `onActivate`.

A fourth hook, `onBeforeActivate`, is the **guard** hook:

```chemical
route #"admin" {
    onBeforeActivate(() => { return session().isAdmin })   // false cancels the nav
    <Admin />
}
```

It runs *before any DOM change* (before the current route is hidden), so a `false`
return simply keeps the current route active and logs via `$__uni_router_error`. This is
the auth/"unsaved changes" mechanism, and it is why the deactivate-then-activate
ordering in §2.1 matters: checks precede every mutation.

### 5.4 Reactive router state (active links, route-aware UI)

A gap in the first draft: `isActive(id)` as a plain method cannot drive JSX — a
nav item that highlights when its route is active needs the router state to be a
**signal**. The router therefore keeps its current route in the existing runtime
signal primitive:

```js
// inside the router record created at bootstrap (§15.2)
r.$current = window.$_us(null);      // active route **id**  (id routes, active links)
r.$url     = window.$_us(null);      // active resolved **url** (URL routes, param-aware links)
```

`activateRoute`/`deactivate` set `r.$current.value` (and `r.$url.value` for URL
routes — D-7.8); `r.$current` is nothing new — it is the same `$_us` machinery
components already use, so `$_ucs` computed wrappers and JSX reactivity work on it
unchanged. **Two signals, not one:** an id-keyed nav link must not re-render when
only a param changes (`/p/1 → /p/2` keeps the id), while a URL-keyed link must.
A single token signal cannot express both without re-rendering every link on every
param change, so the split is both cheaper and more correct. `deactivate()`
writes `null` to both.

```chemical
#universal NavLink(props) {
    const r = router("main-router")
    // id routes read $current; URL routes read $url (param-aware, D-7.8)
    const active = props.id
        ? r.$current.value == props.id
        : r.$url.value == r.normPath(props.href)   // same rule the matcher uses
    <a class={active ? "active" : ""} aria-current={active ? "page" : null} ... />
}
```

Note the read is of the **signal** (`r.$current.value`), never of
`r.currentRoute` — a plain field read is not a subscription and would make the
link stale after the first navigation. The macro's compile-time validation (§4.4)
checks that any `props` used here is declared.

This costs two signal objects per router and zero extra runtime concepts, and it is
what makes `Link` (§6.6) get `aria-current` for free.

---

## 6. URL layer (built on the primitive, never inside it)

The ideation is explicit: core = ids; URLs are a layer. This section is the layer,
shipped in the same library but kept strictly separate from §2–§5.

### 6.1 Server: URL → route selection

The first draft had a load-bearing ordering bug: it had the user call
`page.activate_route_by_url("main", req.path)` **before** `#html { <App/> }`,
assuming the page could match `req.path` against a pattern table for router
`"main"`. But the router (and therefore its patterns) is *declared inside*
`App`, whose generated server function has not run yet at that point — the page
owns no pattern table, and inventing a cross-module registry for one would add a
fragile module-init ordering contract for no benefit. The fix splits the work
along the natural ownership boundary:

- **The app hands the page the raw request URL.** Matching is deferred.
- **The generated router server function — which *does* own the compile-time
  patterns — performs the match while it renders** and writes the result back to
  the page.

```chemical
// lang/libs/router/src/url.ch — extension receiver form (no explicit self param,
// matching `func (page : &mut HtmlPage) name(...)` in components/src/theme.ch:10)
public func (page : &mut HtmlPage) set_route_url(
    path : std::string_view, base : std::string_view = ""
) {
    page.add_parameter("__route_url", path)      // raw; matched at render time
    page.add_parameter("__route_base", base)
}
```

`set_route_url` may be called before the router is compiled — it only stores two
strings; nothing reads them until a generated router function renders. The
generated function calls a **pure library matcher**:

```chemical
// lang/libs/router/src/match.ch — pure, no page state, unit-testable
public variant RouteParam { Param(name : std::string_view, value : std::string_view) }

public struct RouteMatch {
    var matched : bool
    var id : std::string_view                 // matched route id ("projects")
    var params : vector<RouteParam>           // page-owned copies
    var is_fallback : bool                    // matched `route *`
}

public func match_route(
    routes : &std::vector<RoutePattern>, path : std::string_view, base : std::string_view
) : RouteMatch
```

The generated router SSR function (emitted by the macro, §4.3) then does:

```chemical
// pseudo-generated, inside the component that declares the router
var m = match_route(compiled_patterns, page.get_parameter("__route_url"),
                    page.get_parameter("__route_base"))
if(m.matched) {
    page.add_parameter("main-router", m.id)               // §3.4 selection
    for(var i : uint = 0; i < m.params.size(); i++) {
        var Param(name, value) = m.params.get(i) else unreachable
        page.add_parameter(name, value)
    }
} else if(!m.is_fallback) {
    page.mark_route_missing()
}
```

- **Names.** `activate_route_by_url` remains the **client** entry
  (`activateRouteByUrl`, §6.2/§6.6); the server entry point is `set_route_url`.
  There is deliberately no server-side `activate_route_by_url`, because the
  router name in user code cannot be resolved to patterns at that point in the
  program.
- `base` strips a mount prefix first (apps served under `/app/`); without it,
  deep links into sub-path deployments would never match.
- Path normalization before matching: strip trailing slash (except root), decode
  percent-encoding, reject `.`/`..` segments as non-matching (matching is
  segment-wise against declared patterns, so traversal cannot match; `..` is
  never resolved against the filesystem — the router has no filesystem concept).
  **Decoder source (corrected — see §14.4):** do **not** reuse
  `http::url_decode`/`http::parse_query`. They live in the `http` module, and
  `lang/libs/http/chemical.mod` imports `net`, `tls`, `async`, `mime` and `json`
  — importing `http` from the router library would drag the entire networking
  stack (and a TLS dependency) into every app that routes, breaking §0.5's
  "the importer pays the cost" rule. The net-free home for percent coding is the
  `encoding` module (`lang/libs/encoding/src/url.ch`: `url_encode`,
  `url_encode_query`, `url_decode`), whose module imports only `std` and
  `crypto`. The router library therefore `import encoding` and implements the
  ~20-line query splitter itself. This also supplies `buildPath`'s
  percent-encoding (§6.6) from the same source.
- The router library ships components (`Link`, `NavLink`, `Outlet` — §6.3), so
  it is a CBI-plugin library like `components`; it imports `page` and the html
  stack, never `net`.
- Patterns arrive as `RoutePattern` values built at **compile time** by the macro
  and passed into the generated function (no runtime table assembly); a linear
  scan is fine for realistic route counts, `unordered_map` on static prefix
  segments if it ever matters (§7.5 budget: R=50 ≤ 10µs).
- On match: the generated function calls `page.add_parameter(router_name, id)`
  (§3.4) and stores `{id}` params as `Text` parameters keyed by their pattern
  name, so the route's serialized props carry them (§6.4) — a route body reads
  `props.id`, never a server-only helper.
- On miss: the `route *` fallback route if declared; otherwise
  `mark_route_missing()` and the app decides (404 — §6.3.1). The *generated*
  code picks the fallback, because it is the only party that knows the declared
  patterns.
- **Ordering requirement, explicit:** the URL must be stored on the page before
  the component that declares the router renders (it always is in §6.3.1).
  Setting a path after `<App/>` has rendered has no effect — the routing decision
  is part of rendering, not a post-pass.
- **Status/SEO:** `mark_route_missing()` sets a page flag the app reads after
  rendering (`page.route_missing()`), so the 404 status is applied to a fully
  rendered fallback body — which is also more correct than the first draft's
  "404 before rendering", where a `route *` body could never be the 404 page.

Server integration with `net_http` stays in user code (or a thin
`router::serve_page(srv, ...)` helper), because the pipeline has no streaming
request/response server inside the SSR architecture (the professionalization plan
notes this). The router library must not depend on `net`.

### 6.2 Client: history integration (opt-in)

```js
// emitted only when the page uses URL routes
window.$__uni_sync_url("main-router");
```

- `activateRoute` on a URL route pushes `history.pushState` (popstate → activate).
- `activateRoute` on an id route is URL-less — ids stay the universal primitive.
- Unknown URL on the client → activates the fallback route if the router has one.
- No redirect loops: activation triggered by popstate never pushes.
- **Coalescing:** rapid navigations (double-click, a link click racing a
  popstate) do not stack. `activateRoute` is synchronous, and a navigation
  requested from inside a hook is queued (§2.1), so the sequence is applied in
  order and the last request wins through the normal `r.currentRoute` comparison.
  Because the core path is synchronous there is no interleaving to guard against
  beyond the hook queue — the *only* asynchronous step is a `remote` fragment
  fetch, which is guarded by the in-flight flag (§12.4).
- **A guard on popstate must re-sync the URL.** `onBeforeActivate` returning
  `false` cancels the route change, but the browser has *already* changed the URL
  and fired popstate — the URL now disagrees with the visible route. The router
  must therefore `history.pushState`/`replaceState` the previous URL back
  (best-effort, inside the same try/catch as everything else) or, on an opaque
  origin, restore its in-memory URL string. Without this a denied back-navigation
  leaves a bookmarkable URL that renders a different route on reload. This is a
  real interaction between §5.3 guards and §6.2 history and is a test row (§8.2).
- **No automatic browser scroll restoration** (`history.scrollRestoration =
  "manual"` is set when the URL layer installs): the router owns scroll (§6.7)
  instead of having the browser fight it. Back/forward therefore restores the
  route's remembered offset (§6.7), not the browser's guess.
- **Opaque-origin fallback (§12.5):** `pushState` throws `SecurityError` on data
  URLs / opaque origins — exactly how a WebView test harness (and some embedded
  WebViews) load a page. `$__uni_sync_url` therefore wraps every history call in
  try/catch and falls back to **in-memory URL tracking** (the router keeps its own
  path string and the query signal; back/forward are unavailable). The page works
  identically otherwise; the only loss is browser-history integration. This is not
  a corner case — it is the default condition inside `#universal_test`, so the
  fallback is a tested code path, not a theoretical one.

### 6.3 Link component

```chemical
#universal Link(props) {
    <a href={props.href}
       onClick={(e) => {
           // Only plain left-clicks on an internal path are intercepted.
           // Modified clicks (ctrl/cmd/shift/alt/middle) and explicit targets
           // belong to the browser: new tab / new window / download.
           if(!window.$__uni_should_intercept(e, props.href)) { return }
           e.preventDefault()
           router(props.router || "main-router").activateRouteByUrl(props.href)
       }}>
        {props.children}
    </a>
}

Two conventions this depends on, both verified in the components library:

- the event object is **`e`** and handlers call `e.preventDefault()` / `e.stopPropagation()`
  (never a bare `preventDefault()` — that is undefined in the emitted JS);
- runtime helpers are reached through `window.$__uni_*` from JSX
  (`window.$__uni_floating(...)` in `Surface.ch`, `window.$__uni_inert_scan()` in
  `Sheet.ch`), which is why the predicate is `$__uni_should_intercept` and not a
  library function called by name.
```

`$__uni_should_intercept(e, href)` is one runtime predicate (`button === 0 &&
!metaKey && !ctrlKey && !shiftKey && !altKey && no target/download attr && href is a
same-origin path`), emitted with the CONTROL group (§15.2) so `<Link>`, `<NavLink>`
and user markup share it. An `href` that is absolute to another origin,
protocol-relative, `mailto:`/`tel:`, a `#fragment`, or marked `download` is **never**
intercepted — it is a normal navigation. (A `Link` that unconditionally calls
`e.preventDefault()` silently breaks ctrl-click-to-new-tab and middle-click, both of
which are load-bearing browser behaviors users notice immediately.)

Ships in the **router library** (`lang/libs/router/src/Link.ch`), together with
`NavLink` (active-styling variant) and — Phase 6 — `<Outlet />`. One
`import router` therefore provides the full routing surface: the macro, the
control API, the extension functions, and the routing components. These
components use only the public control API, so they compose with custom routers.

### 6.3.1 Complete server setup (all of it — there is nothing else)

The server side is a few lines inside the handler the user already writes. Full
example with the real `http` API (from `lang/compiled/docs/src/stdlib/net_http.md`):

```chemical
import router
import http                // provides http::Request / http::ResponseWriter

public func handle_request(req : &http::Request, res : &mut http::ResponseWriter) {
    var page = HtmlPage()
    page.defaultUniversalSetup()                       // existing universal runtime

    // 1. Hand the page the raw request URL. Matching happens *during* render,
    //    inside the generated router code that owns the patterns (§6.1).
    page.set_route_url(req.path, "/app")               // base optional; the generated
                                                       // router writes it into the
                                                       // client table itself (§15.9-Q42)

    // 2. request context (only if route bodies need it)
    page.set_request(&RouteRequest.make(req.method, req.path, req.query, ""))

    #html { <App /> }                                  // router declared inside App

    // 3. status is decided *after* rendering, so the fallback route's markup
    //    is the 404 body (the router sets the flag; the app chooses the code).
    if(page.route_missing()) { res.status = 404u }
    res.write_string(page.toString())                  // inlines CSS/JS in one doc
}
```

That is the whole integration:

| Step | Required? | Cost |
|---|---|---|
| `page.set_route_url(path, base)` | yes for URL routes | two store inserts |
| `page.set_request(...)` | only if route bodies read the request | one store insert |
| `page.add_parameter("main", "id")` | replaces step 1 for id-only routing | one store insert |
| `page.toString()` | per-request response | inlines pageCss + pageJs (one alloc) |
| `writeToDirectory` + `htmlPageToString(name)` | static-asset export | build-time; 4 files per page |

The macro and client runtime need **no** server setup: declaring `router` inside
a component is enough — the compiler emits the registry, wrappers, and stubs into
the page. `set_route_url` merely tells that machinery what URL the request wants.

> **Two output modes, easy to confuse.** `page.toString()` returns a
> self-contained document (CSS and JS inlined as `<style>`/`<script>`) — correct
> for a per-request handler that writes an HTTP body. `page.htmlPageToString(name)`
> instead emits `<link href="name.css">` / `<script src="name.js">` and **requires
> a `name` argument** (there is no no-arg overload) plus the four files written by
> `writeToDirectory(dir, name)` (`name.html`, `name.css`, `name_head.js`,
> `name.js`) — a static-export deployment. The earlier draft wrote
> `res.write_string(page.htmlPageToString())`: that call does not exist, and if it
> did the browser would 404 every asset. Pick the mode deliberately.

> **Methods.** Route selection is a GET concern. For `HEAD`/`OPTIONS`/non-GET
> requests, the app decides — the router has no opinion; writing a body for a
> `HEAD` request is a server bug, not a router bug. `set_route_url` is
> idempotent-per-page and takes the *request* path only (not a query string —
> query is a separate parameter store, §6.5). A common shape is
> `if(req.method is "GET") { page.set_route_url(req.path) }`, leaving other
> methods to a dedicated handler.

> Static-export deployment (§6.8): a static host serves the pre-built `.html`
> for the URL's page; the emitted client match table selects the route in the
> browser. Server router code only exists in per-request deployments.

### 6.4 Params reach the component on both sides (client-side matching)

The first draft had a hole: server-side matching stores `{id}` params, but after a
**client-side** navigation (`Link` click, popstate) nothing re-runs server code —
how does `<Project />` get `props.id`? The match table built at compile time is
therefore also emitted to the page JS (static segment data, ~1 line per route):

```js
// emitted once per page that uses URL routes, IN PRECEDENCE ORDER (D-6.9)
window.$__uni_routers["main-router"].table = {
  base: "/app",                                   // "" when not mounted under a prefix
  routes: [
    { pattern: ["projects", "{id}"], id: "projects" },
    { pattern: ["settings"],         id: "settings" },
    { fallback: true, id: "not-found" }           // from `route *`
  ]
};
```

> The table hangs off the registry record (`r.table`), not a separate
> `$__uni_route_tables` map — there is exactly one router registry (§15.2, §15.3
> template 3), and two places to look for a router's patterns is exactly how server
> and client drift. An earlier draft emitted an array under its own global; that
> shape is **gone**.

Client matching (`$__uni_match_url(name, path)`, §15.2) walks segments; on match it
extracts `{param}` values into `route.params`, which `$__uni_route_props` merges
over the route's immutable `baseProps` at mount. So the same declaration serves:

- **Server deep link:** the generated router function matches (§6.1), stores
  params (§3.4) and renders the route with `props.id` resolved from the page
  parameter (§15.4, D-2.7).
- **Client nav:** `activateRouteByUrl(path)` matches client-side, passes the params
  as the activation's `params` argument, and the mount merges them — `baseProps` is
  never mutated (§15.9-Q1).
- **popstate:** same client path, no server round-trip.

This keeps the "everything the user did not write never happens" rule: a param
name used in `props.id` is validated at compile time against the route's declared
pattern (the macro knows both), so a typo like `props.iid` is a diagnostic, not an
undefined prop at runtime.

**Param-change semantics (pre-mortem, §12.1 — must be implemented as written):**
the record identity for a param route is the *pattern* id (`"projects"`), but its
`url` + `rawUrl` + `params` fields hold the **resolved** values. Activation therefore
runs a three-way check, not the §2.1 two-way one:

1. `r.currentRoute === route && r.currentRoute.url === url` → true no-op (O(1)).
2. `r.currentRoute === route && r.currentRoute.url !== url` → **param change**: dispose the
   record's instance (`$__uni_dispose`), set `hydrated = false`, update
   `props`/`url`, then run the normal activate path.
3. otherwise → the normal §2.1 path.

Why remount on param change: the record's `ssr` markup was rendered for the
*deep-linked* param values (a page served for `/projects/1` has `id=1` baked into
its HTML). Adopting that DOM for a client navigation to `/projects/42` would show
stale content, so case 2 must mount **fresh** (`ssr = false` from then on — the
SSR'd range belongs to the old url). This is the one deliberate exception to the
"deactivation never disposes" invariant (§1): the dispose happens at *mount
time* as a param replacement (the same thing `$__uni_mount` already does to a
previous instance on the same host), not as a side effect of navigation-away.

### 6.5 Query params

The ideation explicitly defers query strings ("we'll have to be responsible for
query parameters, and so much bullshit around urls") — so they are scoped to the
URL layer and kept minimal:

- **Server:** the query string is split from the path before matching and decoded
  values are stored as `__query_<k>` parameters (§3.4); `page.query_param(key)`
  reads one. (`set_route_url` receives the path only; the app passes
  `req.query` separately or the router parses it from the raw URL — decide at
  Phase 5, the split is mechanical.)
- **Client:** the router record keeps a `$_us` signal holding the parsed query
  object of the current URL; `router("m").query()` returns it. Being a signal,
  `{r.query().search}` bindings re-render when navigation changes the URL — same
  reactivity story as §5.4, no new machinery.
- **Writing:** `router("m").setQuery({ search: "x" })` = `history.replaceState`
  + signal update (no navigation, no scroll reset). Full pushState semantics for
  query-only changes are a v2 refinement.
- v1 covers plain `key=value&k2=v2`; arrays (`k=1&k=2` → last wins) are documented
  behavior, not an error.

### 6.6 Path building and the production `Link`

Matching exists in §6.1; its inverse was missing. The emitted match table also
serves reversal: `router("m").buildPath("projects", { id: "42" })` returns
`/projects/42` (params are percent-encoded at build time; a missing required
param is a compile diagnostic when literal, a runtime error when dynamic).

`Link` (the production version) combines the pieces:

```chemical
#universal Link(props) {
    const r = router(props.router || "main-router")
    // active state comes from the §5.4 signals — no manual wiring (D-7.8: URL
    // routes subscribe to $url, id routes to $current)
    const active = props.routeId
        ? r.$current.value == props.routeId
        : r.$url.value == r.normPath(props.href)   // tolerant of base/query/slash
    <a href={props.href}
       aria-current={active ? "page" : null}
       onMouseEnter={() => { if(props.preload) { r.preload(props.routeId) } }}
       onFocus={() => { if(props.preload) { r.preload(props.routeId) } }}
       onClick={(e) => {
           if(!window.$__uni_should_intercept(e, props.href)) { return }   // §6.3
           e.preventDefault(); r.activateRouteByUrl(props.href)
       }}>
        {props.children}
    </a>
}
```

- **Hover/focus prefetch** (`<Link preload>`): calls `preload(id)` on
  `mouseenter`/`focus`. (`onMouseEnter`, not `onPointerEnter`: the runtime wires any
  `onX` prop to the lowercase event `x` — `key.substring(2).toLowerCase()`,
  page.ch:1300 — so both work, but `onMouseEnter` matches the components library and
  the harness's dispatched `mouseenter` event. `mouseenter` does not bubble, which
  is irrelevant here because the listener is on the element itself.) For eager-SSR
  routes that hydrates early; for
  fetch-on-demand URL routes (§6.7) it warms the fragment request. This was
  explicitly called out in the ideation ("hovering the Projects link causes the
  server/browser to start downloading its HTML before the click") and was missing
  from the first draft.
  - Prefetch is **idempotent and once-per-element**: `preload` no-ops when the
    route is already hydrated or a fetch is in flight, and `Link` fires it at
    most once per mount (a flag on the handler) so sweeping the pointer across a
    nav bar cannot enqueue one request per mouse-move. There is no
    `pointerleave` cancellation in v1 — an already-warm route is harmless, and
    cancelling a fetch that is about to be used by a click would be worse.
- **Keyboard and modified clicks** follow §6.3 (`window.$__uni_should_intercept`): Enter
  on a focused anchor fires `click` with `button === 0` and no modifiers, so
  keyboard navigation works with no extra code; ctrl/middle-click falls through to
  the browser.
- Active styling is `aria-current` + the reactive class shown above; a
  `class={...}` merge with `props.class` follows the existing class-merge rules.
- `Link` is deliberately **not** a global click interceptor: plain `<a href>` in
  user markup does a full page load (MPA navigation, §6.8). Interception is
  opt-in per element, which keeps the runtime out of the document-level event
  path and avoids surprising users who link to non-router pages.

### 6.7 Fetch-on-demand routes, fragments, redirects, scroll & focus

- **Fetch-on-demand (URL layer, beyond eager/lazy SSR):** a route declared
  `route remote "/reports" { <Reports /> }` ships **no SSR HTML**; first activation
  fetches the route's markup from a fragment endpoint. The library ships the
  *builder* — `route_fragment_response(page, router, id)` renders one route's HTML
  alone — while endpoint wiring stays in user code (`net_http`), preserving the
  "router lib must not depend on `net`" rule. On arrival the fragment is mounted
  fresh (`$_urn` path); the boundary marker for the route is emitted as an empty
  placeholder. This closes the ideation's "500 KB Admin" case completely: remote
  routes cost bytes only when visited (or hovered).
  - **Fragment failure is a defined state, not a hang.** The fetch has four
    outcomes: success (mount + `hydrated = true`), HTTP error, network error, and
    a page whose router never declared the route (endpoint returns 404 by
    contract, §12.8). On any failure the router clears the in-flight flag, leaves
    the previous route active, records the failure on the record, and reports
    through `$__uni_router_error` (§2.1). A second activation **retries** (the
    in-flight flag is the only suppression) — a transient network blip must not
    permanently poison a route. The placeholder boundary stays empty, so a failed
    navigation shows the old route, never a blank container.
  - The fragment response is rendered by the *same* SSR pipeline and therefore
    contains a `[data-chx-i]` boundary (or comment marker) that the client
    resolves with `$__uni_boundary` before mounting — the fragment is not
    injected as raw `innerHTML` (§12.7).
- **Redirects:** client — `router("m").replaceRoute(id)` (history `replaceState`,
  no history spam on login→dashboard flows); server — `redirect_to(page, router,
  id)` is `add_parameter` under the hood, or the user returns a real 302 from
  `net_http`. No `redirect` syntax in route bodies in v1: redirects at activation
  time would fight the guard model; they belong in handlers (server) or before
  activation (client).
- **Scroll & focus (all activations, not just URL ones — §12.4):** on activate,
  the route's remembered scroll offset is restored — `0` for a route never seen,
  the saved offset when returning to one (including via back/forward, which is
  why `history.scrollRestoration = "manual"`, §6.2) — and the route's root
  receives focus (`tabindex="-1"`, `focus({ preventScroll: true })`) so
  keyboard/screen-reader users land in the new view. Per-route `route noscroll`
  opts out of the scroll write entirely (a route that owns its own scrolling).
  On deactivate, the router records two things on the record: the route's current
  scroll offset, and `document.activeElement` **if it lives inside the route**;
  re-activating restores focus to the recorded element (falling back to the
  root). Without this, hiding the active route drops focus to `<body>` and
  keyboard users lose their place, and a focused `<input>`'s value survives (DOM
  persists) but focus does not.
  - **Scroll target:** the wrapper is `display:none` while hidden, so offset is
    read from `window.scrollY`/`document.documentElement` when the route *was*
    visible, not from the hidden wrapper (a hidden element reports no meaningful
    scroll). Recorded on deactivate; restored on activate.
  - **Focus + `aria`:** the wrapper owns `tabindex="-1"`; the focusable content
    inside a deactivated route is unreachable anyway because `display:none`
    removes it from the tab order and the accessibility tree — no `inert` needed
    for v1 (unlike a `visibility`/`content-visibility` mechanism, which would
    need it; that is why §12.9's alternative mechanism is a *real* decision and
    not a cosmetic one).
  - **Announcement:** v1 does not add an `aria-live` region; a route change is a
    user-initiated focus move, which screen readers already announce. If real
    usage shows otherwise, an opt-in `router aria-live` is additive.

### 6.8 Multi-page reality

The router is **intra-page**. A real site is several `HtmlPage`s; navigation
between pages is ordinary `<a href>`/MPA links — no client transition, full page
load, which is fine and honest for a static-first architecture. The URL layer
makes each page's router agree on path patterns so deep links land on the right
page+route. SPA-style cross-page transitions are explicitly out of scope (see
§10).

---

## 7. Performance design

Priorities in order (per the request): implementable, powerful/flexible, performant.
The performance budget exists to keep us honest, not to lead the design.

| Operation | Cost | Mechanism |
|---|---|---|
| Navigate to hydrated route | 2 attribute writes (`data-uni-route-active`, one per route) + 2 guarded `!==` compares + 1 identity compare (+ short `url` compare on URL routes) + one bounded focus step and one optional `scrollTo` | §2.1/§15.2 — corrected in §15.9-Q12; measured by §15.6 P-1 |
| First navigate to SSR'd route | one hydration (same as today's dispatch) | §2.2 |
| Initial page with default route | identical to a normal page + hidden siblings | §3.4 |
| `preload` route | hydration at load, off the interaction path | §5.1 |
| Hidden SSR bytes | (N−1) × route HTML in the response | accepted; lazy URL routes avoid it (§6) |

- No DOM queries: boundaries resolve through the existing id/marker map, built once
  (§2.2); route records are created at bootstrap.
- No string hashing in the switch path: the routes object is a plain record;
  engines optimize monomorphic property access. (The ideation's `route_id = 17`
  integer table is the degenerate case of this and can be a later optimization.)
- No hydration of hidden routes unless `preload` or activated — hidden DOM is inert
  (zero effects, zero listeners), which is precisely the "dead DOM" model from the
  ideation.
- Memory: a route record is `{el, comp, props, hydrated, visible, inst}`; per-router
  overhead is one object + N records. Disposal follows the existing instance tree
  (page unload = GC; disposal only when the owner subtree is disposed).
- Bytes: the router runtime is ~40 lines of JS, emitted once per page that uses
  routers; zero cost for pages that don't (extension-library rule, §0.5).

Measured budgets (CI gates, following the professionalization plan's policy):
navigate-to-hydrated-route ≤ 1ms for a 100-node route; 1000 alternating activations
between two hydrated routes ≤ 50ms; router runtime ≤ 20KB unminified (≤ 5KB
gzipped) for a URL-routing page, ≤ 13KB/≤ 3.5KB for an id-only router, both
**measured** in §15.2 — the earlier 2KB/"~40 lines" estimate was wrong by ~7×
because it ignored the URL layer, the remote path, focus/scroll and the method
table (§15.9-Q12); zero hydration of non-preloaded
hidden routes at load (assertable in the WebView suite). **Every one of these has a
measurement method in §15.6 (P-1…P-8)** — a budget without a harness is not a
budget.

## 7.5 Server-side performance (the honest cost analysis)

The client path is cheap by construction; the server path is where a naive router
integration gets expensive. Analysis by request type, for a page with R routes:

**Cost driver 1 — eager SSR of every route, every request.** Default SSR renders
all R route bodies into `pageHtml` on *every* request, even though R−1 ship
hidden. For R=10 with 100-node bodies that is ~10× the markup work of a
single-page render. This is the price of instant navigation and it must be
stated, not hidden. Mitigations, in the order they should ship:

1. **Static-route snapshot cache (the big one).** Most routes are *static*
   markup — their SSR output depends on nothing request-scoped. The macro
   already has the machinery to know this at compile time (the SSR evaluator
   folds static expressions today), so for each route the generated server
   function can render **once** into a module-level `std::string` snapshot and
   append it per request (`append_view` of one pointer+length — no per-node
   work). Dynamic routes (props from request params, query values, request
   context) render per request as today. This turns R eager routes into R−k
   renders where k = static routes, typically all of them. Safety: snapshot
   strings are page-independent by construction (no request values reachable);
   the validator for this is the same static/dynamic classification the
   converter already performs for SSR vs client-only decisions. A dynamic read
   that the classifier misses is caught the same way every converter
   unsupported-construct is caught: it is conservative (classifies as dynamic),
   never silently wrong. `<style>`-emitting helpers and `#css` calls inside
   route bodies append to `pageCss` — those must still run per page (cheap;
   the dedup maps already make repeat appends free), so snapshotting covers
   `pageHtml` only, explicitly documented.
2. **`remote` routes** (§6.7) opt out of the response entirely — zero server
   bytes until visited.
3. **The debug/validation build** (`--mode debug_complete`) skips snapshots and
   renders everything per request, so snapshot corruption surfaces as SSR
   mismatches in tests rather than silent staleness in production.
4. **Snapshots are id-safe — verified, not assumed (D-2.5).** A cached snapshot
   contains hydration boundary ids. If those ids came from a *per-request
   counter*, two snapshots could both contain `u17` and `$__uni_boundary` would
   resolve the wrong element — a silent cross-route hydration bug. They do not:
   the boundary id is `"u" + element.loc` (`lang/libs/html_cbi/src/converter/language/component.ch:292`),
   i.e. **derived from the element's source location**, so it is identical on
   every render and unique per source position. Two distinct route declarations
   can never collide, and appending a snapshot never renumbers anything else.
   This is what makes §7.5 implementable at all; the Phase 7 test asserts it by
   rendering the same page twice (cold and snapshot-warm) and comparing ids.

**Cost driver 2 — the JS/CSS work is request-independent.** Component JS
emission, the router registry, match table, and CSS are identical for every
request of the same page. In a static-export build (`writeToDirectory`) this is
paid once at build time and served as files — the cheapest deployment and the
recommended default when routes don't depend on request data. For per-request
serving (`net_http`), the professionalization plan's segmented-section output
means the JS/CSS sections are stable per page; a server can cache the rendered
JS/CSS strings and rebuild only `pageHtml` per request (a `page`-level cache
hook, Phase 5 optional; the router library must not depend on it).

**Cost driver 3 — per-request overhead of the router itself.** The parameter
store is one hash-map insert per parameter (~100ns class), URL matching is a
linear scan over R compiled patterns with segment compares (µs class for
realistic R; `unordered_map` on static prefixes if it ever matters), and the
snapshot append is one memcpy. Total router overhead per request is dominated
by the SSR work above, not by routing logic.

**Integration shape (server):** the router never touches sockets. `net_http`
user code parses the request, calls `page.set_route_url(req.path, base)` (or
`add_parameter` for id routing), renders the page, writes the response. The
library depends on `page` only — `net` stays out (§6.1). For static exports,
`set_route_url` is simply not called; deep links are served by mapping the URL to
the pre-built page file and the client match table selects the route in the
browser (so a static host needs a rewrite map from URL → page file — see §13.4,
which requires emitting that map as an asset).

**Cost driver 4 — thread safety (new; a threaded server is the default shape).**
Three pieces of state exist on the server and each has a different owner:

| State | Owner | Concurrency |
|---|---|---|
| `HtmlPage` (parameter store, buffers, `__route_*`) | one per request, created in the handler | safe by construction — never share a page across requests |
| `match_route` / `RoutePattern` tables | compile-time constants, read-only | safe (pure function over immutable input) |
| static-route SSR snapshots (module-level `std::string`) | shared across requests | **needs guard** |

Snapshots are the one genuinely shared mutable thing. The rule: a snapshot is
built **once**, read-only afterwards, with publication synchronized (`std::once`
per route, or a mutex around the first build). Readers then do
`page.append_view(snapshot)` with no lock — reading a published, never-mutated
`std::string` concurrently is safe, and `append_view` only reads it. The design
must not use lazy build-on-first-request *without* a once-guard: two threads
racing to build the same snapshot would tear the string. Because this is a
correctness issue that only appears under load, the Phase 7 exit gate includes a
multi-threaded render test (`--libs`), not just the single-threaded snapshot-reuse
assertion. Everything else in the router is either per-page or immutable — there
is no global router registry on the server (unlike the client's
`$__uni_routers`), which is deliberate.

**Numbers to hold ourselves to (extend §7's CI gates):** per-request render of
a 10-route page with all-static routes + snapshot cache ≤ 1.2× the single-page
baseline; URL match + param store ≤ 10µs for R=50; snapshot reuse verified by
an `--libs` test asserting the snapshot string is appended and not re-rendered
(counter exposed in debug builds); a 8-thread concurrent render test with the
snapshot cache enabled produces byte-identical output to the single-threaded run
(§13.3, the thread-safety gate).

---

## 8. Testing strategy (native WebView suite; e2e-in-another-language is deprecated)

The Playwright suite (`lang/compiled/components-e2e`) is **deprecated for new
tests** — testing is shifting to our own `#universal_test` WebView harness
(`lang/tests/universal_webview/`, run with `./scripts/test.sh --tcc
--universal`), which renders fixtures with the exact production SSR + hydration
pipeline and runs raw JS steps in a real WebKit2GTK WebView. The router's test
plan is built on it, plus the compiler-plugin and negative suites.

1. **Converter/plugin tests** (`lang/tests/compiler_plugins/universal/src/`):
   - router/route parse + SSR emission (`to_string.ch` style): wrapper attrs,
     default route visible, fallback ordering, nested routers.
   - diagnostics (§4.4): duplicate ids, orphan `route`, unknown `activateRoute` —
     asserted as compiler errors via the negative-test harness
     (`lang/tests/negative/`).
   - JS emission: activation stubs reference defined component functions;
     `$__uni_activate` emitted once.
2. **Native WebView tests** (`lang/tests/universal_webview/`, `#universal_test`):
   the full behavioral matrix below. Fixtures are inline `#universal` components
   + one raw-JS `<script>` steps element; `isolate` for tests that touch globals,
   history, or fetch.
3. **Server-side unit tests** (`lang/tests/libs/router/`, `--libs`): parameter
   store, URL matching, query parsing, path building (§8.2).

### 8.1 Behavioral matrix (all in `#universal_test`)

| Suite file (planned) | Covers |
|---|---|
| `router_navigation.ut.ch` | first activate, re-activate, self-activate, unknown id → contained `$__uni_router_error` (page still runs), previous route stays active |
| `router_hydration.ut.ch` | SSR shows only default route; activate→hydrate→visible; effects run exactly once across activate/deactivate/activate; `preload` hydrates at load, `lazy`/`remote` do not |
| `router_state.ut.ch` | state preserved across deactivation; `$current` signal re-renders active links; `aria-current` |
| `router_hooks.ut.ch` | `onActivate`/`onDeactivate` ordering, guard `onBeforeActivate` cancels (current route kept), hook re-entrancy queue (§2.1), hook error isolation |
| `router_url.ut.ch` (`isolate`) | client match table: deep link = client nav = popstate deliver identical props; base stripping, trailing slash, percent decoding; fallback on miss; `buildPath` reversal; `setQuery` updates the query signal |
| `router_url_history.ut.ch` (`isolate`) | history integration + the opaque-origin fallback (§12.5): router works when `pushState` throws; back/forward unavailable but activation fine |
| `router_remote.ut.ch` (`isolate`) | `remote` routes: placeholder boundary, fragment mount on activate, hover-prefetch warms, double-activation does not double-fetch |
| `router_nested.ut.ch` | nested routers + `<Outlet />` (Phase 6): layout state preserved across child switch |

> Harness facts that shape this plan: **one page / one WebView, tests run
> sequentially** — router fixtures must scope all state to their container and
> use `isolate` for history/globals/fetch; steps are **raw JS** (use
> `await t.sleep()` around hydration); **15 s timeout** per test; uncaught errors
> fail the test. Router tests can assert "no other route hydrated" by inspecting
> `window.$__uni_routers` directly — no mocking needed.

### 8.2 Server-side tests

The parameter store and URL matching are pure server code — unit tests in
`lang/tests/libs/router/`: pattern matching, param extraction, query parsing,
base stripping, percent decoding, `buildPath` server-side reversal,
`redirect_to`, and `route_fragment_response` byte-exactness (a fragment response
must contain the route boundary + markup and nothing else).

Required matrix additions (following the professionalization plan §4):

| Dimension | Cases |
|---|---|
| Navigation | first activate, re-activate, self-activate, unknown id |
| Hydration | lazy, preload, default route, nested router |
| Server params | present, absent, wrong type, request context |
| URL | match, param extraction, miss→fallback, miss→404, base-path stripping, trailing slash, percent decoding |
| Params | server deep-link, client nav, popstate — all three deliver `props.id` |
| Query | parse server, parse client, `setQuery` replaceState, re-render on change |
| Guards | allow, deny (current route kept), deny-then-error isolation |
| Active links | signal updates on activate/deactivate/popstate, `aria-current` |
| Prefetch | hover hydrates eager route, hover warms remote route, no double-hydration |
| Errors | duplicate ids, orphan route, bad body roots, bad param name (all compile-time) |

---

## 9. Implementation plan (ordered, each phase shippable)

**Phase 1 — parameter store (no compiler changes).**
`PageParameter` (in `page`, §3.1), `HtmlPage.parameters` + the store/status/manifest
methods, `RouteRequest`, extension functions, unit tests in
`lang/tests/libs/router/`. Exit: store roundtrips; page
builds on both backends; zero cost when unused.

**Phase 2 — runtime core (page.ch only, no compiler changes).**
`$__uni_routers` registry, `$__uni_activate`, `route_visible`, `$__uni_router_error`,
hook invocation, unknown-route path. Exercisable by hand-written JS in a
`#js` block. Exit: WebView test proves activate→hydrate→show with a hand-built
registry.

**Phase 3 — syntax + codegen.**
Keywords (append-only), `JsRouterDecl`/`JsRouteDecl` nodes, parser dispatch,
converter emission (SSR wrapper + stub + props), compile diagnostics (§4.4).
Plugin tests + negative tests. Exit: the `App` example from §4.1 compiles and
passes the WebView suite end-to-end.

**Phase 4 — control API + modes + hooks.**
`router("name")` handle with compile-time name checking, the `preload`/`lazy`
*behaviour* (`preload` is the D-7.5 hydrate-without-showing path; the keyword itself
is parsed in Phase 3), `onActivate`/`onDeactivate`/`onBeforeActivate`, reactive router state
(`$current` as a `$_us` signal — §5.4), `replaceRoute`. Exit: §5 examples compile;
active-link reactivity proven in WebView; budgets from §7 measured in CI.

**Phase 5 — URL layer.**
`set_route_url` + generated-function matching (base path + normalization,
§6.1), route params server-side, client-side match table + params-as-props
(§6.4), query params (§6.5), `Link` click interception guards (§6.3),
`buildPath`, production `Link` with aria-current + hover prefetch (§6.6),
`$__uni_sync_url` with the opaque-origin fallback (§12.5), scroll/focus
handling, `replaceRoute`/redirect helpers, `route_fragment_response` for remote
routes (§6.7).
Exit: §6 examples pass in the WebView suite (`router_url*.ut.ch`,
`router_remote.ut.ch`); server 404 path verified in `--libs`; deep-link +
client-nav + popstate all deliver identical props.

**Phase 6 — nested routes / outlets.**
`route` inside `route` (§11 fix A): the outer route renders `<Outlet />`, which
compiles to a nested anonymous router owning the children; each nesting level is
hydrated independently so the layout never remounts when the child switches. URL
nesting (patterns like `/projects/{id}/edit` activating outer+inner) via
segment-prefix matching on the same emitted table. Exit:
`router_nested.ut.ch` proves layout state survives a child switch; deep URL
with nested params verified in `router_url.ut.ch`.

**Phase 7 — server snapshot cache (§7.5, can land independently any time after
Phase 3).**
Static-route SSR snapshots into module-level strings; debug-mode bypass;
`--libs` tests asserting snapshot reuse and the static/dynamic classification
boundary. Exit: §7.5's per-request budget measured in CI; a route whose body
reads request data is classified dynamic (test), never snapshot-stale.

> Static-export deployment note (§6.8/§7.5): with `writeToDirectory`, the JS,
> CSS, and match-table sections are built once; a static host can serve deep
> links by mapping the URL to the declaring page, and the client match table
> selects the route — no server router logic in the deployment at all. This is
> the cheapest production mode and needs no Phase 7.

Dependency notes: Phase 3 depends on the enum-sync rule (no `--no-build` traps:
C++ is untouched — all parser/plugin code is Chemical, so plugin-only rebuilds
apply; but the CBI plugins *are* rebuilt from these sources, so `--cached-plugins`
must not be used while iterating — D-4.4). Phase 2's segmented-JS dependency is
soft: stubs can be emitted in dispatch order today, and the professionalization
plan's segmented sections improve ordering guarantees without changing this
design.

> **Before starting Phase 1, read §14** — the implementation decision record. It
> fixes the `#` token (D-1.1), contextual-keyword parsing (D-1.2), the AST/emission
> points (D-2.1–D-2.4), the `router()` handle as a runtime accessor plus a
> compile-time validation walk (D-3.1), the package graph — including the rule that
> the router library must import `encoding` and **not** `http` (D-4.2) — the URL
> grammar/precedence/normalization rules that server and client must share
> (D-6.1–D-6.6), the runtime records and queue (D-7.1–D-7.5), the frozen diagnostic
> messages (§14.8), and the testing split (§14.9). Phase 0→1 order is in §14.11.

## 10. Explicit non-goals (v1)

- Streaming SSR / Suspense integration (blocked by architecture; unchanged).
  Data loaders remain out for the same reason: data fetches are the existing
  effect+state pattern inside route components (see `Suspense.ch`), which composes
  with guards/params without new runtime concepts.
- Route transitions/animations (can be added via `onActivate` CSS classes later).
- Integer route ids as the public API (internal optimization only).
- Hash-fragment scrolling (`/page#section`) — orthogonal, works via plain anchors.
- SPA-style cross-page transitions between separate `HtmlPage`s (§6.8).
- Query-array conventions (`k=1&k=2` → last wins is documented behavior).

---

## 11. Production-readiness gap analysis (audit vs React Router / TanStack / Solid Router)

This section records the honest audit of the first draft: what was missing, and
where each fix landed. It doubles as the definition of "production quality" for
this router.

| # | Gap in first draft | Why it matters | Fix / where |
|---|---|---|---|
| A | **No layouts/outlets** — nested routers existed but no `<Outlet />` semantics; URL nesting impossible | Every real app has a chrome + content split that must not remount | §6 Phase 6: nested `route` + `<Outlet />` compiling to an independent nested router; URL nesting via segment-prefix matching |
| B | **Params never reached the component on client nav** — server stored `{id}` params, `Link`/popstate did not | Deep link and client nav must behave identically | §6.4: emitted client match table; params become route props on all three paths |
| C | **Query params absent** — ideation deferred them, plan had raw string only | Production URLs carry filters/tabs/pagination | §6.5: server `__query_` params + reactive client query signal + `setQuery` |
| D | **Router state not reactive** — `isActive()` is imperative; active-link styling impossible in JSX | Nav highlight is the most common router UI | §5.4: `$current` as a `$_us` signal; computed wrappers work unchanged |
| E | **No guards** — auth/unsaved-changes cannot block navigation | Table stakes for production apps | §5.3: `onBeforeActivate` returning false cancels before any DOM change |
| F | **No redirects** (client or server) | Login flows, renamed routes | §6.7: `replaceRoute` + `redirect_to`; deliberately no in-route `redirect` syntax |
| G | **No hover prefetch** — explicitly requested in the ideation PDF | Perceived latency on remote routes | §6.6: `Link preload` wires `pointerenter`/`focus` → `preload(id)` |
| H | **No base path / URL normalization** | Apps under `/app/`, trailing slashes, percent encoding | §6.1: `base` param + normalization rules |
| I | **No scroll/focus management** | UX + accessibility on navigation | §6.7: scroll-to-top (`noscroll` opt-out) + route-root focus |
| J | **No path building** — match without reverse | Hand-building URLs duplicates pattern knowledge | §6.6: `buildPath(id, params)` from the emitted table |
| K | **Param-name typos undetected** (`props.iid`) | Silent `undefined` in route bodies | §6.4: macro validates props reads against the declared pattern |

**Verdict:** with A–K addressed, the router covers the production feature set that
does not depend on streaming SSR (loaders/Suspense stay architectural non-goals,
§10). Every fix reuses existing machinery — signals (§5.4, §6.5), the compile-time
match table (§6.4/§6.6), guard-before-mutation ordering already implied by §2.1 —
so the activation hot path stays "2 attribute writes + pointer compare" (§7). The
runtime additions beyond the first draft are: one signal per router, the emitted
match table, and guard-hook invocation — all paid for only by pages that opt into
the URL layer. §12 is the follow-up pre-mortem: five mechanisms were silently
wrong in early drafts and are fixed in place there.

**Definition of done (production gate):** deep link, client nav, and popstate
produce byte-identical route props; a guard can reliably cancel a navigation;
nav links reflect active state without manual wiring; the ideation's "500 KB
Admin" case costs zero bytes until hover or visit (`remote` routes); all §8
matrix rows pass in the native WebView suite (§8.1); unknown ids/URLs never
throw, only a contained `$__uni_router_error` + fallback.

---

## 12. Pre-mortem: blind spots found before implementation

This section is the "what haven't we thought of" pass. Items marked **mechanism
changed** were silently wrong in earlier drafts and are already fixed in the
sections above; the rest are decisions that must hold at implementation time.

### 12.1 Param-route identity breaks the "already active" fast path — **mechanism changed**

`activateRoute("projects")` after navigating `/projects/1 → /projects/42` must
NOT be a no-op: same id, different params. The §2.1 two-way check is wrong for
param routes; §6.4 defines the three-way check and the dispose-and-remount rule.
Also note the SSR-prop contradiction it exposes: a deep-linked page has the
*server's* param values baked into its hidden route HTML — client nav to a
different param must not adopt that DOM.

### 12.2 SSR'd lazy routes conflict with client-side matching — **mechanism changed**

A `lazy` **URL** route ships its SSR HTML in the initial response — but the
response was rendered for *this* request's path, so its param props match only
that path. Navigating client-side to another param value must mount fresh (the
§6.4 rule handles it), and a `remote` route must never adopt SSR markup it does
not have (`ssr: false` at bootstrap). The `ssr`/`url` record fields (§2.1) exist
precisely so the runtime can tell these cases apart; before this audit the
record could not.

### 12.3 `display:none` + reactive style = route silently unhidden — **mechanism changed**

`$__uni_set_prop`'s style-object path rewrites `cssText`; any reactive style
binding on a route root re-running while deactivated would wipe an inline
`display:none` and reveal the hidden route. §2.1/§2.3 now hide routes via a
`data-uni-route-active` attribute + one head-CSS rule (`!important`), which
survives `cssText` rewrites. This would have been a "route randomly shows two
routes at once" heisenbug.

### 12.4 Re-entrancy, focus, and the activation invariants

- **Hook re-entrancy:** hooks run after `r.current` is set; a navigation from
  inside a hook is queued (§2.1) — inline execution would interleave two
  activations over one `r.current` field.
- **Deactivate = hide, and hide kills focus:** deactivation must capture
  `document.activeElement` when it lives inside the route and restore it on
  re-activation (§6.7). Keyboard users otherwise fall to `<body>` on every
  navigation.
- **The pairing invariant (per router, not per page):** after every
  `activateRoute` returns (or fails), *for that router* exactly one route record
  has `visible: true` and it is `r.currentRoute`; every other record of that router is
  hidden with its instance intact. Two independent routers on one page may each
  have one visible route — that is correct, not a violation. The WebView suite
  should assert this invariant for the router under test across every navigation
  test, not just route-specific assertions.
- **Double-hydration guard:** `preload(id)` then `activateRoute(id)` — or a
  hover-prefetch racing a click — must hydrate exactly once. The `hydrated`
  flag is checked inside the single `$__uni_activate` path (no second entry
  point that bypasses it); remote-route fetches set an in-flight flag so a
  double-fetch collapses to one request.

### 12.5 WebView/test-environment realities — **mechanism changed**

The `#universal_test` harness (and embedded WebViews generally) load pages from
opaque origins where `history.pushState` **throws**. `$__uni_sync_url` wraps all
history access in try/catch with an in-memory fallback (§6.2) — the URL layer
degrades, the router does not. Consequence for the harness facts: tests run in
one page/one WebView sequentially, so router fixtures scope state to their
container and use `isolate` for history/fetch tests (§8). The fallback is a
tested path, not an error path.

### 12.6 The CSS-scoping trap (the one that would cost a rewrite)

Route bodies are JSX with `#css`/class helpers — same as any component — but a
route *wrapper* participates in page layout. Two rules the implementation must
not violate:

1. **The wrapper must not be styled by user CSS.** Attribute-driven hiding (§2.1)
   means a user rule like `div { display: grid }` would fight the hide rule only
   via specificity — the router rule uses an attribute selector + `!important`,
   which beats element selectors but **not** a user `!important` rule with equal
   or higher specificity. The wrapper therefore also carries a generated
   router-owned class (e.g. `.chx-route`) and the hide rule is emitted as
   `.chx-route[data-uni-route-active="false"]`, so only a pathological
   user-`!important` on the same class can break it — same escape-hatch semantics
   as the rest of the components library.
2. **The wrapper is the router's element; the boundary span inside it is the
   mount host.** The wrapper is a real element (hide/show, focus, scroll target);
   the `[data-chx-i]` span inside it is `display: contents` and is what
   `$__uni_mount` receives in `"children"` mode (§2.1 note 2). Never mount with
   the wrapper as the host (root mode would adopt/replace it, §2.1 note 2), and
   never focus or scroll-target the `display: contents` span.

### 12.7 Security & data (decided now, cheap now)

- **Route params are untrusted input.** `{id}` values come from the URL; a route
  body that interpolates them into an attribute goes through the normal SSR
  escaping (`appendHtmlEscaped`) — fine. But `dangerouslySetInnerHTML` on a route
  root with a param-derived value is the XSS hole; the macro should **reject**
  `dangerouslySetInnerHTML` in route bodies whose value references a route param
  (compile diagnostic) rather than trust escaping discipline.
- **Fallback 404 must not reflect the path.** The `route *` body is user JSX;
  if it renders the requested path, the user must escape it — document that the
  router passes the raw path as `props.__path` (escaped as a Text param), and
  tests assert a hostile path (`/..%2f<script>`) renders escaped.
- **`remote` fragments are HTML over the wire** — the fragment endpoint must
  serve the same escaped pipeline output (it does: `route_fragment_response`
  renders through the SSR machinery), and the client must not inject it via
  raw `innerHTML` on user-controlled routes without the boundary protocol.
- **The parameter store is server-trusted.** Keys/values come from user *code*
  (plus parsed query/params); nothing client-supplied reaches it unescaped except
  through the §6.1 decode path, which decodes then re-escapes at render time like
  every SSR value.

### 12.8 Server-side correctness details (decided now)

- **`string_view` keys must point at memory that outlives the map lookup** — for
  URL params that means views into a request-scoped buffer; the library documents
  that `match_route` copies param values into page-owned storage (page-lifetime)
  so request-buffer reuse across keep-alive requests cannot dangle. One copy per
  param, at match time, is the acceptable cost. This is also why the generated
  function must store the *copies*, never the original request views.
- **Duplicate activation emission:** two `#html` blocks embedding the same router
  component on one page must not emit the registry twice — the router registry
  emission goes through `require_component`-style dedup (a per-router hash), the
  same mechanism that dedups component JS today.
- **Fragment responses and the JS bundle:** a `remote` route's client function
  must already exist in the *page* that fetched the fragment (emitted because the
  route is declared there). A fragment fetched by a page that never declared the
  router is a 404 — the endpoint is router+id-scoped and validates both.
- **Interpretation mode:** the AST interpreter has no `HtmlPage`/WebView — the
  parameter store and URL matching are pure logic (testable), but nothing router-
  related runs under `--arg-interpret`; document this so a future "why do router
  tests not run in interpret mode" question has an answer.

### 12.9 What we still deliberately don't know (open, resolve at implementation)

Each item below is now a row in §14.10 with a chosen default and the trigger that
reopens it; this subsection keeps the reasoning for why it is open rather than
guessed.

- **`content-visibility` vs `display:none`** as the deactivate mechanism: possibly
  better memory/paint behavior for very large routes, but changes layout
  measurement timing. The attribute contract (§2.1) makes swapping the mechanism
  a one-function change — measure after Phase 2, don't guess now.
- **Signal subscription cost of `$current`** with hundreds of active links: the
  snapshot-notify loop is O(subscribers) per navigation. Fine below ~100 links;
  if it shows up in profiles, batch via the existing microtask effect scheduling
  before considering anything smarter.
- **`transition` animations between routes** need the outgoing route kept visible
  for the animation duration — which conflicts with the pairing invariant
  (§12.4). The hook queue (§2.1) is where a future "deferred deactivate" would
  slot in; out of scope until requested.
- **Nested-router `$current` propagation:** a child navigation should re-render
  parent nav links only if the parent's route id actually changed. Keep per-router
  signals independent (no derived global signal) until a real UI needs the
  combined state.

**Net assessment:** the five things that would have forced a mid-implementation
redesign — param identity (12.1), SSR-vs-client matching (12.2), the CSS hide
mechanism (12.3/12.6), hook re-entrancy (12.4), and the history/opaque-origin
fallback (12.5) — are all now mechanism decisions written into the design rather
than discoveries waiting in the code. What remains open (§12.9) is deliberately
measurement-gated, not assumption-gated.

§12 was a *pre-mortem over the first draft*. The second pass (§13) goes the other
way: it re-read the runtime source line by line and found three mechanisms that
were **factually wrong**, not merely under-specified (the error path that throws,
the mount mode that adopts the wrapper, the server match that assumed a pattern
table existed before render), plus the integration gaps that follow from them.
Section 13 records all of it.

---

## 13. Second pass: verified-mechanism audit (server + client)

This pass re-read `lang/libs/page/src/page.ch`, the `#html` converter, and the
`http` surface, and asked one question per mechanism: *does the design match what
the runtime actually does?* Four mechanisms did not, and one whole integration
was impossible as written. Everything below is either a correction applied to the
sections above or a decision recorded here.

### 13.1 Corrections (design said X, source says Y)

| # | The design said | The runtime is | Where fixed |
|---|---|---|---|
| 1 | recoverable errors log via `$__uni_error` and the page keeps working | `$__uni_error` **throws** (page.ch:469); using it aborts the click handler / hydration flush | §2.1 note 1, §4.4, §5.3: router-owned `$__uni_router_error`, log-only |
| 2 | mount the route with `$__uni_mount(el, comp, props, "root")` where `el` is the wrapper | root-mode hydration **adopts the host as the component root** and *replaces it* on tag mismatch (page.ch:2127) — the wrapper and its hide attributes would be destroyed | §2.1 note 2, §2.2, §2.3, §12.6: mount the `[data-chx-i]` span inside the wrapper in `"children"` mode |
| 3 | (unstated) a param-change remount just mounts again | `$__uni_dispose` (page.ch:2289) tears down effects/signals but **leaves the DOM**, so a fresh mount would hydrate stale nodes | §2.1 note 3, §2.1 code: clear the host's children when `ssr === false` |
| 4 | the user calls `page.activate_route_by_url(name, path)` before render, which matches against the router's patterns | the router is declared *inside* a component — its patterns do not exist on the page until that generated function runs; there is no registry to consult beforehand | §6.1, §6.3.1: split into `page.set_route_url(path, base)` (user) + matching inside the generated function |
| 5 | `res.write_string(page.htmlPageToString())` | no such overload; `htmlPageToString` takes a `name` and emits `<link>`/`<script src>` to four sibling files that only `writeToDirectory` creates | §6.3.1: `page.toString()` for per-request bodies, `htmlPageToString(name)` + `writeToDirectory` for static export |

Each of 1–3 is a *silent* wrongness in the sense that the happy path could pass
casual testing (a `<div>`-rooted route in `"root"` mode works until the root tag
changes; an error path only runs when something is already wrong). These are the
highest-value findings of the pass because they are exactly the class of bug that
otherwise surfaces as a heisenbug in production and a multi-day bisect.

### 13.2 Server side — what a per-request deployment still needs

Beyond the corrected matching flow (§6.1) and the response-mode split (§6.3.1):

1. **Per-route document title (and the head problem).** `pageHead` is an
   append-only `std::string`; the server can emit a `<title>` for the *selected*
   route at render time (the generated router knows the active id), but a
   **client-side navigation cannot change `<title>`, `<meta name="description">`,
   or `rel=canonical`** because those live in `pageHead`, which is only rendered
   by the server. v1 therefore adds one syntax,
   `route #"dashboard" title "Dashboard" { ... }`, which (a) makes the server emit
   the active route's `<title>` and (b) makes the client set `document.title` on
   activation. Everything else in the head is *not* reactive in v1 — document
   that explicitly, because "title changes on navigation" is the one head field
   users expect and notice. A full head-diff layer is a separate feature (it needs
   a head manifest and a client diff), so it is a §10 non-goal until asked for.
2. **404 hygiene.** `route_missing()` → the app sets 404, but a served 404 body
   should also carry `<meta name="robots" content="noindex">`. The generated
   router can emit it automatically when `route_missing()` is set (one
   `pageHead` append), and, symmetrically, a URL that matched a *fallback* route
   should not be indexed as if it were the real page. This is two lines and
   prevents a class of SEO bugs no one notices until a search engine indexes the
   fallback page for every bad URL.
3. **Static-export deep links need a manifest asset, or they do not work.** §6.8
   asserts a static host "maps the URL to the pre-built page file", but a static
   host has no way to know which `.html` declares which pattern unless we emit
   that mapping. `writeToDirectory` must therefore also write a machine-readable
   route manifest (`<name>.routes.json`: `[{pattern, id, fallback}]`) and the build
   should emit a site-level aggregate for rewrites. Without it, "static host +
   client match table" is only true for the *single-page* case; multi-page deep
   links need the rewrite map. This is a deploy-time deliverable, Phase 5.
   **Mechanism (was missing — the manifest previously appeared from nowhere):**
   the generated router function calls `page.add_route_pattern(router, pattern, id,
   is_fallback)` for each declared route at render time (the patterns are known
   there, §6.1), `HtmlPage` accumulates them in a `route_manifest` field, and
   `writeToDirectory` serializes it. In per-request mode nothing reads it, so the
   cost is a few string views per render; in debug builds the accumulator doubles as
   the §13.2.6 observability source.
4. **Fragment endpoint contract (and its security boundary).** The endpoint that
   serves `remote` route HTML is application code, so the library must document
   what it must return and, more importantly, what it must *not* leak: a
   server-auth-gated route (an admin page) must be gated at the fragment endpoint
   too — the router cannot know your auth policy. The fragment handler must
   (a) validate `router` + `id` against its own declaration, (b) run the same auth
   checks the page handler runs, and (c) render with `route_fragment_response`
   so escaping is identical. A fragment that returns the full page (or another
   route's markup) is a correctness *and* information-disclosure bug; the
   byte-exactness test in §8.2 exists to catch the first of those.
5. **Static assets vs. response bytes.** In per-request mode, `page.toString()`
   inlines CSS and JS into every response. That is correct but not optimal; the
   segmentation work in the professionalization plan is what allows serving
   `name.css`/`name.js` once and only rebuilding `pageHtml` per request. The
   router design must not depend on that, but the deployment guide should say
   plainly: *inlining is the v1 default; extracting assets is the optimization*,
   selected by which output method the handler calls.
6. **Observability.** Debug builds should expose a per-request counter block
   (snapshot hits/misses, match attempts, which route was selected for which
   path). This is the only way to verify §7.5's claims in CI without guessing, and
   it is cheap: a struct on the page, incremented at three call sites, dumped in
   `debug_complete` only.

### 13.3 Client side — the gaps that are not about the hot path

1. **Memory is unbounded by design — say so, and give one escape hatch.** "Never
   unmount" is the correct default (state, scroll, focus survive), but every
   visited route keeps its DOM *and* its live effects/subscriptions forever.
   A route with a polling `setInterval` in `useEffect` keeps polling while hidden.
   v1 must (a) document this loudly next to `onDeactivate`, (b) provide
   `router.release(id)` to dispose an **inactive** route's instance (clearing its
   DOM and re-arming `hydrated = false` so it remounts on next visit), and
   (c) state that `release` on the *active* route is an error (`$__uni_router_error`,
   no-op). Without `release`, a long-lived SPA session over many heavy routes is a
   slow memory leak with no user-facing fix; with it, apps choose their trade-off.
2. **Multiple routers per page — semantics, not just "names must differ".** Two
   *independent* routers are a legitimate layout (a sidebar router and a content
   router, or a modal-shell router). The design must state: each has its own
   `currentRoute`/`$current`/`$url`/match table; activation in one never touches the other;
   the §12.4 invariant is per router. `router()` (unnamed) resolves only when
   exactly one router is declared on the page — which is the common case — and is
   a compile diagnostic otherwise (already true in §4.5, now with the multi-router
   behavior spelled out).
3. **Nested router lifecycle.** When the outer router switches, the inner router
   is not reset: it stays `current` on its last child, hidden with the parent.
   Returning to the outer route shows the inner child exactly as left. That is the
   desired layout-preservation semantics (Phase 6), but it has a consequence worth
   writing down: **inner-router URL state and outer-router URL state can
   disagree** while parked. The nested URL scheme (Phase 6, segment-prefix
   matching) must define which one owns the URL when the outer route is re-entered
   — resolved as: the outer router owns the path prefix, the inner owns the
   remainder, and re-entry re-derives the inner from the shared URL rather than
   trusting the parked inner state. Decide it in Phase 6, but record it now so the
   parked-state behavior is not mistaken for a bug.
4. **Prefetch should respect the network.** `preload` on hover is a good default,
   but firing fragment fetches on a metered connection is user-hostile. v1 skips
   prefetch when `navigator.connection && navigator.connection.saveData` is true
   (one property check, no polyfill needed — the property is absent where
   unsupported, and the check short-circuits). Actual navigation still fetches;
   only the *speculative* request is suppressed.
5. **Params reach the component as *runtime values*, not as source text — and the
   escaping rule belongs to whoever emits source.** A client-side match produces a
   JS string in memory (`params.name = "he's"`), so it needs **no** JS-string
   escaping: there is no parser between it and the prop. Escaping is required on
   the *server* path, where param values are interpolated into the generated props
   object as source literals (`js_string_escape`, the AGENTS.md single-quote/
   backslash contract §12.7), and for any compile-time attribute value. The earlier
   text applied the escaping rule to the client matcher, which is both unnecessary
   and somewhere a future reader would "fix" incorrectly; the corrected split is in
   INV-9 and §15.9-Q21. Percent-*decoding* is still required client-side (once, at
   match time) and is the only transformation the client matcher performs.
6. **The `$current` signal is page-lifetime, on purpose.** It is created at
   bootstrap with no owning component instance, so `$__uni_register_resource`
   (page.ch:571) does not attach it to any instance and it is never auto-disposed.
   That is correct (the router outlives every component), and component-level
   `$_ucs` computeds that read it subscribe/unsubscribe through the existing
   dependency tracking, so there is no leak. Document it, because "a signal with
   no owner" reads like a bug to the next maintainer and someone will otherwise
   "fix" it into a component-scoped signal and break routing after a remount.
7. **Deep-link + client-nav + popstate must be *byte-identical*, and the test
   must compare them directly.** §11's definition of done already says this; §8
   now needs the concrete assertion: render the same route via all three paths
   and compare the serialized props object (and the resulting DOM's route root
   class/attrs) for equality, not just "each works".

### 13.4 Deployment modes (now three, explicitly)

| Mode | Server router code | Deep links | Cheapest when |
|---|---|---|---|
| Per-request (`net_http`) | `set_route_url` + `toString()` | matched server-side every request; client table for clicks | routes depend on request data (auth, personalization) |
| Static export | none | rewrite map from the emitted `routes.json` → page `.html`; client table selects the route | all routes static (the recommended default, §7.5) |
| Hybrid | per-request shell, `remote` heavy routes | shell matched server-side; `remote` fragments fetched on demand | a few dynamic routes, several heavy static ones |

All three use the same declarations and the same runtime; only the handler and
the emit call differ. This table belongs in the router library's README at
implementation time, because it is the first thing an app author needs.

### 13.5 New test rows this pass adds (§8)

| Test | Asserts |
|---|---|
| `router_mount_mode.ut.ch` | a route whose root is **not** `<div>` (e.g. `<section>`, `<main>`) activates, stays hidden when inactive, and its wrapper's `data-uni-route-active` still toggles — the regression test for §2.1 note 2 |
| `router_param_remount.ut.ch` | navigate `/p/1 → /p/2`: old DOM cleared (no stale nodes), fresh mount, effects re-run once; `hydrated`/`ssr` flags correct |
| `router_only_default_hydrated.ut.ch` | after initial load, only the default route's instance exists; every other record has `hydrated === false` and no `$__uni_instance` on its host |
| `router_error_no_throw.ut.ch` | unknown id + cancelled guard: handler returns, page continues, previous route visible, exactly one `$__uni_router_error` per failure |
| `router_link_modifiers.ut.ch` | ctrl/middle click and `target=_blank`/`download`/cross-origin hrefs are **not** intercepted; plain left-click is |
| `router_guard_back.ut.ch` | deny a guard on a popstate → URL is re-synced to the visible route (history path and opaque-origin path) |
| `router_title.ut.ch` | `route title` sets `<title>` server-side for the active route and `document.title` on client activation |
| `router_release.ut.ch` | `release(id)` on an inactive route clears DOM + `hydrated`; re-visit remounts; `release` on the active route is a contained no-op |

Server-side additions to §8.2: `set_route_url` + generated matching (match/miss/
fallback/404 flag), `routes.json` manifest contents, multi-threaded snapshot
render (§7.5), and 404 `noindex` emission.

### 13.6 What this pass changes about the phases

- **Phase 1** — unchanged (parameter store). Adds the `__route_url`/`__route_base`
  keys to the documented reserved names.
- **Phase 3** — adds the emission path that registers route bodies *without* the
  hydration queue (§2.2 bullet 5) and the `"children"`-mode mount host. This is
  the phase where corrections 2 and 3 are exercised.
- **Phase 4** — adds `route title` (both sides) since `<title>` is a Phase 4-sized
  decision, not a Phase 5 URL concern.
- **Phase 5** — adds `set_route_url` + generated matching (§6.1), the `routes.json`
  manifest, `Link` click interception guards, the guard/popstate URL re-sync, and
  per-route scroll memory. Larger than before, for the right reasons.
- **Phase 7** — adds the multi-threaded snapshot test to its exit gate.

**Net for the second pass:** the design now matches the runtime on the five
mechanisms it previously got wrong, and the two integrations that were impossible
as written (server-side URL matching, and static-export deep links without a
manifest) are specified. The remaining open questions (§12.9) are still
deliberately measurement-gated; nothing in this pass adds a new open question —
it closes the ones that were silently load-bearing.

---

## 14. Implementation decision record (Phase 0 — decide before writing code)

Sections 0–13 describe *what* the router is. This section is the decision record
for *how each piece is built*, verified against the parser, lexer, converter,
package graph and runtime. Every entry names the constraint that forced it, so a
future reader does not have to re-derive it. Format: **the gap → the decision →
the consequence.**

A note on scope: these are not speculative. Three of them (D-1.1 the `#` sigil,
D-1.2 contextual keywords, D-4.2 the `http` dependency) were discovered *because*
the design as written would not compile or would pull the networking stack into
every routed app. They are the most valuable output of this section.

### 14.1 Lexer & grammar

**D-1.1 — The `#` sigil is not lexable inside a `#universal` body.** *DECIDED:
add a `Hash` token.*

- Verified: `JsTokenType` (`lang/libs/js_syntax/src/TokenType.ch`) has no `#`
  token. The runtime lexer (`js_syntax/src/Tokenizer.ch`) falls through its
  `switch(c)` for `'#'` to `Token { type : 0, value : "unexpected" }`; the plugin
  lexer (`js_cbi_lexer/src/CompilerLexer.ch`, `nextJsToken`, wired by
  `universal_initializeLexer` in `universal_cbi/src/react/macro.ch:127`) has no
  `'#'` case either.
- Verified: **no `#macro` call currently appears inside a `#universal` body.**
  Every `#css` in `lang/libs/components/src/*.ch` is in a plain Chemical helper
  (`return #css { … }`), outside the `#universal` components (e.g. `Alert.ch`
  lines 2–72 vs components from line 104). So `#` inside a universal body is
  free space today — adding it collides with nothing.
- Decision: append `JsTokenType.Hash` to the enum (it is *not* mirrored in C++
  and not part of the CBI enum-sync rule — `JsTokenType` lives entirely in
  `js_syntax`), add a `'#'` case to **both** lexers, and parse `route #"id"`.
- Consequence/risk: if nested `#macro` support inside universal bodies is ever
  wanted, that grammar must reclaim `#`; record it in the macro grammar notes.
  A zero-lexer-change fallback (`route id "dashboard"` / `route url "/x"`) is
  recorded and **not** chosen — `#` is the ideation's token and free today.

**D-1.2 — `router`/`route` must not become reserved keywords.** *DECIDED:
contextual keywords via lookahead.*

- Verified: both are in active use as ordinary identifiers
  (`lang/libs/http/src/server.ch:30,84`, `lang/libs/server/src/main.ch:59`,
  `lang/libs/server/src/async.ch:28`).
- Decision: no new keyword tokens. In `parseStatement`, when the token is an
  `Identifier` whose value is `router`/`route`, peek the **next** token to decide:
  `router` + (`String` | `Identifier` | `LBrace`), `route` + (`Hash` | `String` |
  `Star` | `Default`). Otherwise fall through to the untouched expression path.
  This mirrors the existing destructuring peek (`parser_stmt.ch:41`).
- Consequence: `router`/`route` remain usable as variable names; `route(x)` as a
  function call parses as before.

**D-1.3 — Where each statement is legal.** *DECIDED.* `router` only at the top
level of a `#universal` component body; `route` only inside a `router` block.
Enforced by the parser (the hook lives in `parseStatement`, reached only through
`parseBlock`) and re-checked by the converter, which is what emits the
diagnostic (R1/R2 in §14.8).

**D-1.4 — Mode placement.** *DECIDED (canonical from §4.5):* mode **after** the
id — `route #"admin" preload { … }`. The parser reads: optional `default`, then
(`#id` | `url-string` | `*`), then optional mode keyword, then optional `title`
string, then the body. Modes: `lazy` | `preload` | `remote`; `remote` requires a
URL route (diagnostic otherwise).

### 14.2 AST nodes, parser hooks, and converter emission

**D-2.1 — Enum and AST shape.** *DECIDED.*
- `JsNodeKind` (`js_syntax/src/NodeKind.ch`, tail is `… RegexLiteral, Paren`):
  append `RouterDecl`, `RouteDecl`, `RouteHook`. Append-only, and safe because
  `JsNodeKind` is internal to `js_syntax`: the AGENTS.md enum-sync rule covers
  `ASTNodeKind.h` ↔ `Ast.ch` and `TokenType.h` ↔ `ChemicalTokenType.ch`, neither
  of which mirrors `JsNodeKind`. (Even so, appending rather than inserting keeps
  the same discipline everywhere.)
- Structs in `js_syntax/src/Ast.ch`, matching the existing `JsVarDecl` style:
  ```
  JsRouterDecl { name : std::string_view, routes : std::vector<*mut JsNode> }
  JsRouteDecl  { raw : std::string_view,        // as written: "#x" / "/a/{b}" / "*"
                 id : std::string_view,          // resolved id (id or normalized pattern)
                 is_url, is_fallback, is_default, mode : …, title : std::string_view,
                 hooks : std::vector<*mut JsNode>, body : *mut JsNode }
  JsRouteHook  { name : std::string_view, fn : *mut JsNode }
  ```

**D-2.2 — Parser entry points.** *DECIDED.* Add `parseRouterDecl` and
`parseRouteDecl`; register them in the `parseStatement` chain in
`lang/libs/universal_parser/src/parser/parser_stmt.ch` (the chain starts at
line 31 with `var/const/let/state`, then `if`, `return`, `class`, `async`, …).
Put them in a new `parser_router.ch` in the same package to keep `parser_stmt.ch`
readable; the chain calls one `tryParseRouterStatement(parser, builder)` helper
that returns `null` when the lookahead is not a router statement. Route bodies
parse with the existing `parseBlock`, so anything legal in a component body is
legal in a route body, and JSX-root cardinality is checked at conversion.

**D-2.3 — Two emission paths, one new.** *DECIDED.* Route bodies must **not** go
through `emit_universal_queue` (`lang/libs/html_cbi/src/converter/language/main.ch:417`)
— verified that it is the single dispatcher and it pushes onto
`$__uni_hydration_queue`, which `$__universal_flush()` drains (§2.2 bullet 5).
Add `emit_route_registration(...)` beside it, emitting the stub (§2.2) instead of
a queue push. The SSR wrapper emission is a new `emit_router_server(...)` in
`universal_cbi/src/converter/`. Both are called from the component-body emission
in `react/ast_replace.ch` where `#universal` output is produced.

**D-2.4 — Registry dedup.** *DECIDED.* Registry/wrapper emission is keyed by a
per-router hash through the page's existing dedup map (`HtmlPage.doneComponents`
+ `require_component`, `page.ch:176`) — the same mechanism that dedups component
JS today — so rendering the declaring component twice on one page does not emit
`$__uni_routers["m"]` twice. Additionally: **a router is a page singleton**;
rendering its declaring component more than once is diagnosed when statically
visible and otherwise contained at runtime (R5, §14.8).

**D-2.5 — Route boundary ids are source-location-derived.** *VERIFIED/FROZEN.*
The host id follows the existing universal scheme, `"u" + element.loc`
(`html_cbi/src/converter/language/component.ch:292`; the same id is written into
the SSR `<span id="uN" data-chx-i>` and passed to `$__uni_boundary`). The router
must **not** introduce a per-page route counter for ids: source-derived ids are
stable across renders, which is what lets the §7.5 snapshot cache append cached
markup without id collisions. Corollary for §2.2: the stub's `$__uni_boundary("uN")`
argument is a compile-time constant, not a runtime lookup.

**D-2.6 — Route props reuse the existing serialization path.** *DECIDED.* A
route's JSX-root attributes become its props via the same helper
`emit_universal_queue` uses to build `{ "attr": value }`
(`html_cbi/src/converter/language/main.ch:416` onward) — the attribute-name and
value-escaping rules must live in one function used by both, so route props can
never diverge from `#html`-embedded props (the AGENTS.md single-quote/backslash
contract, and the "unsupported prop type is a diagnostic" rule). Do **not**
write a second props serializer for routes.

**D-2.7 — A `{param}` becomes a route prop on BOTH sides, emitted by two different
call sites.** *DECIDED (closes a hole between §6.1 and §6.4).* A route body reads
`props.id`, so `id` must exist in the props object the component is actually called
with — and there are two callers:

- **SSR:** the generated router function, which owns the compile-time patterns
  (§6.1), emits the route's props with `{ "<name>": page.get_parameter("<name>") }`
  for every `{name}` in that route's pattern. The value was stored by the match
  (§6.1 → §3.4), so this is a page-store read, not a second match. Values are `Text`
  and go through the normal SSR escaping when the component renders them.
- **Client:** the emitted stub's `baseProps` carries a **placeholder** for each
  pattern param (`{ "id": null }`, next to the compile-time attributes) and the
  matcher's `params` overrides it at mount through `$__uni_route_props` (§15.2).
  For the *initially active* route no matcher runs unless the bootstrap performs
  one — which is exactly why `$__uni_activate_initial` (template 5) matches the
  initial URL and passes the params of that route only (§15.9-Q34).

A prop read that is neither a declared root attribute nor a `{param}` of the route's
pattern is diagnostic R11. That is the whole mechanism: there is no runtime
"inject params into props" step, and no per-request prop serialization beyond the
placeholder override.

### 14.3 The `router()` handle: runtime accessor + compile-time validation

**D-3.1 — `router(name)` is a runtime global, not a compiler-only construct.**
*DECIDED.*

- Constraint: a `#universal` body is parsed as **JS/JSX**, so
  `const r = router("main")` and `r.activateRoute("x")` are JS call expressions
  in the emitted output. There is no expression kind that could make them
  compiler-only without inventing new syntax.
- Decision: emit `window.$__uni_router = ((name) => …)` once per page (an
  accessor that creates the record if absent), and add a **validation walk** over
  the parsed body collecting literal-argument calls to `router(…)`,
  `.activateRoute(…)`, `.preload(…)`, `.buildPath(…)` for id checking (§4.4).
  Dynamic arguments skip validation and take the runtime path.
- Consequence: the public surface behaves like a normal JS object
  (`r.$current.value` is a real property read of the `$_us` signal — consistent,
  not special-cased), while literal typos are still compile errors.

**D-3.2 — Internal symbol namespace.** *DECIDED.* Everything the runtime defines
lives under the `$__uni_` prefix, in two groups that must be used consistently
because the previous list named three different spellings for the same idea:

- **router-scoped** (one per router, or the router machinery itself):
  `$__uni_router`, `$__uni_router_null`, `$__uni_router_methods`,
  `$__uni_router_error`, `$__uni_router_queue`, `$__uni_router_busy`,
  `$__uni_router_version`, `$__uni_router_fragment_url`,
  `$__uni_router_methods`;
- **route-scoped / navigation** (helpers that act on a route or on the URL):
  `$__uni_route_register`, `$__uni_route_props`, `$__uni_route_visible`,
  `$__uni_activate*`, `$__uni_preload`, `$__uni_release`, `$__uni_match_url`,
  `$__uni_norm_path`, `$__uni_parse_query`, `$__uni_build_path`,
  `$__uni_should_intercept`, `$__uni_set_url`, `$__uni_sync_url`,
  `$__uni_initial_url`, `$__uni_decode_segment`, `$__uni_dispatch`(existing).

There is **no** `$__uni_route_tables` global: the match table hangs off the router
record as `r.table` (§6.4, §15.3 template 3), so there is exactly one place to look
for a router's patterns. Rejecting user references to `$__uni_*` in route bodies is
diagnostic R14, which is why §4.4's last row exists.

**D-3.3 — `router()` with no name.** *DECIDED.* Resolves at compile time to the
single router declared on the page; a diagnostic when zero or more than one is
in scope (§4.5). It is *not* a runtime lookup fallback — a runtime fallback would
silently pick a router when the app has two.

### 14.4 Package graph and build ownership

**D-4.1 — `lang/libs/router/` is a plain module.** *DECIDED.* Template is
`lang/libs/components/chemical.mod` (`module components`, `source "src"`, imports
`page`, `std`, `universal_cbi`, `css_cbi`). The router module imports `page`,
`std`, `universal_cbi`, `css_cbi`, `encoding`. A plain module (not a CBI) can
declare `#universal` components and `#css` helpers.

**D-4.2 — Do not import `http`.** *DECIDED.*
- Verified: `lang/libs/http/chemical.mod` imports `net`, `tls`, `async`, `mime`,
  `json`. Importing `http` from the router library would make every routed app
  link TLS + networking — a direct violation of §0.5.
- Verified: the net-free URL codec is the `encoding` module
  (`lang/libs/encoding/src/url.ch`: `url_encode`, `url_encode_query`,
  `url_decode`; module imports only `std`, `crypto`).
- Decision: `import encoding` for percent coding; implement `parse_query`
  locally (~20 lines) since `encoding` has none. This also supplies
  `buildPath`'s encoding (§6.6) from one source. §6.1's decoder paragraph is
  corrected accordingly.
- Residual cost, stated honestly: `encoding` imports `crypto` (hashing). That is
  far lighter than `http`'s `net` + `tls` + `async` + `mime` + `json`, but it is
  not zero. If it is ever judged too heavy for a routed app, the fallback is to
  vendor the ~120-line URL codec into `lang/libs/router/src/` — record the
  measurement, then decide; do not pre-emptively duplicate it.

**D-4.3 — The syntax/codegen belongs to the existing `universal` CBI.**
*DECIDED.* No new CBI: `universal_cbi/build.lab` builds CBI name `"universal"`
with dependencies on `js_syntax`, `js_cbi_lexer`, `universal_parser`, `compiler`,
and `universal_parseMacroNode`/`universal_initializeLexer` are its entry points.
A second CBI would double-register the annotation controller and split the
grammar. So the change set is:
`js_syntax` (TokenType + NodeKind + Ast) → `js_cbi_lexer` (the `#` case) →
`universal_parser` (the statements) → `universal_cbi` (conversion).

**D-4.4 — What rebuilds while iterating.** *DECIDED/CAUTION.* No C++ changes
(`--no-build` is safe for the compiler binary), **but** the CBI plugins are
TinyCC-compiled from these Chemical sources, so `--cached-plugins` must not be
used and a plugin rebuild happens on every change (AGENTS.md). Every Phase 3
test run should therefore be `./scripts/test.sh --tcc` without
`--cached-plugins`.

### 14.5 Parameter store decisions (Phase 1)

**D-5.1 — Container.** `std::unordered_map<std::string_view, PageParameter>`;
precedent `lang/libs/css_parser/src/parser/value/all.ch` uses
`unordered_map<std::string_view, T>`, so hash/eq exist. It is node-based, so
`get_ptr(key)` references stay valid across later inserts — the returned
`*mut PageParameter` is safe to hold for the render. Document that; it is the
reason to prefer it over a rehashing flat map.

**D-5.2 — Reserved key namespace.** Router internals use the `__route_` prefix
(`__route_url`, `__route_base`); query values use `__query_<k>`. **Decision:**
`add_parameter` asserts in debug (and documents) that user keys do not start with
`__route_`/`__query_`, so internal state cannot be clobbered by app code. The
router-name→id key is exempt (it is the user-facing `add_parameter("main", …)`).

**D-5.3 — Lifetime.** `Text` views are page-lifetime; `Object` pointers must
outlive the render (§3.2). Server URL params are **copied** into page-owned
storage inside `match_route` before storing views, because the request buffer may
be reused by the next keep-alive request (§12.8). One copy per param.

**D-5.4 — Server method scope.** `set_route_url` stores the path only; the app
gates on method (`if(req.method is "GET") …`). Query is stored separately
(`__query_*`) so the matcher never sees a query string.

### 14.6 URL matching decisions (Phase 5)

**D-6.1 — Pattern grammar (frozen for v1).** Literal segments and `{name}`
single-segment params only. No mid-pattern `*`, no optional segments, no regex.
The only catch-all is `route *`. Unsupported forms are diagnostics (R13).

**D-6.2 — Precedence and ambiguity.** Decide order: (1) more literal segments
first; (2) fewer params; (3) declaration order. Two patterns that can match the
same path with the same shape are an **ambiguity diagnostic** (R9); otherwise
precedence resolves deterministically. This rule lives in one function
(`match_route`) used by **both** the server matcher and the client table, so the
three delivery paths (§6.4) cannot diverge.

**D-6.3 — Normalization.** Strip one trailing slash (except root); do not
collapse `//`; percent-decode **after** segment splitting (so `%2F` in a param is
data, not a separator); literal compare is case-sensitive; `.`/`..` never match.
Same function both sides.

**D-6.4 — `buildPath` encoding.** `encoding::url_encode` per param segment (not
`url_encode_query`), so a `/` in a param becomes `%2F` and round-trips under
D-6.3.

**D-6.5 — Route identity.** The record id for a URL route is a stable
normalization of the pattern as written; the macro generates it and the client
table keys on it. `activateRoute(id)` on a param route without a `url` argument
uses the record's current `url`; if required params are unknown and the id is
literal, it is a compile diagnostic, otherwise a contained runtime no-op. Param
routes are normally entered with `activateRouteByUrl(path)` (§6.4).

**D-6.6 — Default/fallback precedence.** Server parameter wins; then
`route default`; then `route *`; else `mark_route_missing()` (§6.1). A router
with neither a `default` nor a fallback is a **warning** (R10), not an error —
an id-only router driven entirely by `add_parameter` is legitimate.

**D-6.7 — `Link`/`NavLink` derive active state from the URL, not from a
required prop.** *DECIDED.* The §6.6 sketch made `NavLink` need both `href` and
`routeId`, which is redundant and easy to get wrong (and impossible for a plain
`<Link>`). Instead, active state is read from the router signals: for a URL route,
`r.$url.value === normalize(props.href)`; for an id route,
`r.$current.value === props.routeId`. `props.routeId` is therefore accepted only
for id routes (where there is no URL). Both comparisons are the same
normalization the matcher uses (D-6.3), and reading the **signal** (not
`r.currentRoute`) is what makes the link re-render on navigation — a plain field
read is not a subscription. This removes a prop, removes a class of bugs, and
makes `<Link>` and `<NavLink>` the same component with a class function.

**D-6.8 — Multiple URL routers match independently; cross-router overlap is not
statically detectable.** *DECIDED/DOCUMENTED.* Each router's generated function
matches its own patterns against the same `__route_url`, so two routers that both
declare `/projects` will both select it. Declarations can live in different
modules, so a compile-time cross-router check is not generally possible; the
design therefore (a) emits the router name into the §13.2 `routes.json` manifest,
(b) documents "a URL pattern should be owned by exactly one router; scope them by
prefix", and (c) leaves a page-assembly warning (R10-style, best-effort) as an
option. This is the same class of decision as §12.9's nested-`$current` note:
kept simple, documented, revisitable.

**D-6.9 — Precedence is resolved at emission time, not at run time.** *DECIDED.*
The emitter sorts each router's patterns once (more literal segments → fewer
params → declaration order, D-6.2) and emits the match table **in that order**, so
both `match_route` (server) and `$__uni_match_url` (client) are a straight
first-match scan with no scoring code. This removes the one place where two
implementations of the same rule could drift, and it makes client matching
allocation-free for literal routes (§15.6 R-6).

**D-6.10 — URL writing is owned by activation, not by its callers.** *DECIDED.*
`$__uni_activate_now` performs the `pushState`/`replaceState` (for `route.isUrl`
routes) exactly once, selected by an explicit `historyMode` argument:
`0 = none` (popstate, the initial activation, and any id-route activation),
`1 = push` (a `Link` click, `activateRouteByUrl`), `2 = replace`
(`replaceRoute`/`replaceRouteByUrl`). Before this, the write lived in the caller
(`$__uni_activate_by_url`) and a direct `activateRoute` on a URL route would mount
the route and leave the URL stale — the exact class of "URL and visible route
disagree" bug §6.2 warns about. A guard denial on popstate re-syncs with mode 2.

### 14.7 Runtime state-machine decisions (Phase 2)

**D-7.1 — Activation queue = FIFO drain, not a microtask.** Simplify §2.1's
parenthetical: `$__uni_router_queue` + a `busy` flag; `$__uni_activate` sets
`busy`, performs the transition, clears `busy`, then drains the queue in a loop
(re-entrant calls push). Hooks run while `busy` is still set, so a hook-initiated
navigation queues and is applied after the in-flight transition — deterministic
FIFO, easier to reason about and to test than a scheduled microtask. Last-wins
is preserved by the `r.currentRoute === route` comparison. (The network fetch for a
`remote` route remains the only async step; the `inFlight` flag guards it.)

**D-7.2 — Record fields (authoritative).** Per route: `id`, `key` (`"name#id"`,
used verbatim in error messages), `el`, `host`, `comp`, `baseProps`, `params`,
`hydrated`, `visible`, `ssr`, `remote`, `isUrl`, `url`, `rawUrl`, `inst`, `failed`,
`scrollY`, `focusEl`, `noscroll`, `inFlight`, `fragment`, `pendingActivate`,
`fetchUrl`, `beforeActivate`, `onActivate`, `onDeactivate`, `title`. Per router:
`name`, `currentRoute`, `$current`, `$url`, `routes`, `table`, `$query`, plus the
method table of §15.2. (`props` is *derived* at mount time by `$__uni_route_props`
and is deliberately **not** a stored field — §15.9-Q1. `wrapperId` is a *stub
argument*, not a stored field, because registration consumes it immediately.)

**D-7.3 — `route_visible` is the only visibility mutator.** It toggles the
attribute **and** maintains `record.visible`, so the pairing invariant (§12.4) is
assertable from `window.$__uni_routers` without reading the DOM. Focus/scroll
capture happens in the `false` branch and restore in the `true` branch
(§6.7).

**D-7.4 — Bootstrap ordering (verified).** Registry + stubs → `pageJs`;
`window.$__universal_flush();` is appended to `pageJsEnd` by
`defaultUniversalSetup`; the initial `$__uni_activate(…)` is appended to
`pageJsEnd` **after** it. `toString`/`getFinalizedPageJs` emit
`pageJs + pageJsEnd` in that order (`page.ch:2502–2508`), so this needs no
`move_js_range` and is stable.

**D-7.5 — `preload` is activate-minus-visibility.** Same `hydrated` guard
(no double mount), never touches `r.currentRoute`, never writes `$current`/`$url`,
never calls `onActivate`. It **does** mount into the route's own host, so a
`preload` followed by `activate` must find the DOM already mounted and take the
`hydrated === true` branch — that is the double-hydration guard (§12.4), and it
is asserted explicitly (§15.5 INV-2).

**D-7.6 — Mount failures are contained, and never retried in a loop.**
*DECIDED.* `$__uni_mount` catches errors thrown *by the component body* and
renders the error-boundary fallback (page.ch:2198), but it calls the **throwing**
`$__uni_error` for a missing host and a non-function factory (page.ch:2160–2165).
`$__uni_activate` therefore wraps the mount call in `try/catch` and reports via
`$__uni_router_error`. The route is still marked `hydrated = true` so a broken
component cannot become a remount storm (navigating back would otherwise retry
on every switch). If the *host element* is the thing that is missing (an SSR bug),
mark the record `failed = true` and keep it hidden; the page continues.

**D-7.7 — `route_visible` is total.** *DECIDED.* If a record's `el` is null
(SSR/JS mismatch, a component rendered without its boundary), `route_visible`
reports once via `$__uni_router_error` and returns without throwing, and the
record keeps `visible` in sync with `r.currentRoute` so the §12.4 invariant is not
violated by a missing element. No DOM access in the activation path is
unguarded. `route.key` is the `"name#id"` string used in the error message; it is
set by the stub and is the only per-route string the runtime allocates.

**D-7.8 — Two router signals: `$current` (id) and `$url` (resolved url).**
*DECIDED (fixes a real reactivity hole; §15.9-Q2).* A single signal cannot drive
both link styles correctly: a param change (`/p/1 → /p/2`) keeps the record's id
(so an id-keyed link must **not** re-render — that is the O(1) property of §7)
but must re-render a URL-keyed link. Therefore:
`$current` holds the active route **id**; `$url` holds the active **resolved url**
(`null` for id routes). Both are written only when the string differs (so `$_us`
bails and no subscriber runs on a same-route re-activation). Public API:
`r.current()` → id, `r.currentUrl()` → resolved url, `r.currentRoute` → the record
(escape hatch for advanced code, deliberately a *field* so nothing can confuse it
with the methods). Any UI that must react to param changes reads `$url`; any UI
that only cares which route is showing reads `$current`. This is the only correct
partition and it costs one extra signal object per router.

**D-7.9 — `current` is a method; the record field is `currentRoute`.**
*DECIDED (naming freeze).* The §5.2 handle exposes `current()` / `currentUrl()` as
methods, so the record field **cannot** also be called `current` (it would shadow
the method on the same object and produce "`r.current is not a function`" in user
code). Frozen names: field `currentRoute`, signals `$current`/`$url`, methods
`current()`, `currentUrl()`. Every §2/§6/§12/§13 reference to "`r.current`" means
this field.

**D-7.10 — Hydration precedes deactivation; `props` is derived, not stored.**
*DECIDED (fixes two ordering bugs; §15.9-Q1/Q3).* In `$__uni_activate_now`:
(a) the incoming route is mounted **before** the outgoing route is hidden, so a
mount failure cannot leave zero visible routes — the previous route stays on
screen and the call returns `false`; (b) the mount receives
`$__uni_route_props(route)` (a fresh merge of the immutable `baseProps` with the
current `params`), never a mutated `route.props`. Both are visible in the §2.1
source and are asserted by §15.5's invariant tests.

**D-7.11 — `remote` activation is a two-step, and the step boundary is explicit.**
*DECIDED (fixes an unspecified async path; §15.9-Q20).* A `remote` route has no
SSR markup, so it cannot mount on the synchronous path. `$__uni_activate_now`
detects `route.remote && !route.fragment`, records
`route.pendingActivate = [url, params, historyMode]`, starts the fetch if none is
in flight, and **returns without mutating anything** — no visibility change, no
`currentRoute` write, no URL write (INV-21). When the fragment lands,
`$__uni_mount_fragment` adopts it into the route's own host and the fetch
continuation re-enters `$__uni_activate` with the recorded arguments, which now
takes the ordinary synchronous path. A prefetch fetch stores the fragment and
leaves `pendingActivate` null, so the later click consumes it immediately. Every
failure clears `pendingActivate` and leaves the previous route visible; the next
activation retries, because only an in-flight fetch suppresses a fetch.

### 14.8 Diagnostics catalogue (frozen)

All are parser/converter diagnostics with a `SourceLocation` except R5, which is
a contained runtime report (`console.error`).

| # | Trigger | Severity | Message |
|---|---|---|---|
| R1 | `route` outside `router` | error | `'route' declaration is only valid inside a router block` |
| R2 | `router` not inside a `#universal` component | error | `router must be declared inside a #universal component` |
| R3 | duplicate route id in one router | error | `route '#x' is declared twice in router "m"` |
| R4 | duplicate router name in one module | error | `router "m" is declared twice` |
| R5 | second registration of a router name at runtime | runtime | `router "m" already registered` |
| R6 | route body with 0 or >1 JSX roots | error | `route body must render exactly one root element` |
| R7 | `route *` duplicated or not last | error | `fallback route must be the last route` |
| R8 | literal id in `activateRoute`/`preload`/`buildPath` not declared | error | `no route '#x' in router "m"` |
| R9 | two patterns can match one path identically | error | `route patterns '/a/{x}' and '/a/{y}' are ambiguous` |
| R10 | router with no `default` and no `*` | warning | `router "m" has no default route; the page renders inert without a server parameter` |
| R11 | a `props.X` read in a route body that is neither a declared attribute on the route root nor a `{X}` in its pattern | error | `route prop 'x' is not declared: not an attribute of the route root and not a param of '/a/{y}'` |
| R12 | `dangerouslySetInnerHTML` fed a route param | error | `route params must not be injected as raw HTML` |
| R13 | unsupported pattern form | error | `unsupported route pattern '…'` |
| R14 | route body references `$__uni_*` internals | error | `route bodies cannot call runtime internals` |

### 14.9 Testing decisions

**D-9.1 — Deep-link SSR is a `--libs` server test, not a WebView test.**
The WebView harness loads pages from opaque origins with no real navigation
(§12.5), so deep-link *server selection* is tested by rendering: build a page,
`set_route_url("/projects/42")`, `#html { <App/> }`, then assert the rendered
HTML has the active wrapper and the param value. This is a stronger SSR test than
a browser check would be, and it avoids the harness's limitations entirely.

**D-9.2 — Client matching is tested by calling the API.** In WebView, call
`activateRouteByUrl("/projects/42")` directly; history integration gets its own
`isolate` test that asserts the in-memory fallback (since `pushState` throws).
Never drive a test through real history.

**D-9.3 — Invariants are read from `window.$__uni_routers`.** Because
`record.visible` and `record.hydrated` are maintained fields (D-7.2/D-7.3), the
pairing invariant and the "only the default route hydrated at load" assertion are
direct property reads — no DOM introspection, no mocking.

**D-9.4 — Timing budgets are reports, not hard gates, at first.** The WebView
harness is noisy; §7's ≤1ms/≤50ms numbers are logged and tracked, and promoted
to CI gates only once they are stable across runs. Correctness gates ship first.

### 14.10 Deferred, with triggers

| Question | Resolve when | Default until then |
|---|---|---|
| `content-visibility` vs `display:none` | after Phase 2 profiling | `display:none` (§2.1) |
| Route transition animations | first real request | none; hook queue is the seam |
| `router.release(id)` shape | shipped in v1 | settled: inactive-only, refused under an in-flight fetch (§15.2, §13.3.1) |
| Per-route `<meta>`/head diff | first request for per-route meta | `<title>` only (§13.2) |
| Nested URL ownership of parked inner state | Phase 6 | outer owns prefix; re-derive inner from URL (§13.3) |
| Back/forward on opaque origins | a real embedded target needs it | in-memory URL (§6.2) |
| Wildcard/optional URL segments | first real need | not supported (D-6.1) |
| Per-route fragment endpoint syntax (`route remote … fragment "…"`) | first real need | `fetchUrl` is always `null` in v1; the page-wide `$__uni_router_fragment_url` resolver supplies the URL (§15.9-Q27) |
| Streaming / Suspense loaders | architectural | non-goal (§10) |

### 14.11 First implementation steps (Phase 0 → 1)

Ordered so each step is independently verifiable:

1. **Confirm the two lexer/parser facts** by writing the smallest possible
   `#universal` fixture that contains a `route #"x"` — first with the `Hash`
   token added, then parsed by `universal_parser` (plugin test in
   `lang/tests/compiler_plugins/universal/`). This validates D-1.1/D-1.2 before
   anything else is built on them.
2. **Phase 1 store**: `PageParameter`, the five `HtmlPage` methods + route-status
   flag, `RouteRequest`, `set_route_url`/`get_request` extensions; `--libs` unit
   tests (roundtrip, reserved-key assertion, view lifetime).
3. **Phase 2 runtime**: the registry, `$__uni_route_register`, `$__uni_activate`
   (with the FIFO queue), `$__uni_router_error`, `$__uni_route_visible`,
   `$__uni_route_props`, the head-CSS rule — the exact source in §15.2, exercised
   by hand-written JS in a `#js` block in a WebView fixture.
4. **Then Phase 3** (D-4.3 file order: enum → lexer → parser → converter), with
   §14.8's diagnostics landing alongside each construct.

The order matters: steps 1–3 add no user-visible syntax, so a mistake in the
lexer/parser decision costs nothing to correct before 14.2–14.3 are built on it.

---

## 15. Final implementation blueprint (build-ready)

§13 made the design *match the runtime*. §14 froze the *decisions*. This section is
the part that stops the implementation from drifting: the exact files and symbols
per phase (§15.1), the authoritative runtime source (§15.2), the byte-exact
emitted artifacts (§15.3), the compiler-side skeletons (§15.4), the register of
invariants each with the test that asserts it (§15.5), how each performance budget
is actually measured (§15.6), the per-phase production gate (§15.7), how to land
and undo it safely (§15.8), and the quality fixes this pass applied to the earlier
sections (§15.9).

**Precedence rule (read this first).** Where §15 and an earlier section disagree,
**§15 wins** and the earlier section is a stale narrative copy. §15.2 is the single
normative copy of the runtime JS; §2.1 reproduces the activation part for
narrative continuity only. If they ever drift, regenerate §2.1's block from §15.2
— never the other way round. Same rule for §14.8 (diagnostics) and §15.3 (emitted
artifacts).

### 15.1 Deliverable inventory — exact files, symbols, tests, commands

Nothing is "roughly in the router library". Every phase below names every file it
touches, the symbols it adds, the tests it must add, and the command that proves
it. A phase is **not done** until its Exit gate line is true.

#### Phase 0 — schema & emission prerequisites (hours, not days)

| Kind | Item |
|---|---|
| Modify | `lang/libs/js_syntax/src/TokenType.ch` — append `Hash` (tail only) |
| Modify | `lang/libs/js_syntax/src/Tokenizer.ch` — `case '#':` → `Hash` |
| Modify | `lang/libs/js_cbi_lexer/src/CompilerLexer.ch` — same `case '#'` in `nextJsToken` |
| Modify | `lang/libs/js_syntax/src/NodeKind.ch` — append `RouterDecl`, `RouteDecl`, `RouteHook` |
| Modify | `lang/libs/js_syntax/src/Ast.ch` — the three structs of D-2.1 |
| Modify | `lang/libs/universal_parser/src/parser/parser_stmt.ch` — one call to `tryParseRouterStatement` |
| Add | `lang/libs/universal_parser/src/parser/parser_router.ch` |
| Test | `lang/tests/compiler_plugins/universal/src/router_parse.ch` — a fixture whose body contains `router "m" { route #"a" { <A/> } }`; asserts the three nodes exist with the expected fields |
| Command | `./scripts/test.sh --tcc --plugins` (no `--cached-plugins`) |
| Exit gate | the fixture parses; a `route #` inside a body is not `Token{0,"unexpected"}`; the three existing suites still pass |

#### Phase 1 — parameter store (no compiler change)

| Kind | Item |
|---|---|
| Add | `lang/libs/router/chemical.mod` (`module router`, `source "src"`, deps per D-4.1) |
| Add | `lang/libs/router/src/params.ch` — `PageParameter`, `get_parameter_object<T>`, `query_param`, `parse_query` |
| Add | `lang/libs/router/src/request.ch` — `RouteRequest`, `(page) set_request`, `get_request` |
| Add | `lang/libs/router/src/url.ch` — `(page) set_route_url`, `(page) get_route_url`, `(page) get_route_base` |
| Modify | `lang/libs/page/src/page.ch` — `parameters` field, `add_parameter`, `add_parameter_object`, `get_parameter`, `has_parameter`, `route_missing_flag`, `mark_route_missing`, `route_missing`, plus the manifest accumulator `route_manifest` + `add_route_pattern` + `route_manifest_json` (§13.2.3 — the table has to be collected somewhere) |
| Add | `lang/tests/libs/router/src/store.ch`, `.../query.ch` (`--libs` unit tests) |
| Command | `./scripts/test.sh --tcc --libs` and `./scripts/test.sh --tcc` |
| Exit gate | store round-trips all three variants; `has_parameter`/`get_parameter` miss paths return empty/`false` without touching memory; the reserved-key assertion fires in debug; **a page that never calls a router API emits byte-identical output to today** (§15.7 G-2) |

#### Phase 2 — runtime core (client, no compiler change)

| Kind | Item |
|---|---|
| Modify | `lang/libs/page/src/page.ch` — the **CORE** group of §15.2 + the one head-CSS rule |
| Add | `lang/tests/universal_webview/src/router_runtime.ut.ch` (hand-built registry, raw JS) |
| Command | `./scripts/test.sh --tcc --universal` |
| Exit gate | activate→hydrate→show; re-activate is a no-op with zero subscriber runs; unknown id is a contained log; the FIFO queue applies a hook-initiated navigation after the in-flight one; INV-1/INV-2/INV-3/INV-4 green |

#### Phase 3 — syntax + codegen

| Kind | Item |
|---|---|
| Add | `lang/libs/universal_cbi/src/router/emit.ch` — `emit_router_server`, `emit_route_registration`, `emit_router_table` (D-2.3) |
| Modify | `lang/libs/universal_cbi/src/react/ast_replace.ch` — call the three emitters from component-body emission |
| Modify | `lang/libs/html_cbi/src/converter/language/main.ch` — expose the props serializer used by `emit_universal_queue` as one shared function (D-2.6) |
| Modify | converter validation — R1–R7, R9, R13 (D-1.3, §14.8) |
| Add | `lang/tests/negative/src/router_diagnostics.ch` — one case per diagnostic message string |
| Add | `lang/tests/universal_webview/src/router_navigation.ut.ch`, `router_hydration.ut.ch`, `router_mount_mode.ut.ch`, `router_only_default_hydrated.ut.ch` |
| Command | `./scripts/test.sh --tcc --plugins`, `--tcc --negative`, `--tcc --universal`, then `--tcc --llvm` for backend parity |
| Exit gate | the §4.1 example compiles on **both** backends; every R* message matches §14.8 **verbatim**; `router_mount_mode.ut.ch` proves a `<section>`-rooted route hides and shows (regression for §2.1 note 2) |

#### Phase 4 — control API, modes, hooks, reactive state

| Kind | Item |
|---|---|
| Modify | §15.2 runtime — CONTROL group: `$__uni_router` accessor, method table, `$__uni_preload`, `$__uni_release`, `$__uni_should_intercept` |
| Modify | `lang/libs/universal_cbi/src/router/emit.ch` — `default`/`mode`/`title` emission, hook registration, initial activation tail (D-7.4) |
| Add | `lang/libs/router/src/Link.ch`, `lang/libs/router/src/NavLink.ch` |
| Modify | converter — R8, R10–R12, R14 |
| Add | `router_hooks.ut.ch`, `router_state.ut.ch`, `router_release.ut.ch`, `router_title.ut.ch`, `router_error_no_throw.ut.ch` |
| Command | `./scripts/test.sh --tcc --universal && ./scripts/test.sh --tcc --libs` |
| Exit gate | guard denial keeps the previous route visible and the page usable; `$current`/`$url` re-render exactly the right links (§15.5 INV-11); `release` on an inactive route clears DOM and re-arms; budgets of §15.6 report within range |

#### Phase 5 — URL layer

| Kind | Item |
|---|---|
| Add | `lang/libs/router/src/match.ch` — `RoutePattern`, `RouteMatch`, `match_route`, `normalize_path` (pure, shared by server and compiler-side emission) |
| Add | `lang/libs/router/src/build_path.ch` — `build_path(pattern, params)` |
| Modify | §15.2 runtime — URL group: `$__uni_match_url` (over `r.table`), `$__uni_set_url`, `$__uni_activate_initial`, `$__uni_sync_url` + popstate wiring, `$__uni_build_path`, `$__uni_activate_by_url`, `$__uni_fetch_route`/`$__uni_mount_fragment` (remote), the query helpers and the `rawUrl` branches |
| Modify | `lang/libs/page/src/page.ch` `writeToDirectory` — also write `<name>.routes.json` (§13.2.3) |
| Modify | `lang/libs/universal_cbi/src/router/emit.ch` — emit the table **in precedence order** (D-6.9) |
| Add | `router_url.ut.ch`, `router_url_history.ut.ch`, `router_link_modifiers.ut.ch`, `router_guard_back.ut.ch`, `router_param_remount.ut.ch` |
| Add | `lang/tests/libs/router/src/match.ch`, `.../manifest.ch`, `.../ssr_deeplink.ch` |
| Command | `./scripts/test.sh --tcc --universal && ./scripts/test.sh --tcc --libs` |
| Exit gate | deep link = client nav = popstate produce byte-identical props (G-6); a denied popstate re-syncs the URL; a static-export page's manifest round-trips |

#### Phase 6 — nested routes / `<Outlet />`

| Kind | Item |
|---|---|
| Modify | `lang/libs/universal_parser/src/parser/parser_router.ch` — `route` inside a `route` body |
| Modify | `lang/libs/universal_cbi/src/router/emit.ch` — anonymous nested router per parent route, segment-prefix matching |
| Add | `lang/libs/router/src/Outlet.ch` |
| Add | `router_nested.ut.ch` + a `--libs` nested-deep-link render test |
| Exit gate | layout state survives a child switch; removing/re-adding a child does not remount the parent; parked-inner URL ownership follows §13.3.3 |

#### Phase 7 — server snapshot cache (independent; any time after Phase 3)

| Kind | Item |
|---|---|
| Modify | `lang/libs/universal_cbi/src/router/emit.ch` — static/dynamic classification reuse + once-guarded snapshot build |
| Modify | `lang/libs/page/src/page.ch` — debug-mode bypass flag, render counters (debug builds only) |
| Add | `lang/tests/libs/router/src/snapshot.ch` — cold-vs-warm byte equality, id stability (D-2.5), 8-thread concurrent render, dynamic-route exclusion |
| Command | `./scripts/test.sh --tcc --libs -j 8` |
| Exit gate | warm render byte-identical to cold; a route reading request data is classified dynamic; the concurrent run matches the single-threaded one byte-for-byte; §15.6's server budget reported |

### 15.2 The frozen runtime source (normative)

This is the whole client runtime. It is emitted **only** when the page declares a
router (§0.5), in `pageJs` (segmented component-definitions section), once per
page. Order inside the block matters: helpers → registry + queue → activation →
accessor → URL layer; the initial `$__uni_activate(...)` call is appended to
`pageJsEnd` after `$__universal_flush()` (D-7.4).

It is written as one block for reviewability but ships in three independently
landable groups — do not build a later phase's group early, and do not ship a
later group's symbols in an earlier phase:

| Group | Symbols | Lands in |
|---|---|---|
| **CORE** | `$__uni_router_version`, `$__uni_router_error`, registry + queue + `busy`, `$__uni_route_visible`, `$__uni_route_register`, `$__uni_route_props`, `$__uni_HISTORY_*`, `$__uni_norm_path`, `$__uni_base_title`, `$__uni_activate`, `$__uni_activate_now` (id routes only) | Phase 2 |
| **CONTROL** | `$__uni_preload`, `$__uni_release`, `$__uni_router`, `$__uni_router_null`, `$__uni_router_methods`, `$__uni_should_intercept`, the `deactivate()` title/query reset (the `title` write inside `activate_now` is inert until a route declares one, so it stays in CORE) | Phase 4 |
| **URL** | `$__uni_decode_segment`, `$__uni_parse_query`, `$__uni_query_sig`, `$__uni_initial_url`, `$__uni_router_fragment_url`, `$__uni_url_mem`/`history_ok`, `$__uni_set_url`, `$__uni_match_url`, `$__uni_activate_by_url`, `$__uni_activate_initial`, `$__uni_fetch_route`, `$__uni_mount_fragment`, `$__uni_set_query`, `$__uni_set_table_base`, `$__uni_build_path`, `$__uni_sync_url`, the `rawUrl`/query branches | Phase 5 (plus `remote` in Phase 5) |

A phase that lands CORE only must not emit the `rawUrl`/query/remote branches of
`$__uni_activate_now`; they are inert without the URL group but are dead bytes
(INV-18's spirit: every byte is paid for by a page that uses it).

Names are frozen: changing any of them is a design change, not an implementation
detail. `window.$__uni_router_version = 1` is emitted with the block so a
future runtime can detect a page built by an older macro.

```js
// ─────────────────────────────────────────────────────────────────────────────
// Chemical universal router runtime — v1.  Emitted once per page that declares a
// router.  No DOM queries in the activation path.  Nothing here throws.
// ─────────────────────────────────────────────────────────────────────────────
window.$__uni_router_version = 1;

window.$__uni_router_error = ((message, details = "") => {
    // Recoverable faults only.  console.error never throws, so a failed
    // navigation can never abort a click handler or the hydration flush.
    console.error("[router] " + message + (details ? ": " + details : ""));
});

window.$__uni_routers = window.$__uni_routers || Object.create(null);
window.$__uni_router_queue = window.$__uni_router_queue || [];
window.$__uni_router_busy = window.$__uni_router_busy || false;

// The only visibility mutator.  Total: never throws, and it is where focus and
// scroll are captured/restored (§6.7, D-7.3).
window.$__uni_route_visible = ((route, visible) => {
    route.visible = visible;
    if(!route.el) {                                     // D-7.7
        window.$__uni_router_error("route wrapper element missing", route.key);
        return;
    }
    if(!visible) {
        route.scrollY = window.scrollY;
        const a = document.activeElement;
        if(a && a !== document.body && route.el.contains(a)) route.focusEl = a;
    }
    route.el.setAttribute("data-uni-route-active", visible ? "true" : "false");
    if(visible) {
        if(!route.noscroll) window.scrollTo(0, route.scrollY || 0);
        const f = (route.focusEl && route.el.contains(route.focusEl))
            ? route.focusEl : route.el;
        try { f.focus({ preventScroll: true }); } catch(_) {}
    }
});

// Percent-decoding of a matched segment or a query key/value.  decodeURIComponent
// throws URIError on malformed input (a bare "%"), and INV-3 says nothing here
// throws — so a bad segment degrades to its raw text instead of aborting the
// navigation.
//
// NOTE: this is *decoding*, not JS-string escaping (the earlier name
// `$__uni_js_escape` conflated the two).  Client-side params are runtime JS
// values, so they need no string escaping at all; the single-quote/backslash
// escaping contract applies only to props the *server* serializes into generated
// JS (§13.3.5, INV-9, §15.9-Q21).
window.$__uni_decode_segment = ((s) => {
    try { return decodeURIComponent(s); } catch(_) { return s; }
});

// The query object of the current URL, held in a signal (§6.5).  Built from the
// raw URL on every activation, so popstate and Link clicks populate it exactly
// like setQuery does — the earlier design only ever wrote it from setQuery, so a
// deep link or a popstate left `r.query()` stale (§15.9-Q24).
window.$__uni_parse_query = ((s) => {
    const out = Object.create(null);
    const i = s ? s.indexOf("?") : -1;
    if(i < 0) return out;
    const parts = s.slice(i + 1).split("&");
    for(let k = 0; k < parts.length; k++) {
        const p = parts[k];
        if(!p) continue;
        const eq = p.indexOf("=");
        out[window.$__uni_decode_segment(eq < 0 ? p : p.slice(0, eq))] =
            eq < 0 ? "" : window.$__uni_decode_segment(p.slice(eq + 1));   // last wins (§6.5)
    }
    return out;
});

// INV-11 requires a signal to be written only when its value actually changes,
// and query objects are new objects every time — so compare signatures, not
// identities, before writing `$query`.
window.$__uni_query_sig = ((q) => {
    if(!q) return "";
    let s = "";
    for(const k in q) s += k + "=" + q[k] + "&";
    return s;
});

// The URL the page was actually loaded with (deep-link support, §15.3 template 5).
// On an opaque origin (data:/blob:) `location.pathname` is not a usable path, so
// anything that does not start with "/" becomes the root.
window.$__uni_initial_url = (() => {
    let p = "/";
    try { p = location.pathname + location.search; } catch(_) {}
    if(!p || p.charCodeAt(0) !== 47) p = "/";
    return p;
});

// The document title a route without `title` falls back to (§13.2.1).  Without
// this, navigating from a titled route to an untitled one leaves the previous
// route's title in the tab (§15.9-Q25).
window.$__uni_base_title = (window.$__uni_base_title === undefined)
    ? document.title : window.$__uni_base_title;

// Bootstrap-only: resolves the wrapper and its boundary span ONCE per route
// (§15.6 R-2).  Everything the activation path needs is captured here.
window.$__uni_route_register = ((routerName, routeId, spec) => {
    const r = window.$__uni_routers[routerName];
    if(!r) { window.$__uni_router_error("route registered for unknown router", routerName); return; }
    // By id, not by `[data-uni-route="…"]`: a URL pattern can legitimately contain
    // characters that a CSS attribute selector would need escaped (`/`, `{`, `}`,
    // and a pattern is user text), and `getElementById` is both cheaper and total
    // (§15.9-Q32).  The wrapper id uses the same source-location scheme as the
    // boundary id (D-2.5), so it is stable across renders and snapshot-safe.
    const el = spec.wrapperId ? document.getElementById(spec.wrapperId) : null;
    const host = window.$__uni_boundary(spec.hostId);
    if(el && host && el === host)                    // INV-5: never mount the wrapper
        window.$__uni_router_error("route boundary resolves to the wrapper", spec.key);
    r.routes[routeId] = {
        id: routeId, key: spec.key, el: el, host: host,
        comp: spec.comp, baseProps: spec.baseProps, params: null,
        hydrated: false, visible: false, failed: false,
        ssr: !spec.remote, remote: !!spec.remote, isUrl: !!spec.isUrl,
        url: null, rawUrl: null, inst: null, inFlight: null,       // null | 1 while fetching
        fragment: null, pendingActivate: null, fetchUrl: spec.fetchUrl || null,
        scrollY: 0, focusEl: null, noscroll: !!spec.noscroll, title: spec.title || null,
        beforeActivate: spec.beforeActivate || null,
        onActivate: spec.onActivate || null,
        onDeactivate: spec.onDeactivate || null
    };
});

// Immutable compile-time props + current params, merged at each mount (§6.4).
window.$__uni_route_props = ((route) => {
    if(!route.params) return route.baseProps;
    const out = Object.create(null);
    for(const k in route.baseProps) out[k] = route.baseProps[k];
    for(const k in route.params) out[k] = route.params[k];   // pre-escaped (§13.3.5)
    return out;
});

// Synchronous entry.  A navigation requested from inside a hook is queued and
// applied after the in-flight one — never inline (D-7.1).
// historyMode is explicit and numeric (cheap to compare, no string work in the
// hot path):  0 = do not touch history,  1 = pushState,  2 = replaceState.
// popstate and the initial activation use 0; a Link click uses 1; replaceRoute 2.
window.$__uni_HISTORY_NONE    = 0;
window.$__uni_HISTORY_PUSH    = 1;
window.$__uni_HISTORY_REPLACE = 2;

// `url` is the NORMALIZED path (the record's identity); `rawUrl` is the URL as
// requested, query included, and is what gets written to history.  They are
// separate because matching and active-state comparison must ignore the query
// while the browser URL must keep it (§15.9-Q22).
window.$__uni_activate = ((routerName, routeId, url, params, historyMode, rawUrl) => {
    if(window.$__uni_router_busy) {
        window.$__uni_router_queue.push([routerName, routeId, url, params, historyMode, rawUrl]);
        return true;
    }
    window.$__uni_router_busy = true;
    const ok = window.$__uni_activate_now(routerName, routeId, url, params, historyMode, rawUrl);
    window.$__uni_router_busy = false;
    while(window.$__uni_router_queue.length) {
        const q = window.$__uni_router_queue.shift();
        window.$__uni_router_busy = true;
        window.$__uni_activate_now(q[0], q[1], q[2], q[3], q[4], q[5]);
        window.$__uni_router_busy = false;
    }
    return ok;
});

window.$__uni_activate_now = ((routerName, routeId, url, params, historyMode, raw) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) {                                        // unknown id: contained
        window.$__uni_router_error("activateRoute: unknown route", routerName + "#" + routeId);
        return false;
    }
    if(route.failed) {                                  // D-7.6: no retry storm
        window.$__uni_router_error("activateRoute: route failed earlier", route.key);
        return false;
    }
    url = (url === undefined) ? route.url : url;
    // Defaulting `raw` to the record's own rawUrl (when the path is unchanged) is
    // what makes `deactivate()` then `activateRoute(id)` restore the full URL the
    // route had, query included, instead of silently dropping it (§15.9-Q33).
    raw = (raw === undefined)
        ? ((url === route.url) ? route.rawUrl : url)
        : raw;                                          // id routes: both are null
    // O(1) no-op — compared on BOTH the normalized path and the raw URL, because
    // a same-path navigation with a different query (?tab=1 -> ?tab=2) is a real
    // navigation that must update the query signal and the history entry
    // (§15.9-Q22).  For id routes both are null, so this is the original compare.
    if(r.currentRoute === route && route.url === url && route.rawUrl === raw) return true;
    // Param change (§6.4): the record was rendered for a DIFFERENT normalized path,
    // so its SSR'd props and mounted instance belong to the old params.
    //   - The guard is `route.url !== url` and it must NOT require the record to be
    //     the *current* one: a route that was visited, hidden, and is now re-entered
    //     with a new path is still mounted with stale content (§15.9-Q30).
    //   - It must not fire for a same-path navigation with a different query
    //     (`?tab=1 -> ?tab=2`), which is not a param change at all (§15.9-Q22).
    //   - It must not fire on first activation, where `hydrated` is false: there is
    //     nothing to dispose and the fresh path already applies.
    if(route.url !== url && (route.hydrated || route.inst)) {
        if(route.inst) window.$__uni_dispose(route.inst);
        route.inst = null;
        route.hydrated = false;
        route.ssr = false;
        route.fragment = null;                          // belongs to the old url
    }
    if(route.beforeActivate && route.beforeActivate(url) === false) {
        window.$__uni_router_error("navigation cancelled", route.key);
        return false;                                   // nothing mutated yet
    }
    // Remote routes: no SSR markup exists, so they can be neither mounted nor made
    // visible synchronously.  If the fragment is absent, fetch and come back —
    // nothing is mutated here, so the previous route stays on screen (INV-15/INV-21).
    if(route.remote) {
        if(!route.fragment) {
            route.pendingActivate = [url, params, historyMode, raw];
            if(!route.inFlight) window.$__uni_fetch_route(routerName, routeId, false);
            return true;                                // accepted (asynchronous)
        }
        // The fragment is already held (a prefetch, or a fetch from before a
        // `release`), but its DOM may not be in the host.  Adopt it now; without
        // this branch a released or prefetched remote route would mount its
        // component against an empty host (§15.9-Q23).
        // `!route.host` must be part of the test: `$__uni_mount_fragment` contains
        // that case and marks the record failed, but reading `.firstChild` here
        // would throw first (§15.9-Q31).
        if(!route.host || !route.ssr || !route.host.firstChild) {
            if(!window.$__uni_mount_fragment(routerName, routeId)) return false;
        }
    }
    route.url = url;                                    // before mount: props derive from it
    route.rawUrl = raw;
    route.params = params || null;
    // Hydrate first: a mount failure must not leave zero routes visible (D-7.10a).
    if(!route.hydrated) {
        if(route.comp) {                                // §2.4: static routes have none
            // A missing host (an SSR/JS mismatch) must be a contained failure, not a
            // TypeError out of a click handler — the DOM access below is outside the
            // mount try/catch, and $__uni_route_visible being total (D-7.7) is not
            // enough on its own (§15.9-Q31).
            if(!route.host) {
                route.failed = true;
                window.$__uni_router_error("route has no mount host", route.key);
                return false;                           // previous route still visible
            }
            if(!route.ssr) {                            // $__uni_dispose leaves DOM behind
                while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
            }
            try {
                window.$__uni_mount(route.host, route.comp,
                                    window.$__uni_route_props(route), "children");
                route.inst = route.host.$__uni_instance;
            } catch(err) {
                route.hydrated = true;                  // do not retry every switch
                window.$__uni_router_error("route mount failed", route.key + " " + err);
                return false;                           // previous route still visible
            }
        }
        route.hydrated = true;
    }
    if(r.currentRoute) {
        if(r.currentRoute.onDeactivate) r.currentRoute.onDeactivate();
        window.$__uni_route_visible(r.currentRoute, false);
    }
    window.$__uni_route_visible(route, true);
    r.currentRoute = route;
    if(r.$current.value !== routeId) r.$current.value = routeId;   // D-7.8
    if(r.$url && r.$url.value !== url) r.$url.value = url;
    // The URL is written by the ONE place that owns navigation, not by callers,
    // so push/replace can never be applied twice or lost (D-6.10).  It writes the
    // RAW url, so a query survives the round trip (§15.9-Q22).
    if(route.isUrl && historyMode !== window.$__uni_HISTORY_NONE) {
        window.$__uni_set_url(raw, historyMode === window.$__uni_HISTORY_REPLACE);
    }
    if(r.$query && route.isUrl) {
        const q = window.$__uni_parse_query(raw);
        if(window.$__uni_query_sig(r.$query.value) !== window.$__uni_query_sig(q)) {
            r.$query.value = q;
        }
    }
    // Falls back to the page's own title for routes that declare none (§15.9-Q25).
    try { document.title = route.title || window.$__uni_base_title; } catch(_) {}
    if(route.onActivate) {
        try { route.onActivate(url); }
        catch(err) { window.$__uni_router_error("onActivate failed", route.key + " " + err); }
    }
    return true;
});

// preload = hydrate-minus-visibility (D-7.5).  Idempotent; writes no signal.
window.$__uni_preload = ((routerName, routeId) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) { window.$__uni_router_error("preload: unknown route", routerName + "#" + routeId); return false; }
    if(route.failed) return false;                      // do not report success for a dead route
    if(route.hydrated || route.inFlight) return true;   // once per route
    if(route.remote) {
        if(!route.fragment) return window.$__uni_fetch_route(routerName, routeId, true);
        if(!route.host || !route.ssr || !route.host.firstChild) {   // cached fragment: adopt, never refetch
            if(!window.$__uni_mount_fragment(routerName, routeId)) return false;
        }
    }
    if(route.comp) {
        if(!route.host) {                               // same containment rule (Q31)
            route.failed = true;
            window.$__uni_router_error("preload: route has no mount host", route.key);
            return false;
        }
        if(!route.ssr) { while(route.host.firstChild) route.host.removeChild(route.host.firstChild); }
        try {
            window.$__uni_mount(route.host, route.comp,
                                window.$__uni_route_props(route), "children");
            route.inst = route.host.$__uni_instance;
        } catch(err) {
            route.hydrated = true;
            window.$__uni_router_error("preload mount failed", route.key + " " + err);
            return false;
        }
    }
    route.hydrated = true;
    return true;
});

// release = dispose an INACTIVE route's instance and drop its DOM, re-arming it
// for a fresh mount (§13.3.1).  Releasing the active route is a contained no-op.
window.$__uni_release = ((routerName, routeId) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) { window.$__uni_router_error("release: unknown route", routerName + "#" + routeId); return false; }
    if(r.currentRoute === route) {
        window.$__uni_router_error("release: cannot release the active route", route.key);
        return false;
    }
    if(route.inFlight) {                                // never dispose under a fetch
        window.$__uni_router_error("release: route is loading", route.key);
        return false;
    }
    if(route.inst) window.$__uni_dispose(route.inst);
    route.inst = null;
    route.hydrated = false;
    route.failed = false;
    route.params = null;
    route.ssr = false;
    if(route.host) { while(route.host.firstChild) route.host.removeChild(route.host.firstChild); }
    return true;
});

// Link's click policy (§6.3) as ONE predicate, so <Link> and user markup agree on
// what is an in-page navigation.  Called from JSX as
// `window.$__uni_should_intercept(e, props.href)` — the components library's
// established way of reaching runtime helpers from a `#universal` body
// (`window.$__uni_floating(...)` in Surface.ch, `window.$__uni_inert_scan()` in
// Sheet.ch).  A bare `router_should_intercept(...)` in JSX would be undefined.
window.$__uni_should_intercept = ((e, href) => {
    if(!e || e.defaultPrevented) return false;
    // Modified clicks and non-left buttons belong to the browser (new tab/window).
    if(e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return false;
    if(!href || href.charCodeAt(0) !== 47) return false;    // "http:", "mailto:", "#frag"
    if(href.charCodeAt(1) === 47) return false;             // "//host": protocol-relative
    const t = e.currentTarget;
    if(t && (t.target || t.hasAttribute("download"))) return false;
    return true;
});

// ── the public handle.  The record IS the handle; methods are own properties.
window.$__uni_router = ((name) => {
    const r = window.$__uni_routers[name];
    if(!r) {
        // A component that renders a <Link> on a page whose router component was
        // never rendered gets a null-object, not a TypeError (§15.9-Q4).
        window.$__uni_router_error("router not present on this page", name);
        return window.$__uni_router_null(name);
    }
    return r;
});

window.$__uni_router_null = ((name) => ({
    name: name, currentRoute: null,
    $current: window.$_us(null), $url: window.$_us(null), $query: window.$_us(null),
    routes: Object.create(null), table: null,
    activateRoute: (() => false), activateRouteByUrl: (() => false),
    replaceRoute: (() => false), replaceRouteByUrl: (() => false),
    deactivate: (() => {}), preload: (() => false),
    release: (() => false), current: (() => null), currentUrl: (() => null),
    isActive: (() => false), buildPath: (() => null), normPath: window.$__uni_norm_path,
    query: (() => null), setQuery: (() => {})
}));

// ── method table, installed on each real record at bootstrap (§15.3 template 1)
window.$__uni_router_methods = ((name) => ({
    activateRoute:     ((id, params) => window.$__uni_activate(name, id, undefined, params, window.$__uni_HISTORY_PUSH)),
    activateRouteByUrl:((path) => window.$__uni_activate_by_url(name, path, false)),
    replaceRoute:      ((id, params) => window.$__uni_activate(name, id, undefined, params, window.$__uni_HISTORY_REPLACE)),
    replaceRouteByUrl: ((path) => window.$__uni_activate_by_url(name, path, true)),
    deactivate:        (() => {
        const r = window.$__uni_routers[name];
        if(!r.currentRoute) return;
        if(r.currentRoute.onDeactivate) r.currentRoute.onDeactivate();
        window.$__uni_route_visible(r.currentRoute, false);
        r.currentRoute = null;
        if(r.$current.value !== null) r.$current.value = null;
        if(r.$url.value !== null) r.$url.value = null;
        if(r.$query.value !== null) r.$query.value = null;
        try { document.title = window.$__uni_base_title; } catch(_) {}
    }),
    preload:  ((id) => window.$__uni_preload(name, id)),
    release:  ((id) => window.$__uni_release(name, id)),
    current:  (() => { const r = window.$__uni_routers[name]; return r.currentRoute ? r.currentRoute.id : null; }),
    currentUrl:(() => { const r = window.$__uni_routers[name]; return r.currentRoute ? r.currentRoute.url : null; }),
    isActive: ((id) => window.$__uni_routers[name].$current.value === id),
    buildPath:((id, params) => window.$__uni_build_path(name, id, params)),
    // Public normalization so user JSX can compare a `href` to `$url.value`
    // without re-implementing the rule (§6.6, §15.9-Q26).
    normPath: ((p) => window.$__uni_norm_path(p)),
    query:    (() => window.$__uni_routers[name].$query.value),
    setQuery: ((obj) => window.$__uni_set_query(name, obj))
}));

// ── URL layer.  Emitted only when the page declares a URL route.
// `$__uni_set_table_base` belongs here for the same reason: it patches `r.table`.
// Where a `remote` route's HTML comes from.  The endpoint is application code
// (§13.2.4); this is the default convention and it is overridable, so an app that
// mounts its fragment endpoint elsewhere still works.  A 404 or a network error is
// a contained failure: the previous route stays visible and the next activation
// retries (§15.9-Q27).
// Override by assigning this global in a script that runs before the page's own
// script, or per route by setting `route.fetchUrl` (§14.10 reserves the syntax).
window.$__uni_router_fragment_url = window.$__uni_router_fragment_url ||
    ((name, id) => "/__uni_fragment?router=" + encodeURIComponent(name) +
                   "&id=" + encodeURIComponent(id));

// A mount prefix ("the app is served under /app") is known to the SERVER, which
// passes it to set_route_url, but the client table is emitted at compile time and
// cannot contain it — so the generated server function writes it back into the
// client table before the activation tail runs (§15.9-Q42).  Default is "".
window.$__uni_set_table_base = ((name, base) => {
    const r = window.$__uni_routers[name];
    if(r && r.table) r.table.base = base || "";
});

window.$__uni_url_mem = window.$__uni_url_mem || { path: "/" };
if(window.$__uni_history_ok === undefined) {
    try { history.replaceState(history.state, "", location.href); window.$__uni_history_ok = true; }
    catch(_) { window.$__uni_history_ok = false; }      // opaque origin (§12.5)
}

window.$__uni_norm_path = ((p) => {
    if(!p) return "/";
    const q = p.indexOf("?"); if(q >= 0) p = p.slice(0, q);
    if(p.length > 1 && p.charCodeAt(p.length - 1) === 47) p = p.slice(0, -1);
    return p || "/";
});

window.$__uni_set_url = ((path, replace) => {
    window.$__uni_url_mem.path = path;
    if(!window.$__uni_history_ok) return;               // degrade, never throw
    try { replace ? history.replaceState(null, "", path) : history.pushState(null, "", path); }
    catch(_) { window.$__uni_history_ok = false; }
});

// First-match scan; the emitted table is ALREADY in precedence order (D-6.9),
// so the runtime never sorts or scores.
window.$__uni_match_url = ((name, path) => {
    const r = window.$__uni_routers[name];
    const t = r && r.table;
    if(!t) return null;
    let p = window.$__uni_norm_path(path);
    if(t.base && p.indexOf(t.base) === 0) p = p.slice(t.base.length) || "/";
    const segs = p.split("/");
    let n = 0;
    for(let i = 0; i < segs.length; i++) if(segs[i]) segs[n++] = segs[i];
    segs.length = n;
    for(let i = 0; i < t.routes.length; i++) {
        const e = t.routes[i];
        if(e.fallback) return { id: e.id, fallback: true, params: null };
        if(e.pattern.length !== segs.length) continue;
        let params = null, ok = true;
        for(let j = 0; j < e.pattern.length; j++) {
            const s = e.pattern[j];
            if(s.charCodeAt(0) === 123) {               // "{name}"
                if(!params) params = Object.create(null);
                params[s.slice(1, -1)] = window.$__uni_decode_segment(segs[j]);
            } else if(s !== segs[j]) { ok = false; break; }
        }
        if(ok) return { id: e.id, fallback: false, params: params };
    }
    return null;
});

window.$__uni_activate_by_url = ((name, path, replace) => {
    const m = window.$__uni_match_url(name, path);
    if(!m) {
        window.$__uni_router_error("no route matches", name + " " + path);
        return false;
    }
    // `path` is passed raw as well, so a query on the clicked href is preserved on
    // the history entry while matching still sees only the path (§15.9-Q22).
    return window.$__uni_activate(name, m.id, window.$__uni_norm_path(path), m.params,
        replace ? window.$__uni_HISTORY_REPLACE : window.$__uni_HISTORY_PUSH, path);
});

// remote routes (§6.7).  The builder endpoint is user code; the fetch contract is
// fixed: the response body must be the route's SSR fragment (boundary span
// included), and the endpoint must validate `name`+`id` against its own
// declarations and run the same auth checks as the page handler (§13.2.4).
window.$__uni_fetch_route = ((name, routeId, prefetchOnly) => {
    const r = window.$__uni_routers[name];
    const route = r && r.routes[routeId];
    if(!route) return false;
    if(route.failed) return false;                                     // not a success (Q37)
    if(route.hydrated || route.inFlight) return true;                  // collapse double-fetch
    // fetchUrl (declared) wins; otherwise the page-wide convention resolver, so a
    // `remote` route can actually load instead of failing on a null URL (§15.9-Q27).
    const url = route.fetchUrl || window.$__uni_router_fragment_url(name, routeId);
    route.inFlight = 1;
    fetch(url, { credentials: "same-origin" }).then((res) => {
        if(!res.ok) throw new Error("HTTP " + res.status);
        return res.text();
    }).then((html) => {
        route.inFlight = null;
        route.fragment = html;
        const pend = route.pendingActivate;
        if(!window.$__uni_mount_fragment(name, routeId)) {
            route.pendingActivate = null;                        // adoption failed: stay hidden
            return;
        }
        route.pendingActivate = null;                            // cleared only once the DOM is in place
        if(pend) window.$__uni_activate(name, routeId, pend[0], pend[1], pend[2], pend[3]);
    }).catch((err) => {
        route.inFlight = null;                                  // a later click retries
        route.pendingActivate = null;                           // still hidden, still current-1
        window.$__uni_router_error("fragment fetch failed", route.key + " " + err);
    });
    return true;
});

// A remote fragment is mounted by adoption, never by innerHTML: the fragment is
// the same SSR pipeline output, so its [data-chx-i] boundary is resolved and the
// node is hydrated in place (§12.7).
window.$__uni_mount_fragment = ((name, routeId) => {
    const route = window.$__uni_routers[name].routes[routeId];
    if(!route.fragment) return false;
    // Adoption, not injection: parse the fragment, lift the boundary span's
    // children into the route's own host, and mark the host as SSR-backed so the
    // normal activate path hydrates in place (§12.7, §6.7).
    const tmp = document.createElement("div");
    tmp.innerHTML = route.fragment;
    const src = tmp.querySelector('[data-chx-i]');
    if(!src || !route.host) {
        route.failed = true;
        window.$__uni_router_error("fragment has no boundary", route.key);
        return false;
    }
    while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
    while(src.firstChild) route.host.appendChild(src.firstChild);
    route.ssr = true;
    route.hydrated = false;                                     // the re-activation mounts it
    return true;
});

// Query values live in a signal so { r.query().search } re-renders on change.
// setQuery writes the URL with replaceState and never navigates (§6.5).
window.$__uni_set_query = ((name, obj) => {
    const r = window.$__uni_routers[name];
    if(!r) { window.$__uni_router_error("setQuery: unknown router", name); return; }
    const parts = [];
    for(const k in obj) parts.push(encodeURIComponent(k) + "=" + encodeURIComponent(obj[k]));
    const q = parts.length ? "?" + parts.join("&") : "";
    const base = (r.currentRoute && r.currentRoute.rawUrl) || window.$__uni_url_mem.path;
    const full = window.$__uni_norm_path(base) + q;
    // Keep the record's own identity in sync, or navigating away and back would
    // restore the pre-query URL and silently drop the query (§15.9-Q22).
    if(r.currentRoute) { r.currentRoute.rawUrl = full; r.currentRoute.url = window.$__uni_norm_path(base); }
    // INV-11: write the signal only when the query actually changed.
    if(window.$__uni_query_sig(r.$query.value) !== window.$__uni_query_sig(obj)) r.$query.value = obj;
    // One history path for the whole runtime (the try/catch and the opaque-origin
    // fallback live in $__uni_set_url; a second copy here is how they drift).
    window.$__uni_set_url(full, true);
});

// The bootstrap activation (template 5).  The SERVER picked the route id; the
// client needs that same route's *params* so hydration receives the props the SSR
// render used.  If the URL does not match the chosen route — an id-only router, or
// an app that selected the route with add_parameter("router", id) — params stay
// null, and the visible route is still the server's choice (§15.9-Q34).
window.$__uni_activate_initial = ((name, id, initialUrl) => {
    const r = window.$__uni_routers[name];
    const route = r && r.routes[id];
    if(!route) {
        window.$__uni_router_error("activateInitial: unknown route", name + "#" + id);
        return false;
    }
    if(!route.isUrl) {
        return window.$__uni_activate(name, id, undefined, null, window.$__uni_HISTORY_NONE);
    }
    const m = window.$__uni_match_url(name, initialUrl);
    const matched = !!(m && m.id === id && !m.fallback);
    // Only claim the URL as this record's identity when the URL actually maps to
    // it.  Otherwise a page that selected a route by parameter (or an id-only
    // router) would give that route the URL's path, and the URL's real route would
    // render its nav link as active while a different route is on screen (§15.9-Q45).
    return window.$__uni_activate(name, id,
        matched ? window.$__uni_norm_path(initialUrl) : undefined,
        (matched && m.params) ? m.params : null,
        window.$__uni_HISTORY_NONE,
        matched ? initialUrl : undefined);
});

window.$__uni_build_path = ((name, id, params) => {
    const r = window.$__uni_routers[name];
    const t = r && r.table;
    if(!t) return null;
    for(let i = 0; i < t.routes.length; i++) {
        const e = t.routes[i];
        if(e.id !== id) continue;
        let out = "";
        for(let j = 0; j < e.pattern.length; j++) {
            const s = e.pattern[j];
            out += "/" + (s.charCodeAt(0) === 123
                ? encodeURIComponent((params && params[s.slice(1, -1)]) || "")
                : s);
        }
        return (t.base || "") + (out || "/");
    }
    window.$__uni_router_error("buildPath: unknown route", name + "#" + id);
    return null;
});

window.$__uni_sync_url = ((name) => {
    const rr = window.$__uni_routers[name];
    if(!rr || rr.$popstate) return;                     // install exactly once (§15.9-Q43)
    rr.$popstate = 1;
    window.addEventListener("popstate", () => {
        // location.search is included so a back/forward onto a query URL repopulates
        // the query signal instead of clearing it (§15.9-Q24).
        const raw = window.$__uni_history_ok
            ? (location.pathname + location.search)
            : window.$__uni_url_mem.path;
        const m = window.$__uni_match_url(name, raw);
        if(!m) {
            // No route for this URL: the browser has already moved, so leaving it
            // would put the URL and the visible route in permanent disagreement
            // (the same failure the guard-denied branch below handles).  Report it
            // and put the URL back (§15.9-Q44).
            window.$__uni_router_error("no route matches on popstate", name + " " + raw);
            const cur0 = window.$__uni_routers[name].currentRoute;
            if(cur0) window.$__uni_set_url(cur0.rawUrl || cur0.url, true);
            return;
        }
        if(!window.$__uni_activate(name, m.id, window.$__uni_norm_path(raw), m.params,
                                   window.$__uni_HISTORY_NONE, raw)) {
            // guard denied: the browser already moved.  Put the URL back (§6.2).
            const cur = window.$__uni_routers[name].currentRoute;
            if(cur) window.$__uni_set_url(cur.rawUrl || cur.url, true);
        }
    });
    if(window.$__uni_history_ok) history.scrollRestoration = "manual";
});
```

**Size, measured (not estimated).** Counting exactly this block, comment-stripped
and whitespace-normalized (i.e. the shape the emitter would produce; the emitter
never ships the comments above):

| Portion | Lines | Raw | gzip |
|---|---|---|---|
| Everything (URL-routing page) | 472 | **17.9 KB** | **4.3 KB** |
| CORE + CONTROL, id-only router | 296 | **11.2 KB** | **2.8 KB** |
| URL + remote + query group | 176 | 6.6 KB | ~1.5 KB |

The earlier "~40 lines / ≤ 2 KB" figures in §7 were **wrong by roughly 7×** — they
counted a switch statement and forgot the registry, the URL layer, the remote
fetch path, focus/scroll and the method table. The honest position:

- **4.3 KB gzipped** is what an app pays on the wire for the full feature set;
  **11.2 KB / 2.8 KB** for an id-only router. That is a normal cost for a router
  and it is paid only by pages that declare one (INV-18: zero bytes otherwise).
- 17 KB uncompressed is a parse cost of roughly 0.4 ms on a typical phone — not a
  budget concern, but the *number* should be the measured one.
- §15.6 P-4 gates the emitted size at **≤ 20 KB raw / ≤ 5 KB gzip** (URL page) and
  **≤ 13 KB / ≤ 3.5 KB** (id-only), with a documented levers list if it ever has to
  shrink: drop the null-object's unused methods, split the URL group into its own
  emitted section (already done for id-only pages), shorten the internal names
  (the `$__uni_` namespace policy costs real bytes and is worth it for
  greppability — a deliberate trade, not an accident).

### 15.3 Frozen emitted artifacts (byte-exact templates)

Templates are normative: whitespace may differ, structure and attribute order may
not. Type names in `[·]` are metavariables. Emitted in this order:

1. **Registry object** (once per page, deduped by router name via `doneComponents`):

   ```js
   window.$__uni_routers.name = { name: "name", currentRoute: null, routes: Object.create(null),
       table: null,                       // filled by template 3, URL routers only
       $current: window.$_us(null), $url: window.$_us(null), $query: window.$_us(null) };
   Object.assign(window.$__uni_routers.name, window.$__uni_router_methods("name"));
   ```

2. **Route registration stub** (one per route, after component definitions):

   ```js
   window.$__uni_route_register("name", "id", {
       key: "name#id", id: "id", comp: COMP_OR_NULL, baseProps: PROPS_OBJ,
       wrapperId: "rN", hostId: "uN",
       remote: false, isUrl: true, fetchUrl: null,
       noscroll: false, title: "Title",
       beforeActivate: null, onActivate: null, onDeactivate: null });
   ```

   `isUrl` decides whether activation owns the history write (D-6.10); `baseProps`
   additionally carries a `null` placeholder for each `{param}` in the pattern
   (D-2.7); `fetchUrl` is always `null` in v1 and is resolved through
   `$__uni_router_fragment_url` (§14.10).

   one shared `$__uni_route_register(name, id, spec)` in the runtime does the two
   `document.querySelector` calls (wrapper by `[data-uni-route="name#id"]`, host by
   `$__uni_boundary(spec.hostId)` / `getElementById`) and stores
   `{el, host, comp, baseProps, params: null, hydrated: false, visible: false,
   ssr: !spec.remote, remote: spec.remote, url: null, inst: null, failed: false,
   scrollY: 0, focusEl: null, inFlight: false, ...hooks, title, noscroll}`.
   **Sharing one register function instead of an inline IIFE per route is the
   difference between ~40 bytes and ~120 bytes per route in the page script.**

3. **Match table** (once per page, *only* if URL routes exist), emitted **in
   precedence order** (D-6.9):

   ```js
   window.$__uni_routers.name.table = { base: "/app", routes: [
       { pattern: ["projects", "{id}"], id: "projects" },
       { pattern: ["settings"],         id: "settings" },
       { fallback: true, id: "not-found" }
   ] };
   ```

4. **SSR wrapper** (server, one per route; `data-uni-route-active` omitted when
   `true` is the default for the selected route — emit `="false"` only):

   ```html
   <div class="chx-route" id="rN" data-uni-route="name#id"
        data-uni-route-active="false" tabindex="-1">
     <span data-chx-i id="uN">…SSR body…</span>
   </div>
   ```

   `id="rN"` uses the wrapper element's own source location (`"r" + element.loc`,
   the same scheme as `uN`), so registration is a `getElementById` with no selector
   escaping and the id is stable across renders (D-2.5, §15.9-Q32).

   **`tabindex="-1"` is required**, not cosmetic: `$__uni_route_visible` focuses the
   wrapper on activation (§6.7, G-8), and a `<div>` without it is not focusable, so
   keyboard focus would silently stay on the old route's DOM (§15.9-Q28).

   For a `remote` route the wrapper renders **empty** (the `<span>` placeholder is
   still emitted — a missing host would make the record `failed` per D-7.7).

5. **Activation tail** (`pageJsEnd`, after `$__universal_flush()`):

   ```js
   // Id-only router:
   window.$__uni_activate("name", "default-id");
   // URL router: the SERVER chose the id; the bootstrap matches the real URL so the
   // active route gets its url/rawUrl identity AND the params hydration needs.
   // Passing null params instead would hydrate the SSR DOM with `id: null` while the
   // markup says `id: 42` — a hydration mismatch (§15.9-Q29/Q34, D-2.7).
   window.$__uni_set_table_base("name", BASE_STRING);   // only when the server saw one
   window.$__uni_activate_initial("name", "default-id", window.$__uni_initial_url());
   window.$__uni_sync_url("name");
   ```

6. **`<name>.routes.json`** (static export only), written by `writeToDirectory`:

   ```json
   { "name": "index", "base": "/app", "routers": [ { "name": "main-router",
     "routes": [ { "id": "projects", "pattern": "/projects/{id}", "fallback": false },
                 { "id": "not-found", "pattern": null, "fallback": true } ] } ] }
   ```

### 15.4 Frozen compiler-side skeletons (Phase 1/5)

These are the signatures implementation must match; bodies are the implementer's.

```chemical
// page.ch — the ONLY additions to the page type
public variant PageParameter { Text(view : std::string_view)  Object(ptr : *mut void) }
var parameters : std::unordered_map<std::string_view, PageParameter>
public func add_parameter(&mut self, key : std::string_view, value : std::string_view) : void
public func add_parameter_object(&mut self, key : std::string_view, ptr : *mut void) : void
public func get_parameter(&self, key : std::string_view) : std::string_view
public func has_parameter(&self, key : std::string_view) : bool          // contains(key)
var route_missing_flag : bool = false
public func mark_route_missing(&mut self) : void
public func route_missing(&self) : bool
// §13.2.3: populated by the generated router function during render, serialized by
// writeToDirectory in static-export mode, read by nothing per-request.
// NOTE: a page-local struct, not router::RoutePattern — `page` cannot see the
// router module (the dependency runs the other way, §3.1).
@direct_init
public struct RouteManifestEntry {
    var router : std::string_view
    var pattern : std::string_view    // "/projects/{id}", or "" for a fallback
    var id : std::string_view
    var is_fallback : bool
}
var route_manifest : std::vector<RouteManifestEntry>
public func add_route_pattern(&mut self, router : std::string_view,
                              pattern : std::string_view, id : std::string_view,
                              is_fallback : bool) : void
public func route_manifest_json(&self) : std::string

// router/src/request.ch
@make @direct_init
public struct RouteRequest { var method : std::string_view
                             var path : std::string_view
                             var query : std::string_view
                             var body : std::string_view }
public func (page : &mut HtmlPage) set_request(req : &RouteRequest)
public func get_request(page : &mut HtmlPage) : *mut RouteRequest

// router/src/url.ch
public func (page : &mut HtmlPage) set_route_url(path : std::string_view, base : std::string_view = "")
public func (page : &mut HtmlPage) get_route_url() : std::string_view      // reads __route_url
public func (page : &mut HtmlPage) get_route_base() : std::string_view     // reads __route_base

// router/src/params.ch
public func <T> get_parameter_object(page : &mut HtmlPage, key : std::string_view) : *mut T
public func (page : &mut HtmlPage) query_param(key : std::string_view) : std::string_view
public func parse_query(raw : std::string_view, out : &mut std::unordered_map<std::string_view, std::string_view>)

// router/src/match.ch — pure; no page, no globals.  Shared by the server matcher
// and by emit-time validation so both sides can never diverge (D-6.2).
public struct RoutePattern { var segments : std::vector<std::string_view>   // "projects" | "{id}"
                             var id : std::string_view
                             var is_fallback : bool }
public struct RouteMatch { var matched : bool
                           var id : std::string_view
                           var params : std::vector<RouteParam>      // copied, page-owned
                           var is_fallback : bool }
public func normalize_path(path : std::string_view, base : std::string_view) : std::string
public func match_route(patterns : &std::vector<RoutePattern>, path : std::string_view,
                        base : std::string_view) : RouteMatch
public func build_path(patterns : &std::vector<RoutePattern>, id : std::string_view,
                       params : &std::unordered_map<std::string_view, std::string_view>) : std::string
```

Rules that must hold in these bodies (all from §14, restated as implementation
requirements because they are the ones that silently diverge):

- `match_route` **copies** every param value into page-owned storage (D-5.3) — the
  returned views must never point into the request buffer.
- `normalize_path` is the *only* normalization: strip one trailing slash, strip
  `base` prefix, percent-decode **after** splitting, case-sensitive literals,
  reject `.`/`..` segments. The client `$__uni_norm_path` does the sub-set of this
  that is representable in JS (trailing slash + `?` strip); the server does the
  decoding because only it has `encoding::url_decode`.
- Precedence (D-6.2) is resolved **at emission time**: the emitter sorts patterns
  once (more literal segments → fewer params → declaration order) and emits the
  table in that order, so `match_route` and `$__uni_match_url` are both a
  straight first-match scan. No runtime scoring, no divergence possible.
- Ambiguity (R9) is detected by that same sort: two entries whose literal/param
  shape is identical after the ordering key collide.

### 15.5 Invariant register (the "no divergence" contract)

Each row is an assertion the implementation must uphold, where it is enforced, and
the test that fails loudly when it is broken. **A phase is not complete until its
rows are green.** This table is the checklist a reviewer runs against a PR.

| # | Invariant | Enforced at | Test |
|---|---|---|---|
| INV-1 | Per router, at most one record has `visible === true`; after a successful `activateRoute` **exactly one** and it is `currentRoute`; zero only after explicit `deactivate()` | `$__uni_route_visible` is the only mutator (D-7.3) | `router_state.ut.ch` — read `window.$__uni_routers` after every navigation step |
| INV-2 | A route mounts at most once per (record, url); `preload` then `activate` mounts once; a *different* url for the same record remounts exactly once (Q30) | `hydrated` guard in both entry points + the url-differs branch | `router_hydration.ut.ch` (effect counter) + `router_param_remount.ut.ch`, including the hidden-route case |
| INV-3 | No router entry point ever throws | every failure path is `$__uni_router_error` + `return false` | `router_error_no_throw.ut.ch` (uncaught errors fail the harness) |
| INV-4 | The wrapper element is never removed or replaced | mount is `"children"` on the inner boundary span | `router_mount_mode.ut.ch` (`<section>`/`<main>` roots) |
| INV-5 | The mount host is the `[data-chx-i]` span; the wrapper is never the host | `$__uni_route_register` resolves two distinct elements and asserts `host !== el` | runtime assert + registration test |
| INV-6 | Boundary ids are source-derived and stable across renders | emitted `hostId`, never a counter | `snapshot.ch` cold-vs-warm id comparison |
| INV-7 | Route bodies never enter `$__uni_hydration_queue` | `emit_route_registration` instead of `emit_universal_queue` | plugin test asserting the emitted JS contains no dispatch for route ids |
| INV-8 | Hiding survives reactive style rewrites | attribute + head-CSS rule, never inline `display` | `router_state.ut.ch` — a route root with a reactive style binding stays hidden |
| INV-9 | Params are **percent-decoded** client-side at match time and **JS-escaped** only where the value is emitted as source (server-serialized props, compile-time attributes) | `$__uni_decode_segment`; `js_string_escape` on the server | `router_url.ut.ch` with `he's`, `a\b` and `%2F` params — the client path must not double-escape |
| INV-10 | `baseProps` is never mutated | `$__uni_route_props` builds a fresh object | `router_param_remount.ut.ch` — navigate `1 → 2 → 1`, props identical to the first render |
| INV-11 | Signals are written only when the value changes | explicit `!==` before `$current`/`$url` assignment | `router_state.ut.ch` subscriber counter: same-id activate must not notify #current; param change must notify `$url` only |
| INV-12 | Route bodies cannot reach `$__uni_*` internals | converter diagnostic R14 | `lang/tests/negative/src/router_diagnostics.ch` |
| INV-13 | User params cannot occupy `__route_`/`__query_` keys | debug assertion in `add_parameter` | `lang/tests/libs/router/src/store.ch` |
| INV-14 | No DOM query on the activation path, and no allocation for **id routes**; a URL route additionally does one query parse + one history write, which is where its cost lives | `$__uni_route_register` resolves both elements at bootstrap; §15.6 R-1/R-2 | code review + `performance.now()` budget P-1, asserted separately for id and URL routes |
| INV-15 | Every failure leaves the *previous* route visible, never a blank page | hydrate-before-hide ordering (D-7.10a) | `router_error_no_throw.ut.ch` + a throwing-component fixture |
| INV-16 | Warm (snapshot) render is byte-identical to cold render | snapshot cache | `snapshot.ch` |
| INV-17 | Snapshot publication is race-free | once-guard per route (§7.5) | `snapshot.ch` 8-thread run |
| INV-18 | A page with no router emits zero router bytes and unchanged output | emission gated on declaration | G-2 determinism/zero-cost gate (§15.7) |
| INV-19 | Every diagnostic carries a `SourceLocation` (except R5, runtime) | converter diagnostic sites | negative suite asserts file:line in the message |
| INV-20 | Every `props.X` read in a route body is a declared root attribute or a `{X}` of its pattern, and the prop exists in `baseProps` (as a placeholder) so hydration can never be handed a missing key | converter validation R11 + the emitted stub's placeholders (D-2.7) | negative suite + `router_url.ut.ch` deep-link hydration assertion |
| INV-21 | A `remote` first activation is asynchronous and never blanks the page: the previous route stays visible until the fragment is adopted, then the swap happens through the normal path | `pendingActivate` + re-entry (D-7.11) | `router_remote.ut.ch` — a slow fragment fixture asserts the old route is still visible mid-flight |

### 15.6 Performance: how each budget is measured, and the rules that keep it

§7 and §7.5 state budgets. Budgets without a method are wishes, so each one gets a
harness, a sample size, and a counter. **All timing budgets start as reports
(D-9.4)** and are promoted to hard CI gates only after three consecutive runs land
inside range.

| # | Budget | How it is measured |
|---|---|---|
| P-1 | switch between two hydrated routes ≤ 1 ms (100-node bodies) | `#universal_test`: warm up with one activation, then `performance.now()` around 1000 alternating `activateRoute` calls; report median + p95 via `t.log`; assert the *invariant* (no mount happened: `hydrated` counters unchanged), gate the timing later |
| P-2 | 1000 alternating activations ≤ 50 ms | same loop, wall-clock total |
| P-3 | zero hydration of non-preloaded hidden routes at load | property read from `window.$__uni_routers` (§14.9 D-9.3) — a correctness gate today, not a timing one |
| P-4 | router runtime ≤ 20 KB unminified / ≤ 5 KB gzip (≤ 13 KB / ≤ 3.5 KB id-only) — the measured baseline is §15.2's table | `--libs` render test: render the same page twice, once with the registry emission suppressed, and diff `pageJs.length`; report both raw and gzip |
| P-5 | server: URL match + param store ≤ 10 µs at R=50 | `--libs` micro-benchmark over a generated 50-pattern table, 10 000 iterations |
| P-6 | server: 10-route all-static page with snapshots ≤ 1.2× single-page baseline | `--libs`: render baseline, then the routed page, N times; report ratio |
| P-7 | snapshot reuse (no re-render) | debug-build render counter: cold render increments, warm renders do not |
| P-8 | 8-thread concurrent render byte-identical to single-threaded | `--libs`, `-j 8`; diff the two outputs |

Hot-path implementation rules (violating any of these is what turns ≤1 ms into
10 ms — INV-14 is the enforcement row):

- **R-1** No allocation on the activation path for **id routes**: the runtime's
  closures are created once with the runtime block, and `activate_now` allocates
  nothing (no `Object.assign`, no array literals, no string concatenation) on the
  success path — string building appears only in `$__uni_router_error` calls, which
  are failure paths. **URL routes are the documented exception**: they build the query
  signature and the history write string once per navigation, and that is the
  accepted cost of the URL layer (§15.6 P-1 measures the two separately so a
  regression in either is visible).
- **R-2** No DOM queries on the activation path. All element resolution happens in
  `$__uni_route_register` at bootstrap (one `getElementById` + one boundary lookup
  per route); activation only calls `setAttribute`. The single query in the whole
  runtime is `tmp.querySelector('[data-chx-i]')` inside `$__uni_mount_fragment`,
  which searches a **detached** tree where `getElementById` cannot work and runs
  once per fetched fragment — never on a warm switch.
- **R-8** The query signal is only written when its **signature** differs
  (`$__uni_query_sig`), because a freshly parsed query object is never `===` the
  old one — comparing identities would make every navigation notify every
  `query()` subscriber, breaking INV-11. The signature is key-order sensitive
  (accepted: a reordered query at most re-renders the query consumers once, and
  sorting keys would cost an allocation on the URL path).
- **R-3** Null-prototype dictionaries (`Object.create(null)`) for `routes`,
  `params` and the props merge, so V8 keeps them monomorphic and no
  `Object.prototype` keys leak into route ids (`routes["constructor"]` is a real
  footgun).
- **R-4** `setAttribute("data-uni-route-active", …)` — one attribute write; the CSS
  rule does the layout change. (An attribute change on a `display:none` subtree does
  not invalidate layout of the visible tree.)
- **R-5** `route.params` is only allocated when the pattern actually has params
  (`params: null` for literal routes), so a literal-route switch allocates nothing.
- **R-6** Client matching is a single forward scan with no sort, no regex and no
  `decodeURIComponent` unless a param segment is consumed.
- **R-7** The `remote` fetch is the only async step and it is guarded by `inFlight`;
  nothing else in the runtime is deferred, so there is no microtask/task boundary to
  schedule around.

### 15.7 Production-quality gates (per phase, before merge)

| # | Gate | How |
|---|---|---|
| G-1 | **Determinism** — the same source renders byte-identical output on two runs | `--libs`: render twice in one process, `memcmp` the strings |
| G-2 | **Zero cost when unused** — a page with no `router` declaration emits output byte-identical to the pre-change compiler | golden-output test: check in the current output of an existing components page and diff |
| G-3 | **The compiler never crashes on bad router code** — every R* is a diagnostic, never an assertion failure or a SIGSEGV | negative suite runs the whole catalogue; the plugin suite runs the *valid* fixtures |
| G-4 | **Message fidelity** — diagnostic text matches §14.8 verbatim, with `file:line` | negative suite matches the exact strings |
| G-5 | **Both backends** — Phase ≥ 3 output is identical in shape on TCC and LLVM | `./scripts/test.sh --tcc --universal` then `./scripts/test.sh --llvm --universal` (or `--llvm --libs`) |
| G-6 | **Three-path identity** — server deep link, client nav, popstate produce identical props **and** identical route-root attributes | `--libs` SSR test + `router_url.ut.ch` comparing serialized props |
| G-7 | **No CSP footguns** — no `eval`, no `new Function`, no inline event attributes; listeners via `addEventListener` only | grep the emitted page JS in a test |
| G-8 | **Accessibility** — route root is focused on activation with `preventScroll`; `aria-current` on active links; deactivated routes are out of the tab order (they are `display:none`) | `router_state.ut.ch` + a manual a11y pass per phase |
| G-9 | **Error taxonomy** — every message is prefixed `[router] `; recoverable = `console.error`, never `throw`; unrecoverable SSR bugs use `$__uni_error` and are *not* router-originated | code review + `router_error_no_throw.ut.ch` |
| G-10 | **Observability** — debug builds expose per-request counters (match attempts, selected route, snapshot hits) and the client exposes `window.$__uni_routers` | `--libs` counter test; the WebView suite already reads the registry |
| G-11 | **Docs** — the library `README` carries the §13.4 three-deployment-mode table (the first thing an app author needs) and the §12.9 open items | review |
| G-12 | **No stale narrative** — §14/§15 and the shipped code agree; any deviation updates this document in the same PR | review |

### 15.8 Landing and rollback

- **Additive by construction.** Every phase adds a file or a symbol; nothing in
  Phases 0–3 changes the output of an existing app (G-2 asserts it). A page opts in
  by declaring a `router`, which is the only switch — there is no global flag and no
  runtime branch shipped to non-router pages.
- **Revert = delete.** Phases 1–6 keep no persistent state: no files written outside
  the build dir, no config, no manifest consumed at runtime except the optional
  `routes.json` (static hosts only, and a missing manifest degrades to single-page
  deep links). Reverting a phase is deleting its files plus the emission call site.
- **The one stateful piece is Phase 7's snapshot cache**, and it is bounded by
  construction: one `std::string` per *static* route per page module, i.e.
  `Σ static route body bytes`. There is no eviction because the bound is reached
  during compilation, not at run time. A debug assertion logs the total; a route
  classified dynamic is never cached, so the cache cannot grow with traffic.
- **Runtime versioning.** The emitted block sets `window.$__uni_router_version`
  (§15.2) so a future v2 runtime can refuse to operate on a v1 registry (or
  vice versa) instead of half-working. A page always ships exactly one version —
  the one its macro emitted.
- **Kill switch for a bad snapshot cache** (Phase 7 only): the cache read is
  guarded by a single flag checked in one place; setting it off restores per-request
  rendering with no other change. Debug builds have it off by default (§7.5.3).

### 15.9 Quality fixes applied in this pass

These are corrections to earlier sections, not new features. Each is in the
sections above already; they are listed here so a reviewer can verify them.

Rows are appended in the order bugs were found, so the numbering is chronological
rather than thematic — read it as a changelog, not a taxonomy. The four review
passes that produced it are: Q1–Q20 (pre-implementation audit of §15.1–§15.8),
Q21–Q33 (first bug hunt over the runtime source), Q34–Q44 (second bug hunt, over
the runtime *and* the module/dependency graph).

| # | Fix | Where |
|---|---|---|
| Q1 | **Record props are derived, never mutated** — `baseProps` is immutable, `params` holds the current URL params, `props` is built at mount. The old text overwrote `route.props` with the merged params, which loses the original props on the second param change and makes props depend on navigation history. | §2.1 note 5, D-7.2, §15.2 |
| Q2 | **Two router signals: `$current` (id) and `$url` (resolved url).** With one signal, a param change (`/p/1 → /p/2`) leaves the value unchanged, `$_us` bails, and *no* subscriber re-renders — so a URL-keyed active link goes stale. Worse, making that one signal hold the URL would re-render every id-keyed nav link on every param change. Two signals is the only partition that is both correct and cheap. | §5.4, §6.6, D-7.8, §15.2 |
| Q3 | **Hydrate before hiding the outgoing route.** Previously `onDeactivate` + hide ran before the mount, so any mount failure (throwing component, missing host) left the page with **zero** visible routes. Now the mount happens first and a failure returns early with the previous route still visible and no empty frame. | §2.1 note 4, D-7.10a, §15.2 |
| Q4 | **The record field is `currentRoute`, not `current`.** `router("m").current()` is a method on the very object that held a field named `current`; the field would shadow the method and user code would call a record as a function. Frozen names live in D-7.9. Also added `id` / `key` to the record (the error messages and `current()` need them) and a **null-router object** so a `<Link>` rendered on a page whose router component never rendered logs and no-ops instead of throwing. | §2.1, D-7.9, §15.2 |
| Q5 | **`failed` is actually checked in the activation path.** D-7.6 declared the field and never used it, so a failed route would be retried on every switch — exactly the remount storm the decision exists to prevent. | §2.1, §15.2 |
| Q6 | **`release` semantics were declared but not implemented.** Now specified: inactive-only, refused while a fetch is in flight, clears DOM + re-arms `hydrated`/`ssr`/`params`, and is a contained no-op on the active route. | §13.3.1, §15.2 |
| Q7 | **`route_visible` is where focus/scroll capture actually happens** (it was prose in §6.7 and absent from the only visibility mutator), including the `contains` validity check before restoring focus and the `noscroll` opt-out. | §2.1, D-7.3, §15.2 |
| Q8 | **`$__uni_route_visible` no longer writes a `null` state inconsistently** — the truth of `visible` is maintained *before* the element check, so a missing wrapper still leaves the registry self-consistent (D-7.7) instead of half-updated. | §15.2 |
| Q9 | **Precedence is resolved at emission time**, so runtime matching is a straight first-match scan on both sides with no sorting/scoring code that could diverge. New D-6.9. | §15.1 Phase 5, §15.4 |
| Q10 | **Escaping/decoding happens at exactly one point per side:** percent-*decoding* once, client-side, for pattern params only (never literal segments); JS-string *escaping* only where a param becomes source text, which is the server's serialized props. (Restated by Q21 after the client-side claim turned out to be wrong.) | §13.3.5, INV-9, §15.9-Q21 |
| Q11 | **Per-route registration is one shared function**, not a per-route IIFE — ~3× smaller page JS for multi-route pages, and it makes INV-5 (host ≠ wrapper) assertable in one place. | §2.2, §15.3 template 2 |
| Q12 | **The §7 cost and size rows were too optimistic.** Focus restore, optional scroll restore and the two `!==` compares are real work on the hot path — and the runtime is **17.9 KB raw / 4.3 KB gzipped**, not "~40 lines / ≤2 KB" (that figure was off by ~8× and is now a measured table in §15.2). Both rows are corrected rather than kept as marketing, and §15.6 replaces "≤1 ms" with a harness that measures it. | §7, §15.2, §15.6 |
| Q13 | **`has_parameter` used `contains(&key)`** — taking the address of a local `string_view` key builds a `*mut` key view, not a value. Corrected to a by-value key. | §3.2, §15.4 |
| Q14 | **`preload` writes no signal and never touches `currentRoute`**, and the `preload`-then-`activate` ordering is stated as an explicit branch (the `hydrated` fast path), since it is the double-hydration guard. | D-7.5, INV-2 |
| Q15 | **A `remote` route still renders its placeholder boundary span.** Without it the record's host is `null`, the record is `failed` (D-7.7), and the route can never load. This was implicit in "placeholder" and is now a template requirement. | §6.7, §15.3 template 4 |
| Q16 | **URL writing is owned by activation, not by the caller.** The earlier text had `$__uni_activate_by_url` write history *after* the activation, which means a direct `activateRoute(id)` on a URL route mounts the route and leaves the URL stale (the §6.2 "URL disagrees with the visible route" class of bug). Now it is one write inside `$__uni_activate_now`, selected by an explicit `historyMode` (0 none / 1 push / 2 replace), so popstate and the initial activation can never push. New decision D-6.10. | §6.2, D-6.10, §15.2 |
| Q17 | **Malformed percent-encoding cannot abort a navigation.** `decodeURIComponent` throws `URIError` on a bare `%`, which violates INV-3 (nothing in the router throws) for any user who types a broken URL. Decoding now goes through `$__uni_decode_segment`, which falls back to the raw segment. | §13.3.5, §15.2 |
| Q18 | **The registration spec was incomplete.** The old stub literal listed `{el, host, comp, props, hydrated, visible, ssr, url, inst}` while D-7.2 required `id`, `key`, `baseProps`, `params`, `remote`, `isUrl`, `fetchUrl`, `noscroll`, `title`, `inFlight` and the three hooks. An implementer following §2.2 literally would have shipped records that break INV-5, INV-11 and the remote path. The full spec is now in §15.3 template 2 and one shared `$__uni_route_register` builds it. | §2.2, §15.3 |
| Q19 | **`prefetch → click` race got an explicit path.** `$__uni_fetch_route` stores the adopted fragment on the record, then re-enters `$__uni_activate` with the recorded `pendingActivate` arguments if a click was waiting; a prefetch-only fetch just leaves the fragment for the activation to consume. Without the re-entry, a hover-prefetch whose click arrives before the response completes adopts the DOM but never becomes visible. | §6.6, §6.7, §15.2 |
| Q20 | **`remote` first activation was specified as "fetch on activate" with no mechanism, and the naive reading (mount the placeholder) would hydrate an empty host.** Now it is an explicit two-step with a recorded `pendingActivate` and a re-entry, so a click that lands before a hover-prefetch completes still works, and the page never blanks while the fragment is in flight. New D-7.11 / INV-21. | §6.7, D-7.11, §15.2 |
| Q21 | **The escaping rule was applied to the wrong side.** The client matcher was told to run `js_string_escape` on params, but a client match produces a JS *value*, not source text — nothing to escape, and double-escaping is the likely outcome of following the old text. Escaping belongs to the server, which interpolates params into generated JS. The helper was also misnamed (`$__uni_js_escape` for a percent-decoder); it is now `$__uni_decode_segment`. INV-9 and §13.3.5 are corrected. | §13.3.5, INV-9, §15.2 |
| Q22 | **Three query-related bugs, one root cause: the normalized path was used as if it were the URL.** (a) `Link href="/x?tab=2"` wrote the *stripped* path to history, silently dropping the query; (b) the no-op fast path compared only the normalized path, so navigating `/x?tab=1 → /x?tab=2` was treated as "already active" and did nothing at all; (c) the param-change branch disposed and remounted the component for a *query-only* change. Fixed by separating the record's identity (`url`, normalized, used for matching/active state/no-op) from the requested URL (`rawUrl`, query included, written to history, parsed for the query signal), and by gating the param-change branch on `route.url !== url` instead of identity alone. | §2.1, §6.5, §15.2, D-6.10 |
| Q23 | **A released or prefetched `remote` route mounted against an empty host.** After `release` the fragment was still cached but `ssr` was false, and the mount path cleared the (already empty) host and mounted the component with no SSR DOM to hydrate — a blank route that looks like a component bug. The remote branch now re-adopts a held fragment before mounting, and `preload` never re-fetches a fragment it already holds. | §6.7, §13.3.1, §15.2 |
| Q24 | **The query signal was only ever written by `setQuery`.** A deep link, a popstate, or a `Link` click onto a URL with a query left `r.query()` stale (and returning the previous route's query). The query is now parsed from the raw URL on every activation, including popstate, which reads `location.pathname + location.search`. Comparison uses a signature (R-8) so INV-11 still holds. | §6.5, §15.2, R-8 |
| Q25 | **`<title>` was never reset.** `document.title = route.title` only ran for routes that declared a title, so navigating from a titled route to an untitled one left the previous title in the tab — and the server, which emits the active route's title, disagreed. The runtime captures `$__uni_base_title` and falls back to it; `deactivate()` restores it too. | §13.2.1, §15.2 |
| Q26 | **Active-link comparison could never match a realistic href.** It compared `$url.value` (the normalized path) with the raw `props.href`, so a base path, a trailing slash, or a query made every link render inactive. `normPath` is now part of the public handle and the snippets use `r.normPath(props.href)` — the same rule the matcher applies. | §5.4, §6.6, §15.2 |
| Q27 | **`remote` had no fragment URL to fetch.** `route.fetchUrl` was declared but nothing ever set it, so every `remote` route failed at `"remote route has no fragment url"`. Added a documented, overridable page-wide resolver (`$__uni_router_fragment_url`) as the default convention; the endpoint stays application code (§13.2.4) and its failure is contained and retried. | §6.7, §13.2.4, §15.2 |
| Q28 | **The SSR wrapper lacked `tabindex="-1"`.** `$__uni_route_visible` focuses the wrapper on activation, but a plain `<div>` is not focusable, so focus never moved and G-8 (accessibility) was unsatisfiable. The template now emits it. | §6.7, §15.3 template 4, G-8 |
| Q39 | **`HtmlPage.route_manifest` was typed with a router-module type.** `page` cannot see `router::RoutePattern` — the dependency runs `router → page` (§3.1) — so the manifest accumulator needed a page-local `RouteManifestEntry`. The same mistake in `PageParameter` (§3.1) and `get_route_url(path)` (a getter taking an argument, §15.4) are fixed in the same pass (Q40/Q41). | §3.1, §3.2, §15.4 |
| Q40 | **`get_route_url` took a `path` argument it had no use for**, and `get_route_base` was missing entirely, so the read side of `set_route_url` was unusable. | §6.1, §15.4 |
| Q41 | **`PageParameter` was declared in the router library while `HtmlPage`'s map lives in `page`.** That is the cycle `page → router → page`, which the module system rejects — the store could not have compiled as written. `PageParameter` moves into `page` (`lang/libs/page/src/params.ch`); the router library contributes only extension functions over it. | §3.1, §3.2, D-5.1, §15.4 |
| Q45 | **The bootstrap could hand a route a URL that is not its own.** `$__uni_activate_initial` passed `normPath(initialUrl)` unconditionally, so when the server chose a route by `add_parameter` (or the router is id-only) while the URL mapped to a *different* route, the visible route claimed that URL — and the URL's real route rendered its nav link as active. Identity is now claimed only when the URL actually matches the activated route. | §6.1, §15.2, §15.3 template 5 |
| Q46 | **`$__uni_fetch_route` reported success for a failed route**, the same defect as Q37 in the sibling function — `if(route.hydrated \|\| route.failed \|\| route.inFlight) return true`. | §15.2 |
| Q47 | **`setQuery` had its own copy of the history write** (its own `try/catch` and its own `$__uni_history_ok` downgrade) instead of going through `$__uni_set_url`, and `release` refused an in-flight route silently. Both now share the one code path / one message. | §6.5, §15.2 |
| Q43 | **`$__uni_sync_url` could install its popstate listener twice.** The registry emission is deduped (D-2.4) but the tail is not guaranteed to be, and a second listener would run a second activation for one back-button press. It now installs once per router, guarded by `rr.$popstate`. | §6.2, §15.2 |
| Q44 | **A popstate onto an unknown URL silently did nothing.** The browser had already changed the URL, the visible route stayed the same, and no diagnostic fired — a bookmarkable URL that renders something else, which is precisely the failure §6.2 fixes for guard denials. Now it reports through `$__uni_router_error` and re-syncs the URL to the visible route. | §6.2, §15.2 |
| Q42 | **The client match table's `base` could never be populated.** `base` is request-time (the app passes `/app` to `set_route_url`) while the table is emitted at compile time, so a base-mounted app's client matching would treat `/app/x` as the path, match nothing, and leave `$__uni_activate_initial` without params (Q34 again). The generated server function now writes the base into the client table via `$__uni_set_table_base(name, base)` in `pageJsEnd`, before the activation tail. | §6.1, §6.2, §15.2, §15.3 template 5 |
| Q34 | **A deep-linked route hydrated with `id: null` while its SSR markup said `id: 42`.** Params reach a component two ways (D-2.7) and only the client half was specified: the props object has a placeholder per pattern param, and the matcher fills it — but the *initially active* route runs no matcher, so its placeholder stayed `null` and reactivity inside the route disagreed with the server-rendered DOM. The bootstrap now goes through `$__uni_activate_initial`, which matches the real URL and passes the params of the route the server chose (and `null` when the URL does not match that route, e.g. an id-only router or an app-selected route). | §6.1, §6.4, D-2.7, §15.3 template 5 |
| Q35 | **The `Link` snippets called things that do not exist in a `#universal` body.** `preventDefault()` was bare (the library convention is `e.preventDefault()` — verified in `Data.ch`, `Select.ch`), `router_should_intercept` was a symbol with no home, and `onPointerEnter` is not the library's or the test harness's event (`Surface.ch` uses `onMouseEnter`; the harness dispatches `mouseenter`). Now: `e.preventDefault()`, a real emitted runtime predicate `window.$__uni_should_intercept(e, href)` (the `window.$__uni_*` path is how JSX reaches runtime helpers — `window.$__uni_floating(...)`, `window.$__uni_inert_scan()`), and `onMouseEnter`/`onFocus`. | §6.3, §6.6, §15.2 |
| Q36 | **Two different shapes for the client match table.** §6.4 emitted `window.$__uni_route_tables["m"] = [ ... ]` while the runtime read `r.table = { base, routes }` — an implementer following §6.4 would have produced a table nothing could read, and the emitter and matcher would have drifted on precedence. §6.4 now specifies the runtime's shape, on the registry record, with the stale global name removed from D-3.2. | §6.4, D-3.2, §15.3 template 3 |
| Q37 | **`preload` reported success for a route that had already failed.** `if(route.hydrated \|\| route.failed \|\| route.inFlight) return true;` told a caller a dead route was warm, so an app branching on the return value would race a route that can never load. `failed` now returns `false`; `hydrated`/`inFlight` still return `true`. | §15.2, D-7.6 |
| Q38 | **D-3.2 named the internals three different ways.** The "internal symbol namespace" decision listed `$__uni_router*`, `$__uni_route_tables` and `$__uni_sync_url` as though they were one convention, so any reviewer comparing code to the doc would find mismatches everywhere. The namespace is now stated as two explicit groups with the actual symbol lists (router-scoped vs route/navigation), and `$__uni_route_tables` is deleted as a symbol. | D-3.2 |
| Q32 | **The wrapper was resolved with a CSS attribute selector built from user text.** `document.querySelector('[data-uni-route="name#id"]')` breaks for a URL pattern containing `"`, `]` or a backslash — and patterns are user-written strings. The wrapper now carries a source-location-derived DOM id (`id="rN"`) and registration is a `getElementById`, which is also cheaper and needs no escaping. | §2.2, §15.3 templates 2/4 |
| Q33 | **`deactivate()` then `activateRoute(id)` silently dropped the query.** With `raw` defaulting to the normalized path, re-activating a route restored `?tab=1` as `/x`. `raw` now defaults to the record's own `rawUrl` when the path is unchanged. | §6.5, §15.2 |
| Q31 | **A missing mount host threw a `TypeError` out of the click handler.** The host-clearing loop (`while(route.host.firstChild)`) sits *outside* the mount `try/catch`, so the one failure mode D-7.7 was written for (an SSR/JS host mismatch) still violated INV-3 — `$__uni_route_visible` being total did not cover it. Both entry points now check `route.host` first and mark the record `failed`. | §15.2, INV-3, D-7.7 |
| Q30 | **A hidden, previously-mounted route re-entered with a different path showed stale content.** The param-change branch was gated on `r.currentRoute === route`, so it only fired when the route was *active* when its path changed. A route that was visited (`/p/1`), navigated away from, and then activated as `/p/2` kept `hydrated === true` and its old DOM, so it was shown with the previous record's data. The guard is now "this record was mounted and its url differs", independent of which route is current. | §2.1, §6.4, §15.2 |
| Q29 | **A deep-linked route had no URL identity on first paint.** The activation tail called `$__uni_activate(name, id)` with no URL, so `route.url`/`rawUrl` stayed `null`: every nav link rendered inactive until the first client-side navigation, and the first click on the already-visible route was treated as a different URL (a needless remount). The tail now passes `$__uni_initial_url()` (with an opaque-origin guard), and template 5 is explicit about it. | §15.3 template 5, §15.2 |

**Net for this pass.** The design now has, for every phase: the exact files and
symbols, the normative runtime source, byte-exact emission templates, the
compiler-side signatures, an invariant register where every row names the test that
enforces it, a measurement method for every performance claim, and a merge gate.
The quality fixes of §15.9 are the ones that would have been silent bugs (§Q2 in
particular is a reactivity hole that no amount of testing the *happy path* would
have caught, because the fast path is exactly where it hides). Nothing here opens a
new design question; the open items remain the deliberately measurement-gated ones
in §12.9/§14.10.

---

## 16. Implementation status (what has landed)

This section tracks the shipped state against §15.1. It is updated in the same
change as the code (G-12). Where an implementation deliberately diverges from a
frozen signature it is called out.

| Phase | State | Landed |
|---|---|---|
| 0 — lexer/AST/parser | **Done** | `Hash` token (`js_syntax/src/TokenType.ch`, both lexers), `RouterDecl`/`RouteDecl`/`RouteHook` kinds + structs (`js_syntax/src/Ast.ch`), `universal_parser/src/parser/parser_router.ch`, contextual-keyword dispatch, `router_decl_summary` + parser tests. |
| 1 — parameter store | **Done** | `PageParameter`, `RouteManifestEntry`, `HtmlPage.parameters`/`route_missing_flag`/`route_manifest` + store/manifest/status methods (`page`), `RouteRequest`, `set_route_url`/`get_route_url`/`get_route_base`, `get_parameter_object<T>`, `query_param`, `parse_query`, `ensure_router_runtime`; `--libs` tests. |
| 2 — client runtime | **Done** | Normative runtime emitted from `lang/libs/page/src/router_runtime.ch` (CORE + CONTROL + URL groups), one hide rule, `ensure_router_runtime` latch (INV-18); runtime presence tests. |
| 3 — syntax + codegen | **Done (id routes + `preload`/`remote`)** | `universal_cbi/src/router/emit.ch` (`emit_router_server`/`emit_route_server`), wired through `emit_ssr_single_stmt` and `universal_replacementNode`; SSR wrappers/boundary spans, registry, per-route stubs, manifest entries, `preload` calls, activation tail in `pageJsEnd`; route bodies never enter the hydration queue (INV-7). Diagnostics R1/R3/R4/R6/R7/R13 + negative tests. Emission tests in `compiler_plugins/universal/src/router_emission.ch`; both backends compile a routed app. |
| 4 — control API, modes, hooks | **Partial** | `router(...)` lowers to `window.$__uni_router(...)` (D-3.1), `preload`/`remote`/`noscroll` modes, public `RouterLink`/`NavLink`/`Outlet` components (the navigation link is `RouterLink`, not `Link`, because `components` already ships `Link`), hooks (`onActivate`/`onDeactivate`/`onBeforeActivate`) converted into the registration stub, `route title` + 404 `noindex`, and diagnostics **R8** (literal `activateRoute`/`preload`/`buildPath`/`replaceRoute` ids), **R14** (`$__uni_*` in route bodies/hooks) via a JS-AST walk, **R11** (`props.X` reads in a route root component that are neither a declared root attribute nor a pattern param — the component's parsed JS body is exposed via `ComponentSignature.js_body`) and **R12** (`dangerouslySetInnerHTML` fed a route param). All R1–R14 are implemented. |
| 5 — URL layer | **Partial** | Pure `match_route`/`normalize_path_view`/`normalize_path`/`build_path`/`pattern_segments` (`router/src/match.ch`, `build_path.ch`) with `--libs` tests; **server-side matching** in the generated function via `router::apply_route_url` (deep-link selection, base stripping, fallback, 404 flag, param extraction — `--libs` tests); client match table with `{param}` placeholders and `isUrl` stubs; **emit-time precedence sorting** (D-6.9) shared by the server spec and the client table; `route noscroll`; **server-side percent-decoding** into page-owned storage with client-parity `decodeURIComponent` semantics; **mount-base write-back** into the client table (`$__uni_set_table_base`, Q42); **popstate wiring** for URL routers (`$__uni_sync_url`); **`{param}` → SSR-prop injection** (D-2.7) via `HtmlPage.get_parameter_text` + an `SsrText.getSsrAttributeValue` impl (this also fixed a latent bug: `AttrValueConverter.convert_node_attr_value` returned the access chain instead of the call); the `<name>.routes.json` manifest document (template 6: `name`/`base`/`routers`) written by `writeToDirectory`; runtime URL group (matcher, `setQuery`, remote fetch/adopt); **R9** (same-shape URL patterns are ambiguous, checked over the flattened entries so nested patterns count); a **site-level rewrite-map aggregate** (`page::site_routes_aggregate`/`write_site_routes`: a JSON array of the per-page `<name>.routes.json` documents, host-syntax-agnostic; `--libs` tests). |
| 6 — nested routes / `<Outlet />` | **Partial** | A route may declare nested `route` children; the route becomes a layout whose inline `<Outlet />` is expanded in place into the nested router's wrappers. The nested registry name is derived (`outer#route`), the outer route registers `nested`/`nestedDefault` so activation cascades at runtime, each nested level hydrates independently, the outer route's `{param}` values are **inherited by nested children** (both SSR and client), and **full URL nesting works**: nested URL routes are emitted into the outer match table as full accumulated patterns with an activation chain, so `/projects/{id}/settings` selects the outer layout *and* the nested child on both the server (`apply_route_url` stores each chain step's registry/id) and the client (`$__uni_match_url` returns the chain; `$__uni_activate_now` follows it, re-deriving the child from the shared URL on a param change per §13.3.3). A nested `route *` is emitted as a **prefix entry** (matched after every exact entry) so `/projects/{id}/unknown` resolves to the layout + fallback. `buildPath` resolves nested route ids; nested routers get full diagnostic coverage (R3/R6/R7/R8/R9/R10/R13/R14) and their own `<title>`. A **native-rooted layout is hydrated**: an anonymous client function renders the layout body, where `<Outlet />` compiles to an opaque `__uni_outlet` boundary that hydration consumes without touching the SSR'd nested wrappers — so a native layout's own markup is interactive and its DOM survives a child switch. A **component-rooted layout that forwards `{props.children}`** (`<Layout><Outlet/></Layout>`) is hydrated too: the outlet child is emitted as the route record's `children` vnode and passed through `props.children`. A route root component's **compile-time attributes are carried in `baseProps`** (D-2.7) so SSR and the client mount agree. An **`<Outlet />` inside a separate layout component's own body** is supported: the `Outlet` component renders a `data-uni-outlet` slot and the runtime relocates the SSR'd nested wrappers into the nearest slot when the layout activates (the wrappers are server-rendered after the layout so deep links still include the child content). Diagnostics **R11/R12** validate a route root component's `props.X` reads / `dangerouslySetInnerHTML`. `--libs` (SSR deep links incl. an id layout, a nested URL child, a nested fallback, and a separate-component outlet) + WebView (client chain, prefix fallback, buildPath, hydrated native layout, hydrated forwarding layout, separate-component outlet, route-root props) tests. |
| 7 — snapshot cache / static-route SSR | **Done (conservative classifier)** | Routes whose body is a purely-static native subtree (no components, expressions, spreads, hooks or URL params) render once into a process-global, mutex-guarded cache and append the cached bytes on later requests. Cache lives in `page` (`route_snapshot_append`/`route_snapshot_store`); the emitter's `router_body_is_static` classifier is deliberately conservative (anything uncertain is dynamic, so a snapshot can never be stale). `--libs` tests: cold/warm byte-identity, component routes excluded, 8-thread concurrent render byte-identical (INV-17). |

**Known divergences from the frozen signatures (documented, not silent):**

- `RouteMatch.params` values are views into the caller-owned `path`; the URL
  layer (`apply_route_url`) percent-decodes them into page-owned storage
  (`add_parameter_owned`) before they become page parameters, so decoding is a
  separate step from matching.
- URL routes currently register as ids (`isUrl` is set, but the default-route
  identity on the bootstrap still follows the declared default when server
  matching has not selected another route).
- `RoutePattern`/`RouteMatch` gained a `chain : std::vector<RouteChainStep>`
  field (§6.4 nested URL ownership). Nested URL routes are emitted into the
  **outermost** match table as **full accumulated patterns** plus a chain
  (`id` = the root layout route, `chain` = the derived-registry/id steps down to
  the leaf), instead of implementing segment-prefix/remainder matching. This is
  observationally equivalent for the supported pattern grammar (literal and
  `{param}` segments, no mid-pattern wildcard) and keeps a single exact-match
  scan on both server and client. A nested `route *` is represented as a
  **prefix** entry (a `prefix` flag on `RoutePattern`): it matches the parent
  pattern plus any remaining segments, and is scanned after every exact entry so
  an exact child always wins.
  `apply_route_url` stores each chain step under its derived registry name so the
  nested wrappers render visible server-side.

