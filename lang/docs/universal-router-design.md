# Universal Router Design

**Status:** Design (pre-implementation). Audited for production readiness (§11),
pre-mortemed for silent failure modes (§12), and re-audited mechanism-by-mechanism
against the runtime source (§13). This document is the implementability-reviewed
plan for the universal component router: a server-rendered, lazy-hydrating,
id-based route activation system with an opt-in URL layer.

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
    current: null,               // active route record
    $current: null,              // $_us signal holding the active route id (§5.4)
    routes: {                    // filled by the route activation stubs
        // "projects": { el, host, comp, props, hydrated: false, visible: false,
        //               ssr: true, url: null, inst: null }
    }
};
```

Record fields:

| Field | Meaning |
|---|---|
| `el` | the router-owned **wrapper** element (hide/show, focus, scroll target) |
| `host` | the `[data-chx-i]` boundary element **inside** the wrapper — the actual mount host |
| `comp` | the generated route client function (or `null`, §2.4) |
| `props` | the route's serialized props (params merged in for URL routes) |
| `hydrated` | client function has been mounted at least once |
| `visible` | mirrors `data-uni-route-active` (assertion aid, §12.4) |
| `ssr` | the **host currently holds SSR markup for this record's `url`**; `false` → next mount is fresh (clears the host first) |
| `url` | for URL routes, the resolved path this record serves (record identity for param routes is the *pattern*; `url` is the resolved value, §6.4) |
| `inst` | the mounted instance handle (`host.$__uni_instance`) for dispose-on-param-change |
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

window.$__uni_activate = ((routerName, routeId, url) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) {
        window.$__uni_router_error("activateRoute: unknown route", routerName + "#" + routeId);
        return false;                            // previous route stays active
    }
    url = (url === undefined) ? route.url : url;
    if(r.current === route && route.url === url) return true;   // exact no-op — O(1)
    if(r.current === route) {                    // param change (§6.4): SSR range is stale
        window.$__uni_dispose(route.inst); route.inst = null;
        route.hydrated = false; route.ssr = false;
    }
    if(route.beforeActivate && route.beforeActivate(url) === false) {
        window.$__uni_router_error("navigation cancelled", routerName + "#" + routeId);
        return false;                            // nothing mutated; current route intact
    }
    if(r.current) {                              // deactivate: hide, keep alive
        if(r.current.onDeactivate) r.current.onDeactivate();
        route_visible(r.current, false);
    }
    if(!route.hydrated) {                        // one-time hydration / fresh remount
        if(route.comp) {                         // §2.4: static routes have none
            if(!route.ssr) {                     // $__uni_dispose leaves SSR DOM in place
                while(route.host.firstChild) route.host.removeChild(route.host.firstChild);
            }
            window.$__uni_mount(route.host, route.comp, route.props, "children");
            route.inst = route.host.$__uni_instance;
        }
        route.hydrated = true;
    }
    route.url = url;
    route_visible(route, true);                  // show
    r.current = route;
    r.$current.value = routeId;                  // $_us bails out when unchanged
    if(route.onActivate) route.onActivate(url);
    return true;
})
```

**Three corrections to the earlier draft of this function, each verified against
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

`r.$current.value = routeId` is the reactive write from §5.4. The `$_us` setter
bails out when the value is unchanged (page.ch:588), so re-activating the same
route does not notify subscribers — the self-activate fast path costs no
re-render.

`route_visible` toggles the wrapper's `data-uni-route-active` attribute between
`"true"` and `"false"`; pageCss carries one rule, appended once:

```css
[data-uni-route][data-uni-route-active="false"] { display: none !important; }
```

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

**Re-entrancy rule (must be defined before implementing, §12.4):** hooks
(`onActivate`/`onDeactivate`) run *after* `r.current` is set, and a call to
`activateRoute` from inside a hook does not run inline — it is queued on a
microtask and executed after the in-flight activation completes. Without this,
an `onDeactivate` that navigates would interleave two activations over the same
`r.current` field and corrupt the visibility pairing (a route left visible with
`current` pointing elsewhere). The queue is one array and a scheduled flag — a
few lines, decided here so it is not discovered as a heisenbug.

### 2.2 Route activation stubs

Each route emits a two-line stub into the page JS (component-definitions section):

