---
name: Universal Router
description: How to use the Chemical universal router (server-rendered, lazy-hydrating, id-based routes with an opt-in URL layer) and how it is built — the lexer/parser/converter/runtime pipeline, the `RouterLink`/`NavLink`/`Outlet` components, the node/plugin test suites, and the maintenance rule that this skill must be updated with every router change. Load before writing router app code, changing `lang/libs/router`, `lang/libs/page`'s router support, or `universal_cbi`'s router emission.
---

# Universal Router

> ⚠️ **This skill must be kept in sync with the code.** The router is an
> actively-developed subsystem. **Any change to router behaviour, syntax,
> emission, runtime symbols, diagnostics, or tests MUST update this file in the
> same change** — including the “Change-impact map” at the bottom. If you change
> an emitted JS global, an invariant, a diagnostic string, or a public API, it is
> a bug to leave this skill stale. Treat a stale skill as a failed review.
>
> The normative design is
> [`lang/docs/universal-router-design.md`](../../../lang/docs/universal-router-design.md)
> (sections §2–§15, with §16 = implementation status). **Where this skill and the
> design doc disagree, the code wins; fix the skill and the doc.** For an app
> author's quick reference see
> [`lang/libs/router/README.md`](../../../lang/libs/router/README.md).

## What the router is

A client-side router for Chemical universal components. HTML is server-rendered
and inert until its route activates; the router decides *when* a route hydrates.
The core primitive is `router.activateRoute("id")`. URLs are a layer on top of
ids, never inside the core. A route is never destroyed on navigation — activation
is “hide the old, show the new”, so state, scroll and focus survive.

Five runtime states per route component: `SERVER_RENDERED → UNHYDRATED →
HYDRATING → HYDRATED` (effects run once, at hydration), with a separate
`visible` flag that toggles often.

---

## Part 1 — How to use the router

### 1.1 Declare a router (inside a `#universal` component)

```chemical
#universal App(props) {
    router "main-router" {
        route default #"dashboard" { <Dashboard /> }
        route #"projects"           { <Projects /> }
        route "/projects/{id}"      { <Project /> }   // URL route
        route #"admin" preload      { <Admin /> }     // hydrate at load
        route #"reports" lazy       { <Reports /> }   // SSR only (default for hidden)
        route "/frag" remote        { <Frag /> }      // fetch body on first activation
        route #"about" noscroll     { <About /> }     // do not restore scroll
        route #"home" title "Home"  { <Home /> }      // <title> + document.title
        route *                     { <NotFound /> }  // fallback
    }
}
```

Rules and notes:
- `router "name" { ... }` must be inside a `#universal` component body. Router
  names must be unique per page.
- `route #"id"` is an **id route** (no URL semantics). `route "/a/{b}"` is a
  **URL route** (its record id is the normalized pattern). `route *` is the
  fallback; it must be last and there may be at most one.
- A route body must render exactly **one** JSX root (diagnostic R6).
- Hooks go next to the root: `onActivate`, `onDeactivate`, `onBeforeActivate`
  (returning `false` cancels before any DOM change).
- Modes: `preload` | `lazy` | `remote`, optionally `noscroll`, then `title "…"`.
  Order: `route [default] (#id | "/path" | *) [mode] [noscroll] [title "…"] { … }`.
- **Nested routes (layout):** a route may declare nested `route` children and an
  inline `<Outlet />`. The children render at the Outlet position; each level
  hydrates independently; a nested child inherits the outer route's `{param}`s.

```chemical
route "/projects/{id}" {
    route default #"overview" { <Overview /> }
    route #"settings"          { <Settings /> }
    <div class="layout"><Outlet /></div>
}
```

### 1.2 Control API

`router("name")` (lowered to `window.$__uni_router("name")`) returns the handle
(or a null-object when the router is absent from the page):

