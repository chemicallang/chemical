# Universal Router Design

**Status:** Design (pre-implementation), audited once for production readiness (§11).
This document is the implementability-reviewed plan for the universal component
router: a server-rendered, lazy-hydrating, id-based route activation system with an
opt-in URL layer.

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
        // "projects": { el, comp, props, hydrated: false, visible: false,
        //               ssr: true, url: null, inst: null }
    }
};
```

Record fields: `ssr` — whether the boundary contains server-rendered markup for
*this record* (false → first activation mounts fresh, see §6.4/§12.2); `url` —
for URL routes, the resolved path this record serves (§6.4: the record identity
for param routes is the resolved path, not the pattern).

`activateRoute` is a tiny, monomorphic function:

```js
window.$__uni_activate = ((routerName, routeId) => {
    const r = window.$__uni_routers[routerName];
    const route = r && r.routes[routeId];
    if(!route) {
        window.$__uni_error("activateRoute: unknown route", routerName + "#" + routeId);
        return false;
    }
    if(r.current === route) return true;           // already active — O(1)
    if(r.current) {                                 // deactivate: hide, keep alive
        route_visible(r.current, false);
    }
    if(!route.hydrated) {                           // one-time hydration
        window.$__uni_mount(route.el, route.comp, route.props, "root");
        route.hydrated = true;
    }
    route_visible(route, true);                     // show
    r.current = route;
    return true;
})
```

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
Every later switch = two attribute writes and a pointer compare. No DOM queries, no
string hashing (the routes record is a plain object the engine optimizes), no
re-render, no effect re-runs.

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
// server renders <div data-uni-route="main-router#projects" hidden>…</div>
(function(){
    const el = window.$__uni_boundary("u17");        // existing boundary resolver
    window.$__uni_routers["main-router"].routes["projects"] =
        { el: el, comp: Projects, props: {}, hydrated: false, visible: false };
})();
```

Points that make this correct:

- `el` is the route's **hydration boundary** resolved through the existing
  `$__uni_boundary(id)` mechanism (element id or comment marker for table
  contexts). No `querySelector` by attribute is needed; the id is known at compile
  time.
- The stub only runs when the component's client function has been emitted (stubs
  are emitted after component definitions in the segmented JS output, so `Projects`
  is defined; the professionalization plan's segmented sections make this ordering
  deterministic instead of relying on `move_js_range`).
- `props` are the route's serialized props (§4.3), passed to the component on mount,
  so a route can be `<Projects filter="active" />`.

### 2.3 SSR side: the wrapper element

The route body is server-rendered inside a wrapper that is hidden by attribute:

```html
<div data-uni-route="main-router#projects" data-chx-i data-uni-route-active="false">
   ...SSR of the route body...
</div>
```

- The wrapper is the hydration host; `$__uni_mount(el, comp, props, "root")` adopts
  the SSR'd subtree in place (already supported for `__uni_uc` vnodes).
- The default route (§3.4) renders with `data-uni-route-active="true"` (or no
  attribute — visible is the default); other routes render `"false"`. Hiding is
  CSS-rule-driven (§2.1), so no inline style is involved and reactive style
  bindings on the route root cannot unhide a deactivated route.
- SSR renders **every** route body eagerly (subject to mode, §5.1). This is what
  makes navigation instant. The ideation's cost warning (500 KB of Admin HTML for a
  Dashboard visitor) is answered by lazy routes (§5.1) and the URL layer (§6), not by
  making eager rendering implicit.

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
4. Emits `window.$__uni_activate("main-router", "<id>")` at the end of pageJsEnd so
   the initial route activates (and hydrates) exactly like a click does.

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

1. **SSR**: the route body is rendered inside the hidden wrapper div with
   `data-uni-route="name#id"` (§2.3). Concretely the generated server function emits:
   `page.append_html('<div data-uni-route="main-router#projects" style="display:none">')`,
   then the body's SSR, then `</div>`. The default route (§3.4) emits without the
   `display:none` part.
2. **Client JS**: the route body is compiled like a nested universal child — an
   anonymous client function + `$_uc_c` style function-reference vnode — and the
   route's activation stub registers `{ el, comp, props }` in
   `$__uni_routers["name"].routes[id]` (§2.2).
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

