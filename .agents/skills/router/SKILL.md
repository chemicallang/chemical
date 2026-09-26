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
> design doc disagree, the code wins; fix the skill and the doc.** This skill is
> also the app-author quick reference (there is no `lang/libs/router/README.md`;
> library sources ship comment docs above the code, not Markdown).

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
    route "/settings"          { <Settings /> }   // nested URL child: /projects/{id}/settings
    <div class="layout"><Outlet /></div>
}
```

Nested URL children use **relative** patterns; the full path is the ancestor
patterns concatenated (`/projects/{id}` + `/settings`). A deep link selects the
outer layout *and* the nested child on the server and on the client, and the
ancestor `{param}` values are inherited by the nested child (`props.id`).
A nested URL child also works under a top-level **id** layout. A nested
`route *` catches an unknown remainder under the layout (e.g.
`/projects/{id}/unknown`) and is still shadowed by any exact child.

A **native-rooted** layout (root is a native element with an inline `<Outlet/>`)
is hydrated: its own markup is interactive and its DOM survives a child switch.
A **component-rooted** layout that forwards `{props.children}`
(`<Layout><Outlet/></Layout>`) is hydrated too. An `<Outlet/>` written inside a
layout component's *own body* is supported as well: the `Outlet` component
renders a `data-uni-outlet` slot and the runtime relocates the SSR'd nested
wrappers into it when the layout activates.

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

| Mode | Server router code | Deep links | Cheapest when |
|---|---|---|---|
| Per-request (`net_http`) | `set_route_url` + `toString()` | matched server-side each request; client table for clicks | routes depend on request data (auth, personalization) |
| Static export | none | rewrite map from the emitted `<name>.routes.json` → page `.html`; client table selects | all routes static (recommended default) |
| Hybrid | per-request shell, `remote` heavy routes | shell matched server-side; `remote` fragments fetched on demand | a few dynamic routes, several heavy static ones |

All three use the same declarations and runtime; only the handler and the emit
call differ.

Components (`RouterLink`, `NavLink`, `Outlet`) and the matcher live in the
`router` library; `page` owns only the generic parameter store and the client
runtime emission. The library depends on `page` + `std`, **never** on
`net`/`tls`/`http`.

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
`{route, url, rawUrl}` (so a same-path query change still re-runs); param-change
dispose (`route.url !== url` → dispose + `ssr=false` + clear host children, but
**never** for a `nested` layout); guard `beforeActivate` before any mutation;
remote two-step (`pendingActivate` + fetch + re-entry); **hydrate before hiding**
the outgoing route; focus/scroll capture/restore in `route_visible`; two signals
written only when the value actually changes; nested cascade
`$__uni_activate(route.nested, route.nestedDefault, undefined, route.params, 0)`.

URL-layer guards: `$__uni_norm_path` strips a `#fragment` **and** the query (in
that order) plus one trailing slash, so matching and link-active comparison never
see a hash; `$__uni_should_intercept` rejects an href containing `#` (hash
scrolling is a plain anchor) and one whose second char is a backslash (WHATWG
treats `\` as `/`, so `/<backslash>host` is protocol-relative); `.`/`..` never
match a `{param}` on either the client (`$__uni_match_url` decodes + checks) or
server (`match_route`'s `is_traversal_segment`) matcher, in literal **or**
percent-encoded (`%2E`) form (D-6.1). The server decodes a short `%`-bearing
segment only to test it and still returns raw views, so `apply_route_url` keeps
ownership of decoding.

### 2.4 Server matching + URL layer

- `router/src/match.ch` — `RouteParam`, `RouteChainStep` (`reg`,`id`),
  `RoutePattern` (segments/id/is_fallback/prefix/chain), `RouteMatch` (+`chain`),
  `pattern_segments`, `normalize_path_view`, `match_route` (straight first-match
  scan; exact entries precede prefix entries, which precede the fallback, and the
  matched chain is copied into the result). `prefix` marks a nested-fallback
  entry that matches the pattern plus any remaining segments.
- `router/src/build_path.ch` — `build_path` (reverse map, percent-encodes params).
- `router/src/apply.ch` — `apply_route_url` (called by generated code; matches,
  percent-decodes into page-owned storage, stores the id + params, stores each
  activation-chain step under its derived registry, or `mark_route_missing`);
  `parse_route_chain` decodes the `reg` US `id` (RS separated) chain field.
- `router/src/params.ch`, `request.ch`, `url.ch` — `get_parameter_object<T>`,
  `parse_query`, `RouteRequest`, `set_route_url`/`get_route_url`/`get_route_base`.
- Client matcher `$__uni_match_url` mirrors `match_route` over `r.table` and
  returns the entry's `chain`; `$__uni_build_path` resolves both a top-level
  entry id and a nested entry whose chain ends with the requested id.

**Nested URL ownership (§6.4, §13.3.3).** Nested URL routes are emitted into the
*outermost* match table as **full accumulated patterns** with a `chain`
(`id` = the root layout route; each step = `[derivedRegistry, routeId]` down to
the leaf). The server spec carries the same chain (control-char encoded) so
`apply_route_url` stores every level's selection; `$__uni_activate_now` follows
the chain instead of `nestedDefault` and passes the remaining steps down. This
is deliberately *not* segment-prefix matching: the pattern grammar is literal +
`{param}` segments, so full patterns are equivalent and keep one exact scan.

**Hydrated layout.** A route with nested children whose body root is a
native element gets an anonymous client function (`$__uni_route_layout_<loc>`)
emitted by `emit_route_layout_client`; the route stub's `comp` points at it. In
the client vnode tree its `<Outlet/>` becomes `$_ur.createElement("__uni_outlet",
null)`. The universal runtime (`page.ch`) treats `v.t === "__uni_outlet"` as an
**opaque boundary**: `$__uni_hydrate_node` consumes consecutive `.chx-route`
siblings (the SSR'd nested wrappers) and returns the node after them, and `$_urn`
renders nothing. So the layout hydrates in place while the nested router keeps
sole ownership of the wrappers. A **component-rooted** layout that forwards
`{props.children}` gets `comp` = the root component and a generated
`$__uni_route_children_<loc>()` vnode array as the route record's `children`
(merged into props by `$__uni_route_props`); `emit_route_children_client`
converts the root's children with the outlet boundary. An **`<Outlet/>` inside a
layout component's own body** (no route context) renders a `data-uni-outlet` slot;
the runtime relocates the nested registry's wrappers into the nearest slot inside
the activating route's host (they are server-rendered after the layout, so deep
links still include the child content — the visual position is corrected at
hydration).
`baseProps` also carries a route root component's compile-time attributes
(D-2.7), not just `{param}` placeholders.

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
All are **errors** except R10, which is a **warning** (the CBI
`ASTDiagnoser.warning` channel; see the change-impact map). R3/R6/R7/R8/R9/R10/
R11/R12/R13/R14 apply recursively to nested routers under their derived name
`parent#id` (R11/R12 use the *accumulated* pattern). R11/R12 read the route root
component's parsed JS body via `ComponentSignature.js_body` (set by the
`#universal` macro).