| Method | Behaviour |
|---|---|
| `activateRoute(id)` / `activateRoute(id, params)` | activate; idempotent; returns `bool` |
| `activateRouteByUrl(path)` | match a URL client-side and activate |
| `replaceRoute(id)` / `replaceRouteByUrl(path)` | as above, history `replaceState` |
| `deactivate()` | hide current, keep state |
| `preload(id)` | hydrate without showing |
| `release(id)` | dispose an **inactive** route's instance and re-arm it |
| `isActive(id)` / `current()` / `currentUrl()` | query active state |
| `buildPath(id, params)` | reverse-map a route to a path |
| `normPath(href)` | the router's path normalization |
| `query()` / `setQuery(obj)` | parsed query / replace the query string |

Reactive state lives in **two** signals: `$current` (active **id**) and `$url`
(active resolved **URL**). Read `.value` in JSX to keep links live. They are
split so an id-keyed link does not re-render on a param change (`/p/1 → /p/2`)
while a URL-keyed link does.

### 1.3 Components (`lang/libs/router`)

- **`<RouterLink href="…" router="…" preload>`** — same-origin navigation link.
  Intercepts only plain left-clicks on internal paths (the shared
  `window.$__uni_should_intercept` predicate); ctrl/middle click, `target`,
  `download`, cross-origin, `mailto:`/`tel:`, `#frag` fall through.
  It is named `RouterLink`, **not `Link`**, because the `components` library
  already ships a typography `Link`.
- **`<NavLink href="…" routeId="…">`** — `RouterLink` + active class and
  `aria-current`.
- **`<Outlet />`** — inside a route body with nested `route`s, expands in place
  into the nested router's wrappers.

### 1.4 Server setup (per request)

```chemical
public func handle_request(req : &http::Request, res : &mut http::ResponseWriter) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    page.set_route_url(req.path, "/app")                       // base optional
    page.set_request(&RouteRequest.make(req.method, req.path, req.query, ""))
    #html { <App /> }                                          // router declared inside App
    if(page.route_missing()) { res.status = 404u }            // decide after render
    res.write_string(page.toString())
}
```

The generated router function performs the match *during* render (it owns the
compile-time patterns) and stores the selected id and `{param}`s in the page
parameter store, which drives the server-rendered active wrapper, the route
props, and the client activation tail.

### 1.5 Deployment modes

| Mode | Server router code | Deep links |
|---|---|---|
| Per-request (`net_http`) | `set_route_url` + `toString()` | matched server-side each request; client table for clicks |
| Static export | none | rewrite map from the emitted `<name>.routes.json`; client table selects |
| Hybrid | per-request shell, `remote` heavy routes | shell matched server-side; fragments fetched on demand |

The router library depends on `page` + `std`, **never** on `net`/`tls`/`http`.

---

## Part 2 — How the router is built

### 2.1 Pipeline map (source → emitted page)