The runtime error path is also clean: `$__uni_error` logs with router/id context and
returns `false`; the previous route stays active. No throw, no broken page.

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
They are effect-like (cleanup-safe, error-isolated through the existing
`$__uni_error` containment) but **not** effects — they fire on activation
transitions, not on state changes. First activation also runs the component's
`useEffect`s once (hydration); later activations only fire `onActivate`.

A fourth hook, `onBeforeActivate`, is the **guard** hook:

```chemical
route #"admin" {
    onBeforeActivate(() => { return session().isAdmin })   // false cancels the nav
    <Admin />
}
```

It runs *before any DOM change* (before the current route is hidden), so a `false`
return simply keeps the current route active and logs via `$__uni_error`. This is
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

An extension that maps the request path to a router + id + params:

```chemical
// lang/libs/router/src/url.ch — extension receiver form (no explicit self param,
// matching the established `func (page : &mut HtmlPage) name(...)` pattern)
public func (page : &mut HtmlPage) activate_route_by_url(
    router_name : std::string_view, path : std::string_view,
    base : std::string_view = ""
) : bool
```

- `base` strips a mount prefix first (apps served under `/app/`); without it,
  deep links into sub-path deployments would never match.
- Path normalization before matching: strip trailing slash (except root), decode
  percent-encoding, reject `..` segments (matching is segment-wise against
  declared patterns, so traversal cannot match — normalization keeps `//` and
  dot-segments from matching unexpectedly).
  **Decoder source:** the `http` module already ships `http::url_decode` and
  `http::parse_query` (pure string helpers, no sockets involved). The router
  library depends on them for decoding/query parsing rather than duplicating
  ~40 lines — this is a compile-time dependency on string utilities only; the
  "no `net` dependency" rule (§6.1 header) is about sockets/serving and is
  unaffected. If that dependency is judged unacceptable, fall back to a local
  decoder — decide at Phase 5, defaulting to reuse.
- The router library ships components (`Link`, `NavLink`, `Outlet` — §6.3), so
  it is a CBI-plugin library like `components`; it imports `page` and the html
  stack, never `net`.
- Matches `route "/projects/{id}"` patterns (compile-time-built match table; linear
  scan is fine for realistic route counts, `unordered_map` for static prefixes).
- On match: `page.add_parameter(router_name, matched_id)` (§3.4) and stores
  `{id}` params as `Text` parameters keyed `__route_param_<name>`, plus a helper
  `route_param<T>(page, name)` for typed reads inside components. The matched
  params are **also** passed as props to the route component on both server and
  client (§6.4) — a route body should read `props.id`, not a server-only helper.
- On miss: the `route *` fallback route; if none, no activation (page renders
  inert/hidden and the server can 404 — the ideation's "user's mistake" case).

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
- No automatic scroll restoration, no redirect loops (activate on popstate never
  pushes); these are documented non-goals for v1.
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
    <a href={props.href} onClick={() => { preventDefault(); router("app").activateRouteByUrl(props.href) }}>
        {props.children}
    </a>
}
```

Ships in the **router library** (`lang/libs/router/src/Link.ch`), together with
`NavLink` (active-styling variant) and — Phase 6 — `<Outlet />`. One
`import router` therefore provides the full routing surface: the macro, the
control API, the extension functions, and the routing components. These
components use only the public control API, so they compose with custom routers.

### 6.3.1 Complete server setup (all of it — there is nothing else)

The server side is **three lines inside the handler the user already writes**.
Full example with the real `http` API (from `lang/compiled/docs/src/stdlib/net_http.md`):

```chemical
import router
import net
import net.client          // the http module

