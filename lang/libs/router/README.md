# `router` — server-rendered, lazy-hydrating universal router

An id-based route activation system for Chemical universal components, with an
opt-in URL layer. See the full design in
[`lang/docs/universal-router-design.md`](../../docs/universal-router-design.md).

The core primitive is imperative and cheap — `router("name").activateRoute("id")`
— and URLs, query parameters, history and guards are layers above it. Activation
is "deactivate + activate" (hide/show); a route is never destroyed on navigation,
so state, scroll and focus survive.

## Quick start

```chemical
#universal App(props) {
    router "main-router" {
        route default #"dashboard" { <Dashboard /> }
        route #"projects" { <Projects /> }
        route * { <NotFound /> }
    }
}
```

```chemical
#universal Nav(props) {
    return <nav>
        <NavLink href="/" routeId="dashboard">Dashboard</NavLink>
        <NavLink href="/projects" routeId="projects">Projects</NavLink>
    </nav>
}
```

## Route syntax

| Form | Meaning |
|---|---|
| `route #"id" { <X/> }` | id route (no URL semantics) |
| `route default #"id" { <X/> }` | declared default (used when no server parameter selects one) |
| `route * { <X/> }` | fallback (matches any unmatched URL) |
| `route "/a/{b}" { <X/> }` | URL route with a `{param}` segment |
| `route #"id" preload { <X/> }` | hydrate at load while hidden |
| `route #"id" lazy { <X/> }` | SSR only; hydrate on first activation (default for hidden routes) |
| `route "/x" remote { <X/> }` | no SSR body; fetch the fragment on first activation |
| `route #"id" noscroll { <X/> }` | do not restore scroll on activation |
| `route #"id" title "Title" { <X/> }` | server `<title>` + client `document.title` |

Hooks: `onActivate(() => { ... })`, `onDeactivate(() => { ... })`,
`onBeforeActivate(() => { ... })` (returning `false` cancels the navigation).

## Control API

`router("name")` returns the router handle (or a null-object when the router is
absent from the page):

| Method | Behavior |
|---|---|
| `activateRoute(id)` | activate, idempotent; returns bool |
| `activateRouteByUrl(path)` | match a URL client-side and activate |
| `replaceRoute(id)` / `replaceRouteByUrl(path)` | as above with history `replaceState` |
| `deactivate()` | hide the current route, keep its state |
| `preload(id)` | hydrate without showing |
| `release(id)` | dispose an **inactive** route's instance and re-arm it |
| `isActive(id)` | bool |
| `current()` / `currentUrl()` | active id / resolved URL |
| `buildPath(id, params)` | reverse-map a route to a path |
| `normPath(href)` | the router's path normalization |
| `query()` / `setQuery(obj)` | parsed query object / replace the query string |

Reactive state lives in `$current` (active **id**) and `$url` (active resolved
**url**) signals; read `.value` in JSX to keep links live.

## Server setup

```chemical
public func handle_request(req : &http::Request, res : &mut http::ResponseWriter) {
    var page = HtmlPage()
    page.defaultUniversalSetup()
    page.set_route_url(req.path, "/app")           // optional base
    page.set_request(&RouteRequest.make(req.method, req.path, req.query, ""))
    #html { <App /> }
    if(page.route_missing()) { res.status = 404u }
    res.write_string(page.toString())
}
```

Components (`Link`, `NavLink`) and the matcher live in this library; `page` owns
only the generic parameter store and the client runtime emission. The library
does **not** depend on `net`/`tls`/`http`.

## Deployment modes

| Mode | Server router code | Deep links | Cheapest when |
|---|---|---|---|
| Per-request (`net_http`) | `set_route_url` + `toString()` | matched server-side every request; client table for clicks | routes depend on request data (auth, personalization) |
| Static export | none | rewrite map from the emitted `<name>.routes.json` → page `.html`; client table selects the route | all routes static (recommended default) |
| Hybrid | per-request shell, `remote` heavy routes | shell matched server-side; `remote` fragments fetched on demand | a few dynamic routes, several heavy static ones |

All three use the same declarations and runtime; only the handler and the emit
call differ.

## Status

Implemented: Phases 0–5 (syntax/codegen, parameter store, client runtime, server
matching, params, URL table, query, titles, manifest, precedence) and Phase 6
core (nested `route` + inline `<Outlet />`, derived nested registries, cascade
activation). Not yet implemented: URL nesting / segment-prefix matching, an
`<Outlet />` supplied by a separate layout component, and the static-route SSR
snapshot cache (Phase 7). See the design doc §16 for the exact state and known
divergences.