| Stage | File(s) | Key symbols |
|---|---|---|
| `#` sigil token | `js_syntax/src/TokenType.ch`, `js_syntax/src/Tokenizer.ch`, `js_cbi_lexer/src/CompilerLexer.ch` | `JsTokenType.Hash` |
| AST nodes | `js_syntax/src/NodeKind.ch`, `js_syntax/src/Ast.ch` | `RouterDecl`, `RouteDecl`, `RouteHook`; `JsRouterDecl`, `JsRouteDecl`, `JsRouteHook` |
| Parser | `universal_parser/src/parser/parser_router.ch`, `parser_stmt.ch` | `tryParseRouterStatement`, `js_parse_router_decl`, `js_parse_route_decl`, `js_parse_route_body` |
| Emission + validation | `universal_cbi/src/router/emit.ch` | `emit_router_server`, `emit_route_server`, `emit_nested_routes`, `router_validate*`, `router_body_is_static`, `router_scan_internals` |
| Hook points | `universal_cbi/src/converter/converter_utils.ch` (dispatch `RouterDecl`/`RouteDecl`), `converter_jsx.ch` (`<Outlet>` interception), `converter_core.ch` (`router(...)` → `window.$__uni_router`), `react/ast_replace.ch` (router-only bodies) | `emit_ssr_single_stmt`, `convertJSXComponent` |
| Support resolution | `universal_cbi/src/sym_res/support_fix.ch`, `js_syntax/src/SymResSupport.ch` | `applyRouteUrlFn` |
| Server store + runtime latch | `page/src/page.ch` | `PageParameter`, `RouteManifestEntry`, `add_parameter*`, `get_parameter*`, `route_selected`, `ensure_router_runtime`, `append_router_initial_activation`, `append_router_table_base`, `emit_route_title`, `emit_route_noindex`, snapshot cache |
| Client runtime (normative JS) | `page/src/router_runtime.ch` | `router_runtime_js()` → emitted once per page |
| Attribute serialization helper | `page/src/ssr.ch` | `SsrText.getSsrAttributeValue`, `page.ssr_string_ptr` |
| Router library | `lang/libs/router/src/` | `match.ch`, `build_path.ch`, `apply.ch`, `params.ch`, `request.ch`, `url.ch`, `Link.ch`, `NavLink.ch`, `Outlet.ch` |

### 2.2 What the converter emits (per router)

Emitted into `pageJs` during render, guarded by the page's component-dedup map so
a router is a page singleton:

1. `page.ensure_router_runtime()` — appends `router_runtime_js()` to `pageJs`
   and the one hide rule `.chx-route[data-uni-route-active="false"]{display:none
   !important;}` to `pageCss`, once per page (zero bytes otherwise).
2. Server matching: `apply_route_url(page, name, spec)` where `spec` is
   `"<id>\t<pattern>\t<is_fallback>\n"` per URL route + fallback, in precedence
   order.
3. Registry: `window.$__uni_routers["name"] = {…}` + `$__uni_router_methods`.
4. Client match table: `window.$__uni_routers["name"].table = { base:"", routes:[…] }`.
5. Per route: the SSR wrapper + boundary span, the route body SSR, and the
   registration stub `window.$__uni_route_register("name","id",{…})`.
6. Manifest entries (`page.add_route_pattern(...)`), per-route `<title>`, 404
   `noindex`, `preload` calls.
7. Activation tail in `pageJsEnd` (after `$__universal_flush()`):
   `$__uni_set_table_base` (URL routers) + `$__uni_activate_initial(...)` +
   `$__uni_sync_url(...)`.

Route bodies are registered and mounted **only by the router** — they never go
through `$__uni_hydration_queue`.

### 2.3 The frozen client runtime (`router_runtime.ch`)

Emitted once; nothing here throws. Key globals:

- `$__uni_routers`, `$__uni_router_queue`, `$__uni_router_busy`,
  `$__uni_router_error` (log-only, never throws), `$__uni_router_version`.
- `$__uni_route_register`, `$__uni_route_props`, `$__uni_route_visible`.
- `$__uni_activate`, `$__uni_activate_now` (historyMode `0 none / 1 push / 2 replace`).
- `$__uni_preload`, `$__uni_release`.
- `$__uni_router` (accessor; returns a null-object when absent),
  `$__uni_router_methods`, `$__uni_should_intercept`.
- URL layer: `$__uni_match_url`, `$__uni_set_url`, `$__uni_sync_url`,
  `$__uni_parse_query`, `$__uni_set_query`, `$__uni_build_path`,
  `$__uni_activate_by_url`, `$__uni_activate_initial`, `$__uni_decode_segment`,
  `$__uni_norm_path`, `$__uni_set_table_base`, `$__uni_initial_url`,
  `$__uni_router_fragment_url`, `$__uni_fetch_route`, `$__uni_mount_fragment`.

