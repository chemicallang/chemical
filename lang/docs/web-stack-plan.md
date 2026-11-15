# Web application stack — master plan (v2)

**Status:** proposed for approval · **Date:** 2026-11-15 · **Supersedes:** v1 of this file
**Scope:** make `#html` / `#css` / `#js` / `#universal` / `router` / `page` a production
stack for large, multi-platform applications (timeline, 123mind), with the
**JavaScript backend as job #1**.
**Companions:** [`proposals/0007-javascript-backend.md`](proposals/0007-javascript-backend.md) (approved),
[`proposals/0008-web-dom-types.md`](proposals/0008-web-dom-types.md),
[`universal-runtime-professionalization-plan.md`](universal-runtime-professionalization-plan.md),
[`universal-shared-js-bundle-design.md`](universal-shared-js-bundle-design.md),
[`universal-router-design.md`](universal-router-design.md),
[`language-goals.md`](language-goals.md).

**Changes from v1:** no `@server`/`@client` annotations (three real modules, `shared`/`server`/`client`);
an in-memory app/serve model instead of "build an executable that dumps a site"; **three delivery
modes** (static / shell+slots / full SSR) with frozen-shell slot injection for server-time
performance (§3.12); **derived per-route payload tiers** (§3.13); **`--static` MPA builds that share
JS across pages** (§3.14); **typed `#universal` props + hydration-safety analysis** (§3.15);
compilation-performance foundations (§3.16); the explicit breaking-change list (§3.17); the four
acceptance apps as a requirement matrix (§3.18); and **probes with pass criteria** (§3.19).
Plus: corrected measurements; the JS-backend ABI decided before the emitter; the review's cheap
wins moved earlier; a cut list.

---

## 0. Verdict (measured)

The stack is excellent at its design centre: **render the page per request on the server**,
with `#html` static markup, compile-time-scoped `#css`, hydration via `#universal`, and a
server-rendered, lazy-hydrating router. Static sites and server-rendered apps are served well.

It is not yet a stack for large client-heavy apps. Measured on the real
`lang/compiled/components` app (two pages, same component library), content-based:

| measurement | `components.js` | `try.js` | note |
|---|---|---|---|
| total | 320,163 B | 180,659 B | one page bundle each |
| hydration runtime | 92,887 B (**29%**) | 92,887 B (**51%**) | byte-identical copy per page |
| per-instance dispatch text | 133,073 B (**42%**, 805 calls) | 28,280 B (**16%**, 177 calls) | `window.$__uni_dispatch(..., props)` + `getElementById` |
| shared component defs | 63 names, **61 byte-identical**, 45,744 B | same | re-sent per page |
| `import` / `export` / `import()` | 0 / 0 / 0 | 0 / 0 | one concatenated script |
| source maps | none | none | |
| asset names | fixed (`components.js`) | fixed | no content hash |
| minify headroom (comments+whitespace) | −15% raw / −24% gzip | −26% raw / −35% gzip | no minifier exists |
| `attach_shared` used by this app | no | no | the default path duplicates everything |

Corrections to v1 of this plan: the two pages are *not* "sharing nothing" — they share
byte-identical content at different offsets; the runtime is 92,887 B (not ~32 KB); the
minifier numbers above are headroom, not retained size. The dominant cost is **per-instance
dispatch text (42%)**, which the hydration manifest removes without any JS backend.

The real gaps are structural:

1. **No code units.** Client JS is text appended to `pageJs` in emission order, with
   `move_js_range` memmove hoisting. There is no "module", "chunk", "route bundle", or "lazy
   boundary".
2. **No shared bundle by default.** `SharedAssets` is opt-in and only de-duplicates bytes
   appended to one string; 63 components are still re-sent per page in the real app.
3. **No production output.** No minification, tree-shaking, chunking, hashing, or source maps.
4. **No typed client code.** `#js` is raw unchecked JS, so the one place large apps live —
   client logic — is where the language's guarantees stop. This is why agents work around it.
5. **Page is the only build unit.** The artifact is "(page).html/.js/.css"; there is no
   app-level build, no route→bundle optimisation, no in-memory serving story.
6. **Per-instance dispatch text and call-site hydration ids.** 42% of a page is dispatch
   text; ids come from source locations, so two instances of one node collide.
7. **No server-time option between "static" and "render it all".** Serving a prebuilt page means
   returning one captured string; personalising it means re-running the whole component tree. No
   slot/partial-render model exists (§3.12), so large apps pay full SSR cost or give up
   personalisation.
8. **Props are untyped.** Nothing checks a component's props at the call site, and prop
   serialisation is hand-built, so bad values surface in the browser (§3.15a).

---

## 1. What exists and must keep working

| Piece | State | Keep |
|---|---|---|
| `#html` SSR | mature: escaping, attributes, `@{}`, `#styled` | output byte-compatible |
| `#css` / `#globalcss` / `style {}` | deterministic hashed classes, nesting, media, keyframes | as-is |
| `#js` / `#globaljs` | raw JS, page-local / shared | as the escape hatch |
| `#universal` | SSR + hydration, hooks, context, portals, error boundaries, keyed lists | as-is |
| `router` | server-rendered, id-based, nested routes, URL layer; **remote routes already split** (no body until hover/visit, with race handling) and **`document.title` already updates on activation** | as-is, extend |
| `SharedAssets` | opt-in sink; dedups runtime/defs/CSS | frozen + hashed in Phase 2, replaced in Phase 4 |
| `HtmlPage` buffers | `pageHead/Html/Css/Js` | retained as the render primitive |
| Components library | **196 `#universal` declarations** in 23 files | migrated onto chunks |
| `#universal_test` + Playwright `components-e2e` | real-browser verification | extended |

Invariant: a page using none of the new features produces the same HTML, and a static page
still ships no JS.

---

## 2. Root causes (revised)

1. **Client JS is a by-product of SSR emission** — there is no separate compile unit, so
   nothing can be named, hashed, split, cached, or tree-shaken.
2. **No JS backend**, so client logic can only be untyped JS text; there is no second
   representation to build a module graph from.
3. **The runtime is an escaped string inside `page.ch`** (108 KB of source), untestable as an
   asset and unshippable with versioning/hashing.
4. **The page is the only build unit**; bundle composition is never computed, so it cannot be
   optimised or served automatically.
5. **Per-instance dispatch text** (42% of the page) and **call-site hydration ids** (collide
   for repeated instances) are architectural, not cosmetic.
6. **The only per-request choices are "render everything again" or "return a string captured at
   startup"** (§3.12). There is no cheap middle: no slot, no partial render, no per-request
   injection. So pages are either fully dynamic and slow, or fully static and unpersonalised.

*(`move_js_range` hoisting is a consequence of (1), and is already a no-op on the shared-sink
path — a Phase 3 cleanup, not a root cause.)*