| # | Trigger | Message |
|---|---|---|
| R1 | `route` outside a `router` | `'route' declaration is only valid inside a router block` |
| R3 | duplicate route id | `route '#x' is declared twice in router "m"` |
| R4 | duplicate router name in one body | `router "m" is declared twice` |
| R6 | route body ≠ 1 JSX root | `route body must render exactly one root element` |
| R7 | fallback not last | `fallback route must be the last route` |
| R8 | literal id not declared | `no route 'x' in router "m"` |
| R9 | two same-shape URL patterns | `route patterns '/a/{x}' and '/a/{y}' are ambiguous` |
| R10 | router with no `default` and no `*` (warning) | `router "m" has no default route; the page renders inert without a server parameter` |
| R11 | route-root component `props.X` read not covered by a root attribute or pattern param | `route prop 'x' is not declared: not an attribute of the route root and not a param of '/a/{y}'` |
| R12 | `dangerouslySetInnerHTML` fed a route param | `route params must not be injected as raw HTML` |
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
| Behaviour (real WebKit) | `./scripts/test.sh --tcc --universal` | `tests_router.ch` — navigation + exactly-one-visible (INV-1), O(1) no-op re-activate + change-only signals (INV-11), unknown-id/error containment (INV-3/INV-15), `deactivate`, multi-router independence, null-object accessor, hydration (`preload`/`lazy`, effects-once, only-default+preload hydrated), `release` re-arm, `noscroll`, focus restore, hook ordering/guard allow+deny/error isolation/re-entrancy queue, link `aria-current` (+ param-aware `$url`), `NavLink`, preload-on-hover, the `$__uni_should_intercept` matrix, URL client match (percent-decode, `%2F`/`+`, trailing slash, base), query/`setQuery`/`buildPath`, `replaceRoute(byUrl)`, activateRoute-with-params, param-change remount (INV-2/INV-10), `popstate` fallback + guard-back re-sync, remote failure containment (INV-21), nested layout state + independent signals, title/base-title, and the non-div hide rule (INV-8). **Phase 2 hardening:** history push/replace/no-op dispatch (stubbed `pushState`/`replaceState`), `$url`+`$query` change-only fires, query decode parity (`+` literal, bare key, unicode, malformed percent), `setQuery` encode+clear+path preservation, `buildPath` percent round-trips (`%2F`/space/`%`/unicode), duplicate/trailing slash normalization, `normPath` fragment+query stripping, `set_table_base` (match/`buildPath`/`currentUrl`), fragment-bearing activation URLs, guard flag deny→allow, `.`/`..` never a param, nested param inheritance + remount (`/p/1/x/a → /p/2/x/c`), `activate_initial` chain vs id-default fallback, remote prefetch store / in-flight dedup / blocked release / successful-fragment activation, `isActive` on a URL pattern id + id activation nulling `$url`, nested-deactivate state preservation, `%2E`-encoded traversal rejection, `buildPath` for the fallback id (client `null`), preload/release of a never-hydrated route, and relative-href rendering + non-interception |
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
- **A layout with nested routes is never disposed on a param change** (the
  `route.url !== url` dispose is guarded by `!route.nested`): its DOM contains
  the nested wrappers, and the activation chain re-derives the child from the
  shared URL (§13.3.3).