Activation highlights (all in `$__uni_activate_now`): exact no-op compare on
`{route, url, rawUrl}`; param-change dispose (`route.url !== url` → dispose +
`ssr=false` + clear host children); guard `beforeActivate` before any mutation;
remote two-step (`pendingActivate` + fetch + re-entry); **hydrate before hiding**
the outgoing route; focus/scroll capture/restore in `route_visible`; two signals
written only when the value actually changes; nested cascade
`$__uni_activate(route.nested, route.nestedDefault, undefined, route.params, 0)`.

### 2.4 Server matching + URL layer

- `router/src/match.ch` — `RoutePattern`, `RouteMatch`, `pattern_segments`,
  `normalize_path_view`, `match_route` (straight first-match scan; precedence is
  resolved at *emission* time).
- `router/src/build_path.ch` — `build_path` (reverse map, percent-encodes params).
- `router/src/apply.ch` — `apply_route_url` (called by generated code; matches,
  percent-decodes into page-owned storage, stores the id + params, or
  `mark_route_missing`).
- `router/src/params.ch`, `request.ch`, `url.ch` — `get_parameter_object<T>`,
  `parse_query`, `RouteRequest`, `set_route_url`/`get_route_url`/`get_route_base`.
- Client matcher `$__uni_match_url` mirrors `match_route` over `r.table`.

### 2.5 Static-route SSR snapshot cache (Phase 7)

- `page.ch` owns a **process-global**, mutex-guarded store
  (`route_snapshots`, `route_snapshot_append`/`route_snapshot_store`), lazily
  constructed (top-level destructible globals are not auto-constructed; a
  zero-init `std::mutex` is a valid default lock on POSIX/Windows).
- `router_body_is_static` (in `emit.ch`) is deliberately **conservative**: only a
  pure native/static subtree (no components, expressions, spreads, hooks, URL
  params) is cached; anything uncertain renders per request. Keys include the
  route's encoded source location.

### 2.6 Diagnostics catalogue (frozen messages)

Emitted by the converter (`emit.ch`) with a source location, except R5 (runtime).

| # | Trigger | Message |
|---|---|---|
| R1 | `route` outside a `router` | `'route' declaration is only valid inside a router block` |
| R3 | duplicate route id | `route '#x' is declared twice in router "m"` |
| R4 | duplicate router name in one body | `router "m" is declared twice` |
| R6 | route body ≠ 1 JSX root | `route body must render exactly one root element` |
| R7 | fallback not last | `fallback route must be the last route` |
| R8 | literal id not declared | `no route 'x' in router "m"` |
| R13 | unsupported pattern (mid `*`) | `unsupported route pattern '…'` |
| R14 | `$__uni_*` in a route body/hook | `route bodies cannot call runtime internals` |

R5 (second registration of a router name) is prevented at emission time by the
page-singleton component-dedup guard; the runtime reports a
`route registered for unknown router` via `$__uni_router_error` (which never
throws).

---

## Part 3 — Testing

| Suite | Command | What it covers |
|---|---|---|
| Router library + server | `./scripts/test.sh --tcc --libs` | matcher, build_path, query, store, `apply_route_url`/deep links, params, decoding, titles/noindex, nested, snapshot cold/warm + 8-thread |
| Compiler emission | `./scripts/test.sh --tcc --plugins` | `router_emission.ch` — SSR wrappers, registry/stubs, modes, hooks, precedence, nested, `RouterLink` |
| Diagnostics | `./scripts/test.sh --tcc --negative` | `router_diagnostics.ch` — one case per R* |
| Behaviour (real WebKit) | `./scripts/test.sh --tcc --universal` | `tests_router.ch` — navigation, state, hooks, guards, URL, links, nested, title, release, preload |
| Both backends | `--llvm --libs` / `--llvm --plugins` | LLVM parity for emission + server tests |

Fast iteration on a single WebView test:

```bash
cmake-build-debug/TCCCompiler lang/tests/universal_webview/chemical.mod \
    -o /tmp/ut_router --mode debug_quick --no-cache -frecompile-plugins
/tmp/ut_router --test-names "router activates the default route and hydrates it"
```