public func handle_request(req : &http::Request, res : &mut http::ResponseWriter) {
    var page = HtmlPage()
    page.defaultUniversalSetup()                       // existing universal runtime

    // 1. ONE router line: match req.path against the page's declared patterns,
    //    select the default route, store {id} + query params. Returns false on
    //    miss with no fallback route (user decides to 404).
    var matched = page.activate_route_by_url("main", req.path)

    // 2. request context (only if route bodies need it)
    page.set_request(&RouteRequest.make(req.method, req.path, req.query, ""))

    #html { <App /> }                                  // router declared inside App

    if(matched) {
        res.write_string(page.htmlPageToString())      // or res.send_file for static export
    } else {
        res.status = 404u
        res.write_string("Not Found")
    }
}
```

That is the whole integration:

| Step | Required? | Cost |
|---|---|---|
| `page.activate_route_by_url(...)` | yes for URL routes | one call, µs class (§7.5) |
| `page.set_request(...)` | only if route bodies read the request | one store insert |
| `page.add_parameter("main", "id")` | replaces step 1 for id-only routing | one store insert |
| static export (`writeToDirectory`) | replaces all of the above | zero per request; client match table handles deep links (§6.8) |

The macro and client runtime need **no** server setup: declaring `router` inside
a component is enough — the compiler emits the registry, wrappers, and stubs
into the page. `activate_route_by_url` merely tells that machinery which route
the request wants.

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

- **Server deep link:** `activate_route_by_url` matches, stores params (§3.4) and
  renders the route with `props.id` baked into its serialized props.
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

- **Server:** `activate_route_by_url` splits the raw query and stores decoded
  values as `__query_<k>` parameters (§3.4); `page.query_param(key)` reads one.
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
       onClick={() => { preventDefault(); r.activateRouteByUrl(props.href) }}>
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
- Active styling is `aria-current` + the reactive class shown above; a
  `class={...}` merge with `props.class` follows the existing class-merge rules.

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
- **Redirects:** client — `router("m").replaceRoute(id)` (history `replaceState`,
  no history spam on login→dashboard flows); server — `redirect_to(page, router,
  id)` is `add_parameter` under the hood, or the user returns a real 302 from
  `net_http`. No `redirect` syntax in route bodies in v1: redirects at activation
  time would fight the guard model; they belong in handlers (server) or before
  activation (client).
- **Scroll & focus (all activations, not just URL ones — §12.4):** on activate,
  scroll restores to top (URL routes; per-route `route noscroll` opts out) and
  the route's root receives focus (`tabindex="-1"`,
  `focus({ preventScroll: true })`) so keyboard/screen-reader users land in the
  new view. On deactivate, the router records `document.activeElement` **if it
  lives inside the route**; re-activating that route restores focus to the
  recorded element (falling back to the root) — without this, hiding the active
  route drops focus to `<body>` and keyboard users lose their place, and a
  focused `<input>`'s value survives (DOM persists) but focus does not.

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
| Navigate to hydrated route | 2 attribute writes + 1 pointer compare | §2.1 |
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
user code parses the request, calls `page.activate_route_by_url(...)` (or
`add_parameter` for id routing), renders the page, writes the response. The
library depends on `page` only — `net` stays out (§6.1). For static exports,
`activate_route_by_url` is simply not called; deep links are served by mapping
the URL to the pre-built page file + the client-side match table handles route
selection in the browser (the emitted match table makes static deep links work
without server logic — worth noting as a deployment mode: static host + client
router handles `/projects/42` by loading the page that declares the pattern,
then matching client-side).

**Numbers to hold ourselves to (extend §7's CI gates):** per-request render of
a 10-route page with all-static routes + snapshot cache ≤ 1.2× the single-page
baseline; URL match + param store ≤ 10µs for R=50; snapshot reuse verified by
an `--libs` test asserting the snapshot string is appended and not re-rendered
(counter exposed in debug builds).

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
| `router_navigation.ut.ch` | first activate, re-activate, self-activate, unknown id → contained `$__uni_error`, previous route stays active |
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
`$__uni_routers` registry, `$__uni_activate`, `route_visible`, hook invocation,
`$__uni_error`-contained unknown-route path. Exercisable by hand-written JS in a
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
`activate_route_by_url` (base path + normalization), route params server-side,
client-side match table + params-as-props (§6.4), query params (§6.5),
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
throw, only contained `$__uni_error` + fallback.

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
- **The pairing invariant:** after every `activateRoute` returns (or fails), the
  system must satisfy: exactly one route record has `visible: true` and it is
  `r.current`; every other record is hidden with its instance intact. The
  WebView suite should assert this invariant across every navigation test, not
  just route-specific assertions.
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
2. **Hydration boundary spans are `display: contents`.** The route root is the
   wrapper div (a real element), not a `[data-chx-i]` span — the boundary span
   lives *inside* it. Focus restoration (§12.4) and scroll management target the
   wrapper, which is safe to focus; never target the `display: contents` span.

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
  that `activate_route_by_url` copies param values into page-owned storage
  (page-lifetime) so request-buffer reuse across keep-alive requests cannot
  dangle. One copy per param, at match time, is the acceptable cost.
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