- **Nested URL entries are full patterns with a chain.** The outermost table is
  the single source of truth for URL matching; nested registries hold no table.
- **Exact entries always precede prefix entries**, which precede the fallback, in
  both the server spec and the client table (`router_entry_precedes`), so a
  nested `route *` never shadows an exact child.
- **A native layout's `<Outlet/>` is an opaque `__uni_outlet` boundary**;
  `$__uni_hydrate_node` consumes the SSR'd `.chx-route` wrappers and leaves them
  for the nested router. `$__uni_route_props` never invents keys: `baseProps`
  must carry every compile-time root attribute and a placeholder for every
  pattern param (D-2.7/INV-20).
- **A separate-component `<Outlet/>` renders a `data-uni-outlet` slot**; on
  activation the runtime relocates the nested registry's wrappers into the
  nearest slot inside the route host. Never SSR the nested wrappers *inside* such
  a slot (the layout component is compiled independently); the relocation is the
  contract.
- **No DOM queries on the activation path** — boundaries/wrappers are resolved at
  bootstrap by source-derived ids.
- **Route bodies never enter the hydration queue**; only the router mounts them.
- **Signals are written only when the value changes** (`$current`, `$url`,
  `$query`); an empty query compares equal to `null`, so `query()` stays `null`
  until the URL actually carries a query.