When adding a behaviour:
1. If it needs the real engine, add a `#universal_test` in
   `lang/tests/universal_webview/src/tests_router.ch`, scoping state to the
   container and using `unique router names` (registries are global on the shared
   page) or `isolate` for history/global/fetch tests.
2. If it is pure server logic, add a `@test` under `lang/tests/libs/router/src/`.
3. If it is a compile error, add a case to `lang/tests/negative/src/router_diagnostics.ch`.

> Harness gotcha: `expect(obj)` builds its failure message with
> `JSON.stringify(actual)` **eagerly**, so never pass a cyclic object (e.g. a
> router record) to `expect`; compare a primitive instead.

---

## Part 4 — Invariants and gotchas (don't regress)

- **One visible route per router** after every activation; `visible` is
  maintained by `$__uni_route_visible` (the only mutator).
- **Deactivation never disposes.** Mount happens before the outgoing route is
  hidden, so a mount failure leaves the previous route visible.
- **The wrapper is never the mount host.** The `[data-chx-i]` span inside the
  wrapper is the host and is mounted in `"children"` mode; the wrapper only gets
  its `data-uni-route-active` attribute toggled (so a non-`<div>` root is safe).
- **No DOM queries on the activation path** — boundaries/wrappers are resolved at
  bootstrap by source-derived ids.
- **Route bodies never enter the hydration queue**; only the router mounts them.
- **Signals are written only when the value changes** (`$current`, `$url`).
- **`baseProps` is immutable**; params are merged per mount by
  `$__uni_route_props`.
- **Nothing throws**: all recoverable faults go through `$__uni_router_error`
  (log + `return false`).
- **Zero cost when unused**: pages that do not declare a router emit no runtime
  bytes.
- Keep every emitted JS byte **ASCII** (a byte ≥ 0x80 breaks `std::string::find`).
- **Enum-sync rule**: if you ever add a token that is mirrored in C++
  (`TokenType.h`↔`ChemicalTokenType.ch`), update both. `JsTokenType`,
  `JsNodeKind` are plugin-only — append at the **end**.

---

## Part 5 — Known limitations / remaining work

- **Phase 6 remainder**: full URL nesting / segment-prefix matching
  (`/projects/{id}/settings`); an `<Outlet/>` supplied by a *separate layout
  component*; a **hydrated outer layout**. The last two need route bodies to
  compile to an anonymous client function whose Outlet participates in client
  hydration.
- **R11/R12**: `props.X` / `dangerouslySetInnerHTML` validation — these read the
  route *component*, a different component from the router declaration, so they
  need a cross-component pass.
- **Site-level rewrite-map aggregate** (deploy tooling) from the per-page
  `<name>.routes.json`.
- Phase 7 caches only the conservative static subset (by design).

---

## Change-impact map (update these together)

| If you change… | Also update… |
|---|---|
| Syntax/keywords/grammar | `parser_router.ch`, this skill §1.1, the design doc §4/§14.1, `router_emission.ch`/negative tests |
| Emitted artefacts (wrapper, registry, stubs, tail, table) | this skill §2.2/§2.3, design doc §15.3, `router_emission.ch` |
| Runtime symbols/behaviour | `router_runtime.ch`, this skill §2.3/§4, `tests_router.ch`, design doc §15.2 |
| A diagnostic message | `emit.ch`, `router_diagnostics.ch`, this skill §2.6, design doc §14.8 |
| Page server API (`page.ch` router methods) | this skill §2.4/§2.5, `lang/tests/libs/router/*`, design doc §3/§15.4 |
| Router library public API (`match`/`apply`/components) | this skill §1.3/§1.5, `lang/libs/router/README.md`, `--libs` tests |
| Snapshot cache / classifier | this skill §2.5, `snapshot.ch`, design doc §7.5/§16 |
| An invariant | this skill §4, design doc §15.5 |
| **Anything above** | **THIS SKILL** — a router change without a skill update is incomplete |