```js
// server renders:
// <div class="chx-route" data-uni-route="main-router#projects"
//      data-uni-route-active="false">
//   <span data-chx-i id="u17">…SSR of <Projects/>…</span>
// </div>
(function(){
    const el = document.querySelector('[data-uni-route="main-router#projects"]');
    const host = window.$__uni_boundary("u17");      // existing boundary resolver
    window.$__uni_routers["main-router"].routes["projects"] =
        { el: el, host: host, comp: Projects, props: {},
          hydrated: false, visible: false, ssr: true, url: null, inst: null };
})();
```

Points that make this correct:

- `host` is the route's **hydration boundary** resolved through the existing
  `$__uni_boundary(id)` mechanism (element id, or comment marker for table
  contexts). No repeated attribute scanning; the id is known at compile time.
  `el` is the wrapper the router owns for hide/show — resolved once at bootstrap
  by its `data-uni-route` attribute (a single `querySelector` per route, at
  bootstrap only, never in the activation path).
- The wrapper must contain a **real `[data-chx-i]` boundary span** around the SSR
  body (not bare markup) so the host is an element with no leading-text hazard,
  and so the route mount is byte-for-byte the same hydration path a normal
  `#html`-embedded component takes (§2.1 note 2).
- The stub only runs when the component's client function has been emitted (stubs
  are emitted after component definitions in the segmented JS output, so `Projects`
  is defined; the professionalization plan's segmented sections make this ordering
  deterministic instead of relying on `move_js_range`).
- `props` are the route's serialized props (§4.3), passed to the component on mount,
  so a route can be `<Projects filter="active" />`.
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

- The wrapper carries three router-owned attributes/classes and nothing else:
  `class="chx-route"`, `data-uni-route="name#id"`, `data-uni-route-active`. The
  route's own markup lives inside the boundary span, never on the wrapper, so no
  user/component prop can clobber the router's attributes.
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
// lang/libs/router/src/params.ch
public variant PageParameter {
    Text(view : std::string_view)
    Object(ptr : *mut void)
}
```

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
    return parameters.contains(&key)
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
3. Emits its activation stub with `hydrated: false` — the client still hydrates it
   through the normal queue (uniform code path; no SSR-side "already active" branch).
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

`router("name")` compiles to `$__uni_routers["name"]` access with a compile
diagnostic if the name is not declared on the page (§4.4). The handle's methods:

| Method | Behavior |
|---|---|
| `activateRoute(id)` | §2.1; idempotent, returns bool |
| `replaceRoute(id)` | like `activateRoute`, but history `replaceState` (redirects, login flows) |
| `deactivate()` | hide current, keep state; `current = null` |
| `isActive(id)` | bool; also available **reactively** (§5.4) for JSX bindings |
| `preload(id)` | hydrate without showing (§5.1 `preload` keyword does this at load) |
| `current()` | active id or `null` |
| `buildPath(id, params)` | URL routes: reverse-map the emitted match table to a path string (§6.6) |
| `query()` | URL routes: parsed query params of the current URL (§6.5) |

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
// inside the router record created at bootstrap
r.$current = window.$_us(null);      // a $_us signal holding the active route id
```

`activateRoute`/`deactivate` set `r.$current.value`; `route.$current` is nothing
new — it is the same `$_us` machinery components already use, so `$_ucs` computed
wrappers and JSX reactivity work on it unchanged:

```chemical
#universal NavLink(props) {
    const r = router("main-router")
    <a class={r.$current.value == props.id ? "active" : ""}
       aria-current={r.$current.value == props.id ? "page" : null} ... />
}
```

This costs one signal object per router and zero extra runtime concepts, and it is
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
  **Decoder source:** the `http` module already ships `http::url_decode` and
  `http::parse_query` (pure string helpers, no sockets involved). The router
  library depends on them for decoding/query parsing rather than duplicating
  ~40 lines — a compile-time dependency on string utilities only; the "no `net`
  dependency" rule is about sockets/serving and is unaffected. If that dependency
  is judged unacceptable, fall back to a local decoder — decide at Phase 5,
  defaulting to reuse.
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
  order and the last request wins through the normal `r.current` comparison.
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
           if(!router_should_intercept(e, props.href)) { return }
           preventDefault(); router("app").activateRouteByUrl(props.href)
       }}>
        {props.children}
    </a>
}
```

`router_should_intercept` is one library predicate (`button === 0 && !metaKey &&
!ctrlKey && !shiftKey && !altKey && no target/download attr && href is an
internal path`). An `href` that is absolute to another origin, protocol-relative,
`mailto:`/`tel:`, or marked `download` is **never** intercepted — it is a normal
navigation. (A `Link` that unconditionally calls `preventDefault()` — as the
draft above did — silently breaks ctrl-click-to-new-tab and middle-click, both of
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
import net
import net.client          // the http module

public func handle_request(req : &http::Request, res : &mut http::ResponseWriter) {
    var page = HtmlPage()
    page.defaultUniversalSetup()                       // existing universal runtime

    // 1. Hand the page the raw request URL. Matching happens *during* render,
    //    inside the generated router code that owns the patterns (§6.1).
    page.set_route_url(req.path, "/app")               // base optional

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
// emitted once per page that uses URL routes
window.$__uni_route_tables["main-router"] = [
  { pattern: ["projects", "{id}"], id: "projects" },
  { pattern: ["settings"],         id: "settings" },
  { fallback: "not-found" }                       // from `route *`
];
```