- **A fragment never participates in routing.** `$__uni_norm_path` strips `#…`
  (then the query) and `$__uni_should_intercept` falls through for any href with a
  `#`; `/<backslash>` (protocol-relative in WHATWG) also falls through. `.`/`..`
  (literal or `%2E`-encoded) are rejected as params on both matchers (D-6.1).
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

- **`buildPath` for the fallback id diverges by design.** The server
  `build_path` reverses `route *` to `/`; the client `$__uni_build_path` skips
  fallback entries and returns `null` with a contained error. Both behaviours are
  pinned by tests (`match.ch` + `tests_router.ch`); pick an explicit route id when
  you need a real reverse path.
- **SSR position of a separate-component outlet.** When the `<Outlet/>` lives in
  a layout component's own body, the nested wrappers are server-rendered *after*
  the layout and relocated into the slot at hydration, so the child content is
  present in the HTML (crawler-visible) but its pre-hydration DOM position is
  after the layout rather than inside the slot.
- Phase 7 caches only the conservative static subset (by design).

Everything else in the design is implemented: full URL nesting, nested
fallbacks, all R1–R14 diagnostics (R10 as a warning), both hydrated-layout forms,
the separate-component outlet, and the site-level rewrite-map aggregate
(`page::site_routes_aggregate` / `write_site_routes`).

---

## Change-impact map (update these together)

| If you change… | Also update… |
|---|---|
| Syntax/keywords/grammar | `parser_router.ch`, this skill §1.1, the design doc §4/§14.1, `router_emission.ch`/negative tests |
| Emitted artefacts (wrapper, registry, stubs, tail, table) | this skill §2.2/§2.3, design doc §15.3, `router_emission.ch` |
| Runtime symbols/behaviour | `router_runtime.ch`, this skill §2.3/§4, `tests_router.ch`, design doc §15.2 |
| Hydrated layout / `__uni_outlet` (universal side) | `converter_jsx.ch`, `emit.ch` (`emit_route_layout_client`/`emit_route_children_client`), `router_runtime.ch` (`$__uni_route_props` children + slot relocation), `page.ch`'s `$__uni_hydrate_node`/`$_urn`, `Outlet.ch` (slot), the `universal` skill, this skill §2.4/§4/§5 |
| Site rewrite aggregate | `page.ch` (`site_routes_aggregate`/`write_site_routes`), `lang/tests/libs/router/src/site.ch`, this skill §5 |
| A diagnostic message | `emit.ch`, `router_diagnostics.ch`, this skill §2.6, design doc §14.8 |
| A route-root-component prop rule (R11/R12) | `emit.ch` (`router_collect_prop_reads`/`router_validate_props`), `html_comp/ast.ch` (`ComponentSignature.js_body`), `universal_cbi/src/react/macro.ch`, `router_diagnostics.ch`, this skill §2.6 |
| The CBI diagnoser channel (e.g. adding `warning`) | `compiler/cbi/bindings/ASTDiagnoserCBI.{h,cpp}`, `CBI.cpp`'s `ASTDiagnoserSymMap`, `lang/libs/compiler/src/ASTDiagnoser.ch`, `lang/tests/negative/src/main.ch` (`expect_compile_output_contains`), this skill §2.6 |
| Page server API (`page.ch` router methods) | this skill §2.4/§2.5, `lang/tests/libs/router/*`, design doc §3/§15.4 |
| Router library public API (`match`/`apply`/components) | this skill §1.3/§1.5, the library's comment docs above the code, `--libs` tests |
| Snapshot cache / classifier | this skill §2.5, `snapshot.ch`, design doc §7.5/§16 |
| An invariant | this skill §4, design doc §15.5 |
| **Anything above** | **THIS SKILL** — a router change without a skill update is incomplete |