---

## 3. Target architecture

### 3.1 Client/server split by module graph (no annotations)

The split is **structural**: folders, each a **real module with its own `chemical.mod`**, wired by
conditional imports. Directories under one `chemical.mod` would *not* work — `source` merges
every directory into one flat module scope with no import edges, so nothing could be checked.

```
timeline/
  chemical.mod                    # the app: picks sides by flag
  shared/chemical.mod             # models, validation, pure logic  (native + js)
  server/chemical.mod             # routes, handlers, db, SSR assembly, UI  (native only)
  client/chemical.mod             # DOM code, client stores, typed fetches  (js only)
```

```chmod
# timeline/chemical.mod
application timeline

import shared
import server if server
import client if client
```

```chmod
# shared/chemical.mod
module shared
source "."
import std
import json
```
`shared` is deliberately restricted: **no `page`, no `dom`, no HTML/CSS/JS macros** — models and
pure functions only, so the JS backend can compile it verbatim. `#universal` components live in
`server` (they are macros producing SSR *and* a client unit; compiling them with the JS backend
is meaningless).

`chemical build --web` runs two jobs:

| job | flags | modules compiled | backend | output |
|---|---|---|---|---|
| server | `server` | `shared` + `server` | C / LLVM | executable |
| client | `client`, `js` | `shared` + `client` | **JS backend** | ESM units + chunk graph |

Enforcement is now real, because these are import edges:

- `shared` may not import `server`/`client` (module graph).
- `client` may not import `server`, and may not reference a native-only symbol
  (`MIRTargetSupport`, §3.7).
- `server` may not import `client`.
- A module compiled by one job may not require a module the other job does not compile
  (checked per job; the compiler knows the module set).

New target flags `server`/`client`/`js` cost three small edits (`TargetData.h`,
`lang/libs/lab/src/target_data.ch`, `LabJob::is_condition_enabled`), and two jobs over the same
app already work: each job has its own build dir (`set_build_dir`) and `built_files` cache.


### 3.2 Three sources of client JS, one graph

| source | produced by | example |
|---|---|---|
| universal component bodies | `universal_cbi` (today: string; Phase 3: units) | `#universal Button(props) { ... }` |
| client modules | **JS backend** (Chemical → ESM) | `client/editor.ch` |
| raw blocks | `#js` / `#globaljs` (escape hatch) | hand-written interop |

The bundler treats all three as units in one graph, so sharing, splitting, minification and
source maps apply uniformly.

### 3.3 The app is declared once; bundles are derived

Today the author must warm a `SharedAssets` sink by hand, then assemble the document from
`shared.js()` + `getHtml()` + `local_js()`, then write files. Replace it with a declarative app
in a **new `web` module** (own library; `http` already has an unrelated `web` namespace, so do
not graft this onto `http`).

```chemical
// server/app.ch  (native)
var app = web::App.make()
app.layout(RootLayout)
app.page("/",              Home)          // Home(page : &mut HtmlPage)
app.page("/projects/{id}", ProjectPage)
app.page("/settings",      Settings)
app.not_found(NotFound)

var bundle = web::Bundle.load_or_embedded()
app.use(bundle)

http::Server.make().bind("0.0.0.0", 8080).handle(app).start()
```

- A route handler is an ordinary `func (page : &mut HtmlPage, req : &Request)` — the same
  function an author writes today. No new macro is required for SSR.
- `app.render(path, req) -> Rendered { status, html, head, chunks }` renders **in memory**.
- `app.handle(req, res)` matches the route, renders, emits `<script>`/`<link>` from the bundle,
  and serves `/assets/<file>` from it with ETag + immutable caching.
- `http::Server.dispatch` is already a native `Request -> Response`; `web::App` needs it
  **public** and a listener-free form (today `Server` owns a socket + thread pool).

**Deriving the chunk graph (the missing piece).** A native route table cannot know which
components a route renders, so the closure is *measured*, not analysed:

1. At build time (`--web`) or at first start, render every route once into a throwaway
   `HtmlPage`. The existing `require_component`/`require_css_hash`/`require_js_hash` calls
   record exactly which units the route touched.
2. That id set maps to units by name (the JS build knows unit contents), producing
   route → units.
3. Units used by ≥ 2 routes go to `vendor`; each route keeps its remainder; the graph is
   **frozen**.

This reuses the mechanism that already exists for `SharedAssets`, removes the manual warm pass
from the user, and makes the closure check possible (a route that dispatches an unknown id is a
build error). It is a deliberate trade: derivation is by rendering once, so a component used only
behind a runtime branch that no initial render takes is missed — the closure check catches the
dispatch at runtime and the build can be told to include extra units per route.

Escape hatches stay: `HtmlPage` directly, `page.writeToDirectory` for static export, `#html` for
hand-assembled markup.


### 3.4 Deployment: embedded by default, disk optional

| mode | how | when |
|---|---|---|
| **embed** (default) | build emits `assets.ch` — **one string literal per text chunk**, base64 for binary — compiled into the server binary; `web::Bundle.load_embedded()` | simplest deploy: one binary, atomic, no missing-asset bugs |
| **disk** | build writes `dist/assets/*` + `manifest.json`; `Bundle.load(path)` | CDN / separate static host / dev |
| **dev** | `chemical serve` keeps chunks in memory and re-emits on change | authoring |

Two hard constraints from the existing code:

- **One string literal per chunk.** `page.ch` already embeds the 92 KB runtime as a single
  `"""…"""` literal; that is the proven mechanism. 300 KB of `[N]u8` array elements would be
  ~300k AST nodes per compile — not viable. Binary assets are base64 inside a string, or stay on
  disk.
- **Two-phase build.** Chunks are a compile *output*, so `assets.ch` cannot be produced and
  consumed in one pass. `chemical build --web` orchestrates: (1) build the client job, emit
  chunks; (2) generate `assets.ch`; (3) build the server job. A dedicated ordered job kind is
  the alternative if that proves awkward.

`lab` today exposes only `lab::run`; build scripts write files with `fs::write`/`cstd`, not
`lab::write_file` (AGENTS.md is stale on this).


### 3.5 Code units and chunk graph

```
Chemical module ──MIR──> JS backend ──> app/<module>.js   (ESM)
universal component ──macro──> unit ──> comp-<Name>.js
                     │
        chunk graph (reachability + dynamic boundaries + route table)
                     │
  runtime.<hash>.js   vendor.<hash>.js   route.<id>.<hash>.js   page.<id>.<hash>.js
  styles.<hash>.css   route.<id>.<hash>.css   assets/<name>.<hash>.<ext>
                     │
        manifest.json ──> <script type="module"> + modulepreload + <link rel=stylesheet>
```

Invariants (CI-gated, Phase 4):