Client matching (`$__uni_match_url(table, pathname)`) walks segments; on match it
extracts `{param}` values and passes them into the route record's props before
activation. So the same declaration serves:

- **Server deep link:** the generated router function matches (§6.1), stores
  params (§3.4) and renders the route with `props.id` baked into its serialized
  props.
- **Client nav:** `activateRouteByUrl(path)` matches client-side, sets
  `route.props = { id: "42" }`, then `activateRoute(id)` mounts with those props.
- **popstate:** same client path, no server round-trip.

This keeps the "everything the user did not write never happens" rule: a param
name used in `props.id` is validated at compile time against the route's declared
pattern (the macro knows both), so a typo like `props.iid` is a diagnostic, not an
undefined prop at runtime.

**Param-change semantics (pre-mortem, §12.1 — must be implemented as written):**
the record identity for a param route is the *pattern* id (`"projects"`), but its
`url` + `props` fields hold the **resolved** values. Activation therefore runs a
three-way check, not the §2.1 two-way one:

1. `r.current === route && r.current.url === url` → true no-op (O(1)).
2. `r.current === route && r.current.url !== url` → **param change**: dispose the
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
    // active state comes from the §5.4 signal — no manual wiring
    <a href={props.href}
       aria-current={r.$current.value == props.routeId ? "page" : null}
       onPointerEnter={() => { if(props.preload) { r.preload(props.routeId) } }}
       onFocus={() => { if(props.preload) { r.preload(props.routeId) } }}
       onClick={(e) => {
           if(!router_should_intercept(e, props.href)) { return }   // §6.3
           preventDefault(); r.activateRouteByUrl(props.href)
       }}>
        {props.children}
    </a>
}
```

- **Hover/focus prefetch** (`<Link preload>`): calls `preload(id)` on
  `pointerenter`/`focus`. For eager-SSR routes that hydrates early; for
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
- **Keyboard and modified clicks** follow §6.3 (`router_should_intercept`): Enter
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
| Navigate to hydrated route | 2 attribute writes + 1 identity compare (short `url` compare on URL routes) + a no-op signal write | §2.1 |
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
between two hydrated routes ≤ 50ms; router runtime ≤ 2KB unminified; zero hydration
of non-preloaded hidden routes at load (assertable in the WebView suite).

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
`PageParameter`, `HtmlPage.parameters` + 4 methods, `RouteRequest`, extension
functions, unit tests in `lang/tests/libs/router/`. Exit: store roundtrips; page
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
`router("name")` handle with compile-time name checking, `preload`/`lazy`
keywords, `onActivate`/`onDeactivate`/`onBeforeActivate`, reactive router state
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
apply). Phase 2's segmented-JS dependency is soft: stubs can be emitted in
dispatch order today, and the professionalization plan's segmented sections
improve ordering guarantees without changing this design.

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
  has `visible: true` and it is `r.current`; every other record of that router is
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
   `current`/`$current`/match table; activation in one never touches the other;
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
5. **Route props are serialized, so params must be escaped.** §6.4 merges `{id}`
   into the record's props before mounting; those values become part of the
   generated JavaScript props object and are subject to the AGENTS.md
   single-quote/backslash escaping contract. The client matcher must run
   `js_string_escape` on every param before it enters props (the server path is
   already escaped at SSR). A param like `he's` otherwise produces a syntax error
   in the emitted props object, and a param containing `\` produces a wrong value.
   This is a one-line rule with a real injection-adjacent failure mode.
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