- every unit appears in exactly one chunk (or is referenced by two without being duplicated);
- every hydration id is reachable from the entry (closure check — closes today's "incomplete
  warm" hole);
- a request-time render cannot add a unit to a frozen chunk;
- no inline `<script>` in production output (CSP-friendly), no `window`-global registration.

### 3.6 Hydration manifest (moved early; no JS backend needed)

Replace per-instance dispatch text with one payload:

```html
<script id="__chc" type="application/json">
{"c":[{"i":"u7","n":"Button","p":{...}},{"i":"u8","n":"Card","p":{}}]}
</script>
```

- Removes ~42% of `components.js` (133 KB on the real page) and 805 `getElementById` lookups.
- Ids become **instance ids from a counter** (`useId`), not source locations, so repeated
  instances cannot collide.
- Props are serialized once, and the client runtime hydrates from the manifest; the queue /
  `window[fnName]` mechanism is replaced by chunk imports (Phase 4) or a registry (Phase 2).

This is the single largest cheap win and depends on nothing else.

### 3.7 Typed client code (why the JS backend is job #1)

After the backend, the `client/` module is ordinary Chemical compiled to ESM:

```chemical
// client/counter.ch   (compiled by the JS backend)
public func increment(count : &mut int) {
    count = count + 1
    document.set_text("#count", int_to_string(count))
}
```

`document`, `fetch`, `Event`, `localStorage` come from proposal 0008's **`dom`** module, so the
call is checked. No `#js`, no string typing. Target support is a **symbol/type property**, not
a per-opcode gate: `MIRTargetSupport` marks which symbols/types are available on `js`
(`MIRPortability` today has no call sites and classifies `Call`/`Load`/`Store` as
`NativeOnly`, so it cannot be used as-is).

### 3.8 Production output pipeline (bundler outside the compiler)

The compiler keeps only the **emitter** (it needs MIR). The bundle graph, manifest, hashing,
minifier and asset pipeline are build-time work and live in a Chemical library/build script
(`web_build`), so:
- `TCCCompiler` does not pay their size cost (the ≤ 5 MB constraint);
- the bundler is written in Chemical and tested like any library;
- the graph is data, not compiler state.

| stage | technique |
|---|---|
| minify JS | whitespace/comments; identifier mangling for emitter output (scope table) and for the runtime (needs the shared JS parser — budget 45 KB min for the runtime until then) |
| minify CSS | existing parser → compact re-emit; hashed classes preserved |
| tree-shake | reachability from entry + dynamic boundaries (we know every export) |
| split | route table + lazy boundaries (§3.5) |
| hash | content hash of final bytes |
| source map | emitter records generated ↔ Chemical source (`MIR` `source_index`); runtime maps optional |
| assets | copy + hash + rewrite URLs; `srcset`/font helpers; no image transcoding in-tree |
| compress | precompress `.gz` only, or delegate to CDN/brotli at the edge (**no brotli in-tree**) |

Budgets (CI): runtime ≤ 45 KB min (35 KB once runtime mangling lands); components-app page
chunk ≤ 50 KB min; zero duplicate units; every dev chunk has a source map.

### 3.9 CSS and assets (first-class, not an afterthought)

- `styles.<hash>.css` per page/route, referenced by `<link>` (not inline) so CSP `style-src`
  works; critical-CSS inlining is opt-in per route.
- per-route CSS splitting derived from the same route table (component `style {}` and
  `#globalcss` are shared; `#css`/`#js` stay page-local).
- `@layer` ordering emitted once to make the cascade deterministic across chunks.
- theme/dark-mode via `#globalcss` + CSS custom properties (the existing `theme.ch` helper is
  promoted to a documented pattern).
- assets: `assets/<name>.<hash>.<ext>` in the manifest; helper for `srcset`; fonts unchanged
  (TTF only today — pre-convert WOFF2 outside the toolchain).

### 3.10 Data path (no annotations)

- The client calls ordinary HTTP endpoints the server defines; typed wrappers come from
  **shared models** (`#json`) and a small `web::Client` (`get_json`, `post_json`, retry,
  abort, timeout).
- Optional typed endpoints without annotations: a **route table in `shared/`** declares
  `path + request + response`; `server/` registers handlers against it; `client/` gets a
  generated stub. This is opt-in and never required.
- Forms work without JS (progressive enhancement); `web::Form` validates with shared code.

### 3.11 Dev loop

`chemical serve` (reload-based, not HMR): watch → rebuild affected client units → serve SSR
from the server job + chunks from memory; error overlay; source maps on. HMR is cut (see §7).

### 3.12 Delivery modes: static, shell + slots, full SSR

Three modes, one render pipeline, chosen per route:

| mode | when the HTML is produced | per-request cost | use |
|---|---|---|---|
| **static** | at build (`chemical build --static`) | none (served from cache) | marketing, docs, blog, help |
| **shell + slots** | once per route, at build or first use | one alloc + N splices | personalised pages, dashboards, app shells |
| **full SSR** | per request | full component walk | data-dependent structure |

The middle mode does not exist today. The current pattern (`lang/compiled/playground/app/main.ch`)
is: render one `HtmlPage`, call `toString()` once, capture the string in a `send_sync_function`
handler, return `html_response(view)`. That is fast, but there is no way to inject anything
request-specific short of rebuilding the entire page — which is exactly what "we sometimes have
dynamic content per request" turns into.

**Slots.** A render marks request-varying regions:

```chemical
// server/pages/home.ch
public func Home(page : &mut HtmlPage, req : &Request) {
    page.appendView("<header class=\"top\">")
    page.slot("account")                 // filled per request
    page.appendView("</header>")
    /* ... frozen body ... */
}
```

- `web::Shell.compile(Home)` renders the route **once**, records every slot's byte range, and
  freezes the buffer together with its head tags, CSS hash and chunk list. Instance ids are
  frozen with it — which is precisely what the client bundle needs to line up.
- `shell.emit(req, fills) -> Rendered`: one allocation of the frozen length, then one `memcpy`
  per segment. Zero component calls, zero escaping work, zero CSS hashing, no `string` churn.
- **Invariant (gated, §3.19):** `shell.emit` with empty fills is **byte-identical** to a fresh
  full SSR of the same route. If that ever fails, a slot is wrong.
- A fill is plain HTML, not interactive: no `#html` component call and no universal component may
  render into a slot, because their hydration ids are baked into the shell and cannot be allocated
  per request. Enforced by checking that no `require_component` fires while filling (the same
  collector the bundle graph uses, §3.3). Structural variation (signed in vs out, admin vs user) is
  a *shell* concern: compile one shell per variant and pick by a key, instead of branching inside a
  frozen buffer.
- Filling is escaping-safe by construction: the shell records whether the slot sits in text or
  attribute context, so the fill is escaped for the position the author chose.

A route therefore has two halves, and only the second runs per request:

```chemical
public func Home(page : &mut HtmlPage, req : &Request) {
    /* whole page; page.slot("account") marks the varying region */
}

public func HomeSlots(req : &Request) : web::SlotFills {
    var f = web::SlotFills()
    f.set("account", account_bar(req))     // plain HTML, no component calls
    return f
}

app.page("/", Home, HomeSlots)             // compiled into a shell once
```

Cost model to verify in Phase 4 (probe in §3.19): shell emit ≥ 5× full SSR on the components app,
and ≥ 20× on a page with 100+ components.

`HtmlPage` remains the render primitive for all three modes; `web::Shell` is a build/serve-time
cache over it, not a second renderer.

### 3.13 Per-route payload: what a page loads, and in what order

Chunk tiers, assigned automatically by `web_build`:

| tier | contents | cache | loaded |
|---|---|---|---|
| `runtime.<h>.js` | hydration runtime | immutable | every page, first |
| `vendor.<h>.js` | units used by ≥ 2 routes | immutable | routes that need it |
| `route/<id>.<h>.js` | units used by exactly 1 route | immutable | that route |
| `lazy/<id>.<h>.js` | `import()` boundaries (editors, canvases, secondary routes) | immutable | on interaction |
| `styles.<h>.css`, `route/<id>.<h>.css` | CSS chunks | immutable | `<link>` in head |
| `assets/<name>.<h>.<ext>` | images, fonts | immutable | as referenced |

Rules that make "we control how code is shared" true:

- **First-load bytes per route** = runtime + vendor + route + CSS. The build prints this table per
  route and fails a budget (§3.19).
- A route's chunk never contains a unit another route also uses — that unit is in `vendor`. A unit
  used by one route, larger than 8 KB, and not needed before first paint goes to `lazy/`.
- Placement is decided by measurement (unit sizes × route closures), never by hand. `attach_shared`
  and the manual warm pass disappear.
- Document order: CSS `<link>` → `<link rel=modulepreload>` for runtime, vendor, route → the
  route's `<script type="module">`. No inline script (CSP), no blocking classic scripts.
- A shell's head is frozen, so it may only preload tiers it knows: runtime + vendor. Route tags are
  injected per route by `Rendered`.
- Everything is content-hashed, so a deploy invalidates exactly the chunks that changed, and a
  client navigating between routes gets everything else from cache.
- `web_build --profile` prints, per route: first-load bytes, chunk composition (unit → bytes), and
  what navigating to every other route would cost (shared vs new bytes).

### 3.14 Static sites that share JS (`--static`)

Static output today is per page: build an `HtmlPage`, `writeToDirectory`, done — so two pages
duplicate the runtime and every shared component (measured in §0). `--static` is the same client
job and the same chunk graph, with "serve" replaced by "write":

```
chemical build --web --static                 # every route -> dist/<path>/index.html + dist/assets/*
chemical build --web --static --out dist
```

- One pass computes shared chunks **across all pages** (runtime, vendor, per-page), so on a static
  host the second page reuses the runtime and shared component units from cache. Navigation inside
  a static site stays cache-warm and needs no server.
- `#html`-only pages (`#html`/`#css`, no universal components, no client modules) still emit **zero
  JS** — they only join the CSS graph. The "a static page ships no JS" invariant survives.
- `base_path` rewrites absolute URLs for subdirectory hosting (GitHub Pages project sites).
- Router routes export as one file per route (the router already has a static-export route
  manifest); a route that cannot be rendered without a request is a build warning, never a silent
  404.
- Emitted alongside: `sitemap.xml` and `robots.txt` from the route table — cheap, and a real need
  for the marketing/docs case.
- Optional static prefetch: on hover of a same-origin `<a>`, inject `<link rel=modulepreload>` for
  the target page's chunk. One tiny script, opt-in per site.

Static, shell + slots and SSR share the graph builder, minifier and manifest, so one project can
export some routes (docs) and serve others (dashboard) from one `chemical.mod`.

**Interim, before Phase 5 lands:** `toStringExternalJs` plus a frozen `SharedAssets` sink already
lets two static pages reference the *same* `shared.<hash>.js` instead of inlining it, so the
"static pages share JS" story is partially available in Phase 2 — the gap that remains until
Phase 5 is automatic per-page chunks, minification and cross-page vendor computation.

### 3.15 Typed client code, and the end of "check the browser console"

Four pieces; the first two need no JS backend.

**(a) Typed `#universal` props (breaking, Phase 2).** Props are untyped today, so a typo becomes
`undefined` in the browser. Change the declaration:

```chemical
@props
public struct CardProps {
    var title : string_view
    var count : int = 0
}

#universal Card(props : CardProps) { /* ... */ }
```

`<Card title={x} counts={3} />` then fails to compile (`unknown prop 'counts'`, `missing prop
'title'`, `int does not satisfy string_view`). The props serializer is **generated from the
struct** instead of the ad-hoc string building used today, which removes the documented escaping
footguns (single quotes and backslashes in string props, struct/vector props producing corrupt
output) by construction: the payload becomes JSON, escaped for a
`<script type="application/json">` block.

**(b) Hydration-safety analysis (Phase 2).** SSR and the client must agree, so render bodies may
not read nondeterministic or browser-only state: time/random intrinsics, `document`/`window`,
`localStorage`, `fetch`. A check over the render position of a `#universal` body reports the source
location; these are legal in handlers and effects. This turns the whole "hydration mismatch found
by a user" class into a build error, and it is why the mismatch overlay is a fallback rather than
the defence.

**(c) `dom` types (minimal in Phase 3, complete in Phase 7).** Proposal 0008's module: `Document`,
`Element`, typed handles for common tags, `Event`/`PointerEvent`/`KeyboardEvent`, `fetch`,
`localStorage`, `URL`, `CanvasRenderingContext2D`, `requestAnimationFrame`, `WebSocket`. Client
modules are ordinary Chemical, so `document.query_selector("#count")` returns `Option<Element>` and
a typo'd method is a compile error. `#js` stays as the escape hatch for hot loops, but it is no
longer the only way to write client code.

**(d) Dev overlay + error ingestion (Phase 3).** In dev an uncaught client error POSTs to the dev
endpoint with a source-mapped Chemical location and the terminal prints it. In production the same
hook samples and reports to a user-supplied endpoint (off by default). With (a)+(b), "open the
browser console" stops being a required debugging step: type errors and mismatch errors are compile
errors, runtime errors are reported to the server.

Client code compiles under the module's error policy: `Result` must be consumed, and the `client`
job rejects an unhandled `Result` — the no-panic direction applied to the one place where a panic
is a broken tab.

### 3.16 Compilation-performance foundations (so later releases can actually get faster)

None of this is a speedup on its own except where noted; these are the properties the later
speedups depend on, and they are cheapest to establish now.

- **One lowering, thin backends** (already the rule): the JS emitter is a MIR dispatcher — no AST,
  no per-backend re-derivation of layout or ABI.
- **Deterministic unit emission.** A unit's bytes depend only on its own MIR and module version: no
  pointer-derived names, no whole-program counters, no source-order dependence. Required for
  chunk-level caching *and* stable content hashes.
- **Chunk-level incremental caching.** `web_build` keys every unit by the hash of its emitted bytes;
  an unchanged unit is neither re-emitted nor re-minified, and an unchanged chunk is not rewritten.
  This makes the client job feel incremental long before the compiler is faster.
- **Parallel units.** Client units emit in parallel (processing is already per-file parallel); the
  emitter holds no global mutable state; graph work is per-route parallel.
- **Remove the constant cost of the embedded runtime** *(a real win)*. `page.ch` carries a 92 KB JS
  runtime as one escaped string literal (108 KB of source) that every module importing `page`
  lexes, parses and stores. Moving it to `runtime/*.js` + build-time embedding (Phase 4) removes
  that constant from every compile of every `page` consumer — the compile-time argument for doing
  it, independent of the bundle argument.
- **Macro cost proportional to the component.** `universal_cbi` builds a JS AST and emits strings
  per call site; removing per-instance dispatch text (Phase 2) removes most of that work.
- **Two jobs, separate caches.** `server` and `client` have separate build dirs and `built_files`
  caches, so a client-only edit never rebuilds the server, and vice versa.
- **Attributable budgets.** §6 tracks compile time and peak RSS for `--tcc`/`--llvm`; the client job
  gets its own breakdown (parse / symres / typecheck / lower / emit / bundle) so a regression has an
  owner.
- **No front-end sugar for the web stack.** New capability goes into MIR (shared by every backend)
  or into libraries — no new AST-only pass that native targets must also pay for.

### 3.17 Breaking changes to take now

One release, so authors migrate once. Each item is either a compile error with a fix, or
behaviour-only.

| # | change | why now | migration |
|---|---|---|---|
| 1 | `#universal` props typed via an `@props` struct; unknown/missing/mistyped props are errors | biggest error class, and it changes the component signature | compiler lists the declared prop set |
| 2 | props payload becomes a JSON `<script type="application/json">` manifest; `window.$__uni_dispatch` and the global queue are removed | 42% of a page is dispatch text, and the global contract blocks ESM | automatic |
| 3 | hydration ids become per-instance counters (`useId`) | two instances of one node collide today | automatic |
| 4 | `SharedAssets`/`attach_shared` and manual `shared.js() + getHtml() + local_js()` assembly replaced by `web::App`/`web::Bundle` | the manual warm pass is the step that never happens | one `app.page(...)` per route |
| 5 | head metadata becomes a typed `Head { title, meta, canonical, noindex }` instead of ad-hoc title strings | the head is part of the frozen shell and the manifest | mechanical |
| 6 | `http::Server` normalised: `Server.bind(cfg)`, public listener-free `dispatch`, `handle(app)`, one constructor | routing, assets and serving must be one pipeline | constructor rename |
| 7 | `page.writeToDirectory` becomes `web::Static.build(...)` (`@deprecated` shim for one release) | static export must use the same graph | one call |
| 8 | `#globaljs` becomes an ESM chunk with defined order (after runtime, before route) | order is currently emission order | documented order |
| 9 | text parameters are `string_view`; `dom` handles are non-copyable | ABI stability for the JS backend | landed / mechanical |
| 10 | router: typed `Head`, `noindex` on fallback routes, remote-route split as the supported lazy mechanism | one router design for static and served | small |

Deliberately **not** broken: `HtmlPage` itself, `#html`/`#css`/`#js` syntax, CSS hashing, id-based
routes, `#universal` body syntax (only the props declaration changes), and every non-web library
API.

### 3.18 Acceptance matrix: the four target apps

The apps are the acceptance suite, not demos. Requirement → state → phase.

| requirement | needed by | today | phase |
|---|---|---|---|
| SSR + hydration of a large component tree | all | yes | — |
| route splitting, shared runtime/vendor chunks | all | no | 2, 4 |
| shell + slot injection for per-request content | all (auth bar, counts, badges) | no | 4 |
| typed client modules (Chemical → JS) | timeline editor, mind map, canvas | no | 1, 3 |
| `dom` types: `Element`/`Event`/`fetch`/`localStorage` | all | no | 3, 7 |
| canvas 2D, pointer events, `requestAnimationFrame` | sketchable, 123mind | no | 7 |
| file upload (multipart) + image encode (PNG) | sketchable, timeline | no — `image` decodes only, no multipart parser | 7 |
| **realtime transport**: WebSocket/SSE client + server, reconnect/backoff | timeline sync, 123mind live | no — no WS client, no SSE | 6 |
| typed realtime messages over shared models | timeline, 123mind | yes once 6 lands (`#json` exists) | 6 |
| conflict resolution (CRDT/OT) | timeline sync | — | **not the stack's job**: a `sync` library, its own proposal |
| large-document rendering (virtual lists, streaming SSR) | notion-like editor | no | 8 |
| auth/session/cookies + CSRF | timeline, easytodo | no — no cookie API | 7 |
| forms with progressive enhancement | easytodo | partial | 6 |
| offline/local cache | easytodo | no | 7 |
| perf budgets (per-route bytes, LCP/TBT) | all | no | 0, 5 |

Two consequences: the stack must own **transport** (WS/SSE) and **types** (props, DOM), but must
**not** own a sync algorithm — CRDT belongs in a library so it can evolve without touching the
compiler or the bundler. And "insane performance" is only credible once the §3.19 harness exists
before the apps are built.

### 3.19 Probes and regression tests

Probes are *measurements with pass criteria*, run in CI; each becomes a durable test.

| probe | measures | pass criterion |
|---|---|---|
| **generation soundness** | every route of every fixture: well-formed HTML, balanced tags, unique instance ids, every dispatch id resolvable, every `href`/`src` present in the manifest, no inline `<script>` in prod | 0 defects |
| **hydration coverage** | manifest ids vs client closure | bijective |
| **static vs shell** | `shell.emit(empty)` vs full SSR bytes | byte-identical |
| **bundle quality** | per route: first-load bytes, duplicate units, chunk count, raw vs minified, gzip | duplicates 0; per-route ≤ budget; runtime ≤ 45 KB min |
| **cache efficiency** | second page of the same app: bytes from cache vs fresh | shared tiers only, 0 duplicated component bytes |
| **server time** | req/s and p50/p99 for static, shell, full SSR on one route | shell ≥ 5× SSR; static ≥ 50× SSR |
| **compile time / RSS** | per suite (`--tcc`, `--llvm`, client job) | no regression > 3% without a note |
| **app E2E** | `components-e2e`: route navigation, typing in the editor, drawing a stroke, todo round-trip | green, 0 console errors |
| **API quality** | negative tests: unknown prop, wrong prop type, `shared` importing `client`, DOM typo, unhandled `Result` in client | each fails with the documented diagnostic |
| **WebView** | `#universal_test` suite | unchanged |

The harness (`scripts/web-probe.sh` + fixture apps) lands in Phase 0, before any of it can
regress; each later phase adds its probe there instead of to an ad-hoc script.

---

## 4. Phases

### Phase 0 — Truth, fixtures, and the probe harness  *(S)*

- Re-measure output with **content-based** metrics (runtime bytes/share, dispatch-text bytes,
  duplicate-unit bytes) and publish the table in §0 as the baseline.
- Add `scripts/web-probe.sh` implementing the §3.19 probes that can run today (generation
  soundness, bundle quality, cache efficiency, compile time/RSS) and 4 golden fixture apps:
  `static` (pure `#html`), `universal` (components, no client modules), `router` (multi-route
  MPA), `shell` (one route with slots).
- Fix hydration ids to instance counters (today they are source-location based, so two instances
  of one node share an id — a correctness bug independent of everything else).
- Take the router's cheap wins: `noindex` on missing/fallback routes; typed `Head`; keep the
  existing static-export route manifest.
- **Gate:** golden HTML/JS/CSS for the 4 fixtures + the probe harness green and wired into CI.

### Phase 1 — **JavaScript backend** (job #1)  *(XL)*

**Step 0 (blocking, before any emitter code): ABI spike.** Decide how MIR's address-based
core maps to JS: MIR's core ops (`Alloca/Load/Store/Gep/FieldAddr/IndexAddr`) are
`NativeOnly` and layout is byte-offset (`MIRLayout`), and `std::string` is an SSO tagged
union over `malloc/realloc/free`. Two viable routes:

- **(A) emulated linear memory**: a `Uint8Array` heap + `DataView`, offsets from `MIRLayout`,
  `sizeof` meaningful, stdlib compiles unchanged. Larger runtime, slower, but mechanical.
- **(B) place→value promotion**: a MIR pass (places with `place_escaped == 0` → SSA values,
  aggregates → JS objects) + `@runtime_provided(js)` type facts so `string`/`vector`/maps map
  to native JS. Much better output, needs a new pass and a std surface per target.

Deliverable: `lang/docs/js-backend-abi.md` + a hand-written spike of one real module
(struct + `string` + `vector` + a variant) emitted to JS and run, with measured bytes.
**Do not write the emitter before this is decided.**

Then, in order:

1. Emitter skeleton `compiler/mir/MIREmitterJs.{h,cpp}`: opcode → JS over the shared MIR
   traversal; no AST access; declaration printing extracted from `MIRDeclPrinter` behind a
   printer interface (so backend #3 does not copy 529 lines).
2. Value model from the spike: scalars (`number`, masked ints), `BigInt` **not** in v1
   (`i64/u64` rejected in client code with a clear diagnostic until an app forces it),
   strings/vectors/maps per the chosen ABI, structs/variants, `Option`/`Result`.
3. Surface: scalars, control flow, `switch` on variants/ints, functions, recursion, structs,
   fixed arrays, unions, variants, `string`/`string_view`/`&str`, tuples, vectors/maps,
   `for-in`, closures (`std::function`, capturing lambdas).
4. Ownership on GC: destructors are no-ops on JS (no `@dispose` yet); borrow checking stays;
   the *semantic* decision for GC targets is owned by proposals 0006/0004, not by this plan.
5. Interfaces/vtables/`dyn`; async is **stubbed with a diagnostic** (the MIR coroutine form is
   a state machine; mapping it to `async/await` is a re-lowering — its own phase).
6. ESM emission: one module per Chemical module; shared mangling.
7. Test harness: differential **C-vs-JS conformance** (same program, identical stdout) + a
   Node runner path (the current runner spawns child processes over Win32 pipes and prints
   via `printf`).

**Exit:** a real module (JSON/regex/pure logic) runs identically under `--tcc` and `--js`;
core suite subset passes in Node; `--plugins` and all other suites unchanged; compile
time/peak RSS for other backends unchanged.

### Phase 2 — Cheap wins + typed props, no dependency on Phase 1  *(M, parallel)*

The whole phase can run in a second workstream while Phase 1 is in progress, and it carries three
of the breaking changes in §3.17.

1. **Hydration manifest** (§3.6): replaces dispatch text; ~42% smaller pages; removes
   `window[fnName]` and the global queue; props serialized once.
2. **`SharedAssets` freeze + content-hashed names + ETag + `toStringJsOnly`**: the shared-bundle
   design's Phase 4. This alone makes two pages share the runtime and the 63 components today.
3. **Typed `#universal` props** (§3.15a): `@props` struct, unknown/missing/mistyped props are
   compile errors, and the props payload + escaping is generated from the type.
4. **Hydration-safety analysis** (§3.15b): nondeterministic or browser-only reads in render
   position are build errors.

**Exit:** page 2 of a two-page app ships no runtime and no shared component bytes; the closure check
fails the build on an unreachable dispatch; a typo'd or missing prop fails compilation; a `Date.now`
in a render body fails compilation.

### Phase 3 — Client modules + dev server  *(L)*

1. New target flags `server`/`client`/`js` in `TargetData`, CLI `--web`, and the two-job build
   (§3.1). The split is **real modules** (`shared`, `server`, `client`, each with its own
   `chemical.mod`); `shared` is restricted to pure logic/models. Import-edge enforcement +
   per-job module-set check.
2. `MIRTargetSupport`: per-symbol/type availability on `js`, with the first unsupported callee
   reported at its source location (replaces `MIRPortability`-as-gate).
3. Minimal `dom` slice (proposal 0008): `Document`, `Element`, `Event`, `fetch`,
   `localStorage`, `URL` — enough for client modules to be useful; full 0008 later.
4. **Source maps** from the start of generated client code.
5. `chemical serve` (reload, no HMR): SSR + chunks from memory + the dev error overlay (§3.15d)
   and its ingestion endpoint.
6. Unhandled `Result` in client code is an error that must be reported (no-panic direction).

**Exit:** an onboarding example where a route's interaction is written in `client/*.ch`,
compiled to JS, and works in the browser with source maps and a terminal-reported error path.

### Phase 4 — Bundle graph, shell + slots, in-memory serving  *(L)*

1. `web_build` library: unit graph, chunking by the derived route closures, hashing, manifest,
   and the **two-phase orchestration** (client build → `assets.ch` → server build).
2. `web` module: `App` / `Bundle` — route table, in-memory render, `/assets/*` with ETag,
   embedded-or-disk loading. Requires making `Server.dispatch` public (or a listener-free `App`)
   and adding an in-memory asset map (none exists today).
3. **Closure derivation**: render each route once at build/first start to collect `require_*` ids,
   map ids → units, assign shared units to `vendor`, freeze the graph.
4. **`web::Shell` + slots** (§3.12): byte-range slots, per-route frozen buffers, per-variant
   shells, `shell.emit` with no component work. Delivery mode is chosen per route: static,
   shell+slots, or full SSR — one `web::App` can mix them.
5. Runtime extracted from `page.ch` into `lang/libs/page/runtime/*.js`, embedded at build time
   (a LabBuildCompiler asset-embed step — owned here, with a byte budget). Also removes a constant
   compile-time cost from every `page` consumer (§3.16).
6. CSS chunks + `<link>` (CSP-safe), `@layer` ordering, asset manifest, tier ordering and
   `modulepreload` per §3.13.
7. Closure / duplicate / frozen-chunk / static-vs-shell checks in CI.

**Exit:** the components app is served by `web::App` from an embedded bundle; two pages share
runtime + vendor; each route loads only its chunk; a personalised route is served from a slot shell
byte-identically to full SSR; no inline `<script>`.

### Phase 5 — Production output and static sites  *(M)*

1. Minifier (whitespace + emitter identifier mangling), CSS minifier, tree-shaking, gzip
   precompression or CDN delegation, budgets from Phase 0 enforced on minified bytes, `--profile`
   (per-route bytes, chunk composition, navigation cost).
2. **`--static` MPA build** (§3.14): shared chunks across all pages, zero-JS pages stay zero-JS,
   `base_path`, `sitemap.xml`/`robots.txt`, optional hover prefetch, `web::Static.build` replacing
   `writeToDirectory` (deprecated shim for one release).

**Exit:** budgets green; `chemical build --web` emits a deployable directory *or* an embedded
`assets.ch`; `--static` output shares the runtime + component units across pages (probe-verified);
`chemical build --web --static` is the documented path for the marketing/docs site.

### Phase 6 — SPA behaviour, data, and realtime transport  *(L)*

1. Route-level `import()` + prefetch (port the existing `remote`-route fragment machinery and its
   race handling, rather than reinventing), typed `Head` output (`meta`/canonical), `web::Client` +
   shared models, optional route-table typed endpoints, progressive forms, client-side navigation
   without document reload.
2. **Realtime transport** (`web::Realtime`): WebSocket client + server upgrade, SSE as the
   fallback, reconnect with backoff, typed messages via `#json` on `shared` models. No sync
   algorithm in-tree — CRDT/OT stays a library (§3.18).

**Exit:** route change loads one chunk, updates head, fetches typed data, no reload; a two-client
WebSocket echo/sync example runs on server and client with typed messages.

### Phase 7 — Full DOM types, canvas, uploads, security defaults  *(L)*

1. Complete proposal 0008 (`dom` submodules, host-handle borrow rule — a language change, not a
   library), DOM event typing in `#universal`, `--check` rejects unknown DOM members.
2. Canvas/pointer surface: `CanvasRenderingContext2D`, `PointerEvent`, `requestAnimationFrame`,
   `ImageData`, transforms — the sketchable/123mind requirement.
3. Uploads: multipart parsing in `http`, `image` PNG **encode** (today it decodes only).
4. Security defaults: same-origin by default, CSRF tokens for state-changing endpoints, cookies
   (absent from `http` today), CSP-friendly output, secrets from env at build/serve time.

**Exit:** a drawing surface and a note editor are written in Chemical, not `#js`.

### Phase 8 — Hardening and the acceptance apps  *(M, ongoing)*

Streaming SSR and virtual lists for large documents; sampled hydration-mismatch reporting (needs
the error endpoint); `http::FileServer` cache headers/ETag (needed by disk mode); a11y linting;
i18n/l10n + RTL (design in Phase 6 so the manifest can reserve a locale dimension); perf budgets
(LCP/TBT) in `components-e2e`; and the §3.18 acceptance apps (timeline, 123mind, easytodo,
sketchable) run as E2E fixtures — each app must pass the §3.19 probes, not just its own tests.

---

## 5. File-level work map

| area | files |
|---|---|
| JS emitter | `compiler/mir/MIREmitterJs.{h,cpp}`, `MIRDeclPrinter` (printer interface), `compiler/lab` (js job), `TargetData` (`js`/`client`/`server`) |
| ABI | `lang/docs/js-backend-abi.md`, MIR type facts for runtime-provided types |
| Target support | `compiler/mir/MIRTargetSupport.*` (new) |
| Module split | `compiler/processor/*` (conditional sources already exist), `ModToLabConverter`, CLI `--web` |
| Bundle build | `lang/libs/web_build/*` (new module, Chemical: graph, manifest, minifier, two-phase build, `--profile`) |
| App/serve | `lang/libs/web/*` (**new module**: `App`, `Bundle`, `Shell`, `Static`, `Client`, asset map) — do not graft onto `http`'s `web` namespace |
| Shell/slots | `lang/libs/page/src/page.ch` (`slot`), `lang/libs/web/src/shell.ch` |
| Runtime | `lang/libs/page/runtime/*.js` + build-time embedding |
| Components | `universal_cbi/src/react/ast_replace.ch`, `emit_js.ch`, `converter/*` (units instead of `pageJs`), `macro.ch` (`@props` parsing) |
| Hydration | `lang/libs/page/src/page.ch`, `ssr.ch`, `html_cbi/.../component.ch`, `main.ch` |
| Router | `lang/libs/router/*`, `page/src/router_runtime.ch`, `http/src/router.ch` (typed `Head`) |
| DOM | `lang/libs/dom/*` (per 0008), incl. canvas/pointer/rAF |
| Realtime | `lang/libs/web/src/realtime.ch` (WS/SSE client+server), `lang/libs/http/src/server.ch` (upgrade) |
| Static | `lang/libs/web/src/static_build.ch`, sitemap/robots, `base_path` |
| Dev | `compiler/core/main/*` (`serve`), `lang/libs/web_build/dev.*`, error-ingestion endpoint |
| Probes | `scripts/web-probe.sh`, `lang/tests/web/*`, `compiler_plugins/universal/*`, `components-e2e` |

---

## 6. Gates

| layer | gate |
|---|---|
| JS emitter | golden `.mjs` snapshots; `MIRTests` target-support assertions |
| JS semantics | **differential C-vs-JS conformance** (same program, identical stdout) |
| Modules | module-split violations fail (shared→client import, client→native symbol) |
| Hydration | ids unique per instance; closure check (unreachable id = build error) |
| Bundles | duplicate-unit = 0; frozen chunk cannot grow; every chunk hashed |
| Shell | `shell.emit(empty)` byte-identical to full SSR; no component may render into a slot |
| Size | `scripts/web-probe.sh` regression gate + Phase 5 budgets |
| Cache | second page of a 2-page app: 0 duplicated runtime/component bytes |
| Server time | shell ≥ 5× full SSR; static ≥ 50× full SSR (same route, p50) |
| CSP | production output contains no inline `<script>` |
| Static | `--static` output shares runtime + vendor chunks across pages; `#html`-only pages emit 0 JS |
| Props | unknown/missing/mistyped prop = compile error with the declared prop set |
| Determinism | render body reading time/random/DOM state = compile error; hydration-mismatch overlay green in E2E |
| Browser | `components-e2e`: route split, in-memory serving, head update, typed client, assets, canvas stroke |
| Engine | `#universal_test` WebView suite unchanged |
| Compiler | ≤ 5 MB release, per-PR size delta; compile-time/RSS deltas for `--tcc` and `--llvm`; per-phase timing for the client job; no `ast/` include in `MIREmitterJs`/`web_build` |

---

## 7. Non-goals and cuts (deliberate)

- **No `@server`/`@client` annotations.** The boundary is the module split (§3.1).
- **No external bundler** (esbuild/rollup/webpack): the graph and minifier are ours, in a
  build script, not in the compiler.
- **No brotli/deflate in-tree**; `.gz` at most, CDN for the rest.
- **No sync algorithm in-tree.** CRDT/OT for timeline is a `sync` library with its own proposal;
  the stack only supplies transport (WS/SSE) + typed messages + `shared` models.
- **HMR cut** (reload-only dev server); revisit after the stack is stable.
- **Async in the JS backend deferred** (stubbed with a diagnostic) — not needed for SSR,
  routing, or client DOM code.
- **`BigInt` for `i64/u64` deferred**: reject in client code initially.
- **No `@dispose`**; destructors are no-ops on JS.
- **No islands, SPA shell mode, multi-app vendor sharing, SRI, service worker/PWA** in v1.
- **No image transcoding** (PNG/BMP/PPM only; PNG *encode* lands in Phase 7 because sketchable
  needs a save path) or WOFF2 in-tree.
- **No React compatibility layer.**
- **No streaming SSR** until Phase 8.
- **No ownership-on-GC decision** made here: proposals 0006/0004 own it.
- **No user-facing "chunk config".** Placement is derived and measured; if it needs hand-tuning,
  the derivation is wrong.

---

## 8. Decisions needed for approval

Settled by this revision (from the design review), listed so they can be vetoed:

1. ~~`@server`/`@client` annotations~~ → **real modules** `shared`/`server`/`client`, each with
   its own `chemical.mod`; `shared` is pure logic only (no `page`/`dom`/macros).
2. **Route → bundle closures are derived by rendering each route once** (build or first start)
   and freezing the result, not by a native route table (which has no render function).
3. **`assets.ch` uses one string literal per text chunk** (base64 for binary) and a
   **two-phase build**; no byte-array modules.
4. **Three delivery modes** (static / shell+slots / full SSR), chosen per route, one pipeline.
   Slot fills are plain HTML — no component may render into a slot — and structural variation is
   a per-variant shell.
5. **Props become typed (`@props` struct) in this release**, together with the JSON props payload
   replacing dispatch text. This is the largest breaking change and it is taken now, not later.
6. **Chunk placement is derived, never configured.** Users declare routes; the build measures.

Still open:

7. **ABI route** (A) emulated heap vs (B) place→value promotion — Phase 1 step 0 decides with a
   spike; approve the spike as the gate.
8. **Embedded assets by default** (`assets.ch` in the binary) vs disk-first. Recommendation:
   embedded default, disk supported.
9. **Bundler location**: `web_build` library/build script (recommended) vs inside the compiler.
10. **Runtime mangling scope**: budget 45 KB min until a shared JS parser lands.
11. **Typed endpoint route table** (optional convenience) in or out of v1.
12. **Realtime transport scope**: WS + SSE both in Phase 6, or WS only with SSE later.
13. **`--static` default**: is `chemical build --web` static-by-default for apps without
    `server/`, or always two jobs? Recommendation: infer from the module set.

---

## 9. Sequencing

```
Phase 0  truth + fixtures + probe harness + useId fix      (S)
Phase 1  JS backend: ABI spike → emitter → conformance     (XL, critical path)
Phase 2  manifest + typed props + freeze/hash  (M, parallel)  ← can start immediately
Phase 3  module split + client modules + maps + serve      (L, needs Phase 1 ABI)
Phase 4  bundle graph + web::App + shell/slots + runtime    (L, needs 2+3)
Phase 5  minify/tree-shake/budgets + --static MPA          (M)
Phase 6  SPA routes/prefetch/head/data + realtime transport (L)
Phase 7  full DOM + canvas/pointer + uploads + security     (L)
Phase 8  hardening + the four acceptance apps               (M)
```

Phase 2 is independent of Phase 1 and delivers the headline sharing and typing wins in one
workstream while the backend is built in the other. Phase 5's `--static` and Phase 6's transport
are where the "static site" and "realtime app" user stories respectively become real.

---

## 10. How we will know it worked

- Two pages sharing components: runtime + vendor + 63 components load **once**; duplicate
  unit count = 0 (measured by `web-probe.sh`, not by eye).
- A page's initial JS ≤ 50 KB min; runtime ≤ 45 KB min (35 KB after runtime mangling).
- A personalised route is served from a frozen shell in one allocation plus one `memcpy` per slot,
  byte-identical to full SSR, and ≥ 5× faster (measured).
- `chemical build --web --static` produces a site whose pages share runtime and component units,
  where `#html`-only pages ship 0 bytes of JS, with a `base_path`, a sitemap, and hashed assets.
- Route change loads only that route's chunk, updates the head, fetches typed data, no reload.
- Client logic is Chemical in `client/`; a typo'd DOM member, a bad prop, or a nondeterministic
  read in a render body is a compile error, not a browser-console message.
- Realtime: two browsers sync a note over WS/SSE with typed messages, built from `shared` models.
- `chemical build --web` yields either one embedded binary or a hashed `dist/` with a manifest;
  `web::App` serves both, and the same graph drives both.
- Same program under `--tcc` and `--js` produces identical output (differential gate).
- Dev: edit → reload < 1 s on the components app; every chunk has a source map; a client error
  appears in the terminal with a Chemical source location.
- The four acceptance apps (timeline, 123mind, easytodo, sketchable) pass the §3.19 probes, which
  means a regression in HTML/JS soundness, bundle quality, server time, or API ergonomics fails CI
  before a user sees it.
