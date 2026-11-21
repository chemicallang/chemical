# Web application stack — master plan (v3)

**Status:** approved; **v1 scope frozen** (§3.20) · **Date:** 2026-11-15 · **Supersedes:** v1/v2 of this file
**Scope:** make `#html` / `#css` / `#js` / `#universal` / `router` / `page` a production
stack for large, multi-platform applications (timeline, 123mind, sketchable), with the
**JavaScript backend as job #1**.
**Companions:** [`proposals/0007-javascript-backend.md`](proposals/0007-javascript-backend.md) (approved),
[`proposals/0008-web-dom-types.md`](proposals/0008-web-dom-types.md),
[`universal-runtime-professionalization-plan.md`](universal-runtime-professionalization-plan.md),
[`universal-shared-js-bundle-design.md`](universal-shared-js-bundle-design.md),
[`universal-router-design.md`](universal-router-design.md),
[`language-goals.md`](language-goals.md).

## How this plan changes

This document started as a rough draft and stays a **living design**. A discovered fact that
buys safety, compile-time performance, or a better public API is allowed to reshape it — the
gate is that the change is argued in the text and lands in §3, not in a reviewer's head.

Two invariants govern how it may change:

1. **The public surface freezes at v1; the internals stay fluid.** Anything a user writes —
   `chemical.mod` shape, module names, function signatures, `@props`, build flags, chunk naming,
   the manifest format — is final when v1 ships. Everything behind it (the chunking algorithm,
   minifier quality, emitter optimisations, compile speed, runtime size) may change in any later
   release without notice. §3.20 draws that line explicitly.
2. **v1 is the minimum a real app can be built on**, not the whole plan. Later releases are
   additive and performance-only: no user is asked to re-migrate. §3.20 lists what is in v1 and
   what is explicitly deferred.

**Changes since v2 (the approval round):** v1 scope frozen with a public/internal freeze line
(§3.20); **skills + migration are a v1 deliverable** and the four apps are built by other agents,
not by this plan (§3.21); the JS backend is scoped to *minimal but real* (§3.7); all 196
components get typed props in this release; client network calls use a completion-handler API that
survives async; assets are embedded by default.

**Changes from v1:** no `@server`/`@client` annotations (three real modules, `shared`/`server`/`client`);
an in-memory app/serve model instead of "build an executable that dumps a site"; **three delivery
modes** (static / shell+slots / full SSR) with frozen-shell slot injection for server-time
performance (§3.12); **derived per-route payload tiers** (§3.13); **`--static` MPA builds that share
JS across pages** (§3.14); **typed `#universal` props + hydration-safety analysis** (§3.15);
compilation-performance foundations (§3.16); the explicit breaking-change list (§3.17); the
capability matrix (§3.18); and **probes with pass criteria** (§3.19).
Plus: corrected measurements; the JS-backend ABI decided before the emitter; the review's cheap
wins moved earlier; a cut list.

---

## 0. Verdict (measured)

The stack is excellent at its design centre: **render the page per request on the server**,
with `#html` static markup, compile-time-scoped `#css`, hydration via `#universal`, and a
server-rendered, lazy-hydrating router. Static sites and server-rendered apps are served well.

It is not yet a stack for large client-heavy apps. Measured on the real
`lang/compiled/components` app (two pages, same component library) by
`scripts/web-probe.sh` — *Phase 0, 2026-11-15; re-run it, do not trust these numbers if
the emitter changed*:

| measurement | `components.js` | `try.js` | note |
|---|---|---|---|
| total | 320,163 B (56,467 gzip) | 180,659 B (37,234 gzip) | one page bundle each |
| component definitions | 111 defs, 91,700 B (**28.6%**) | 96 defs | one `function X(props)` per line |
| hydration runtime | 57,145 B (**17.8%**) | 57,145 B (**31.6%**) | byte-identical copy per page |
| per-instance dispatch text | 131,403 B (**41.0%**), 805 calls | 27,866 B (**15.4%**), 177 calls | `window.$__uni_dispatch(..., getElementById('u<encoded source location>'))` |
| duplicated bytes across the two pages | — | **142,227 B = 78.7% of page B** | runtime + 63 defs + shared page script |
| shared component defs | 63 common, **63 byte-identical**, 46,670 B | same | re-sent per page |
| `import` / `export` / `import()` | 0 / 0 / 0 | 0 / 0 | one concatenated script |
| source maps | none | none | |
| asset names | `components.js` (unhashed) | `try.js` (unhashed) | 3 unhashed assets per page |
| minify headroom (comments + whitespace, exact) | **8.0%** raw | **14.2%** raw | superseded: the landed minifier takes a generated bundle to ~45% of its bytes (2026-11-20); identifier mangling is on top |
| inline `<script>` | 1 (author `#js`) | 1 (author `#js`) | violates the CSP rule in §3.19 |

Corrections this measurement forced on the plan: the runtime is **57,145 B** in the page
bundle (the earlier 92,887 B figure counted the head/router bootstrap region as runtime); the
dominant cost is **per-instance dispatch text (41%)**, which the hydration manifest removes
without any JS backend; and the honest whitespace/comment minify headroom is 8–14% raw, not
the 15–26% estimated from a cruder regex. The second page duplicates **78.7%** of its bytes —
that number is the case for the shared chunk graph, expressed in one line.

**Progress against that baseline** (re-measured after Phase 0 + Phase 2 step 1):

| | baseline | now | delta |
|---|---|---|---|
| `components.js` | 320,163 B | **285,066 B** | −35,097 B (−11.0%) |
| dispatch text | 131,403 B (41.0%) | **88,326 B (31.0%)** | −43,077 B |
| `components.html` | 247,774 B | **234,630 B** | −13,144 B |
| duplicate bytes on page 2 | 142,227 B (78.7%) | 150,203 B (83.8%) | *worse* — the shared chunk graph has not landed yet; the runtime grew slightly |

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

### 3.6 Hydration manifest (moved early; no JS backend needed) — **part 1 landed**

Two steps, both about the fact that dispatch text was **41% of a page bundle**.

**Step 1 (landed): compact dispatch + per-instance ids.** The converters now emit one call per
instance instead of a full `$__uni_dispatch` invocation, and the element lookup plus the mode string
live in the runtime:

```js
window.$__uni_c(7,'components_Button',{"variant":"info"});      // was:
window.$__uni_dispatch('components_Button', document.getElementById('u3476889310890819674'), {"variant":"info"});
```

Measured on the real components app (805 instances):

| | before | after | delta |
|---|---|---|---|
| dispatch text in `components.js` | 131,403 B (41.0%) | **88,326 B (31.0%)** | −43,077 B |
| ids in the page HTML | 19-digit source locations | 1–3 digit counters | −13,129 B of HTML |
| `components.js` total | 320,163 B | **285,066 B** | −35,097 B (−11.0%) |

**Step 2 (Phase 4, with chunks): one payload.** The remaining per-instance cost is the component
name (≈19 B × 805) and the props. A single `<script type="application/json">` payload plus a name
table removes the name repetition, and it is the correct shape once components can live in a
separate chunk (the payload then carries a chunk id). Doing it now would bake in a single-bundle
assumption, so it waits for the chunk graph.

What is already true and gated:
- ids are per **instance** (a counter), not per source location, so repeated instances cannot collide;
- the runtime owns `$__uni_c`, so emitted pages never repeat the lookup boilerplate;
- `#html` and JSX paths both go through it; route bodies still do not dispatch (INV-7).

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

**v1 backend surface — "minimal but real" (approved).** In scope for the first release:

| in v1 | deferred (later release, additive) |
|---|---|
| scalars with masked ints, control flow, `switch` on ints/variants | async/await lowering (MIR coroutine → JS state machine) |
| functions, recursion, structs, fixed arrays, unions, variants | exact 64-bit arithmetic beyond 2^53 (`BigInt`), a representation change only |
| `string` / `string_view` / `&str`, tuples, `vector`, maps, `for-in` | `dyn`/vtable-heavy generic patterns beyond simple interface dispatch |
| closures (`std::function`, capturing lambdas) | generators/iterators beyond `for-in` |
| interfaces/`dyn` with static dispatch; ESM emission; source maps | atomics, inline asm, `DynamicCall`, `Throw` (diagnostic in v1) |
| `u64`/`i64`/`size_t` as `number`, exact to 2^53 (the stdlib is built on 64-bit `size_t`) | `@dispose`/`@ref` — note destructors still *run* on JS (§ below) |

**ABI decided: Route A, emulated linear memory** — see [`js-backend-abi.md`](js-backend-abi.md).
The JS emitter executes MIR against a `Uint8Array` heap addressed by the *same* byte offsets the C
backend uses (native LP64 profile), so the stdlib compiles unchanged, `sizeof`/`offsetof`/unions/
variants/`memcpy` all work by construction, and **differential C-vs-JS conformance needs no
exclusions**. Route B (place→value promotion + JS objects for aggregates) was rejected on evidence:
unions put N field names at one offset, `packed`/`sizeof` are observable, lifetime is tracked per
place, and there is no MIR-to-MIR pass framework to build promotion on.

Two plan corrections this decision forced: (a) `i64`/`u64` **cannot** be rejected in v1 — `size_t`
is 64-bit and `vector`/`map` are built on it; they lower to `number` with a documented 2^53 bound and
a `--check` warning where the bound is reachable; (b) destructors are **not** no-ops on JS —
`std::string` frees in `@delete`, so drop glue is emitted exactly as on C.

The rule for the boundary is *demonstrable need*: a capability is added to the backend when a real
v1 app cannot express it any other way — never speculatively. Anything missing must be reachable
through the `#js` escape hatch (wrapped in a typed Chemical function, §3.21), so no app is blocked
by the subset.

**Client network without async (approved).** v1 client code has no `await`, so the network API is
completion-handler based from day one, and it is designed so async can be added *without changing a
single call site*:

```chemical
// client/todos.ch
web::Client.get_json<TodoList>("/api/todos", |res| {
    if(res is Result.Err) { return report_client_error(res) }
    var Ok(list) = res else { return }
    render_todos(list)
})
```

Later, `var todos = web::Client.get_json<TodoList>("/api/todos")?` is added as an *additional*
awaitable form; the handler form stays. This avoids a v1-only API that would have to be migrated.

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
  **shared models** (`#json`) and `web::Client`. In v1 the call shape is a **completion handler**
  (§3.7), which is the permanent shape and gains an awaitable sibling when async lands:

  ```chemical
  web::Client.get_json<TodoList>("/api/todos", |res| { /* Result<TodoList, HttpError> */ })
  web::Client.post_json<Todo, Created>("/api/todos", &draft, |res| { /* ... */ })
  ```

  `get_json`/`post_json` carry retry, abort and timeout; the handler always receives a `Result`,
  never a panic. `web::Client` is the *only* sanctioned client transport — hand-rolled `fetch`
  through `#js` is an audit finding, not a pattern.
- Optional typed endpoints without annotations: a **route table in `shared/`** declares
  `path + request + response`; `server/` registers handlers against it; `client/` gets a
  generated stub. This is opt-in and never required (deferred out of v1, §3.20).
- Forms work without JS (progressive enhancement); `web::Form` validates with shared code.
- Sessions: cookies + CSRF are **in v1** (moved up from Phase 7) because no real app ships
  without them; `web::Session` wraps the cookie jar and a server-side store interface.

### 3.11 Dev loop

`chemical serve` (reload-based, not HMR): watch → rebuild affected client units → serve SSR
from the server job + chunks from memory; error overlay; source maps on. HMR is cut (see §7).

### 3.12 Delivery modes: static, shell + slots, full SSR

Three modes, one render pipeline, chosen per route:

| mode | when the HTML is produced | per-request cost | use |
|---|---|---|---|
| **static** | at build (`chemical build --web --static`) | none (served from cache) | marketing, docs, blog, help |
| **shell + slots** | once per route, at build or first use | one alloc + N splices | personalised pages, dashboards, app shells |
| **full SSR** | per request | full component walk | data-dependent structure |

**The unit of delivery is a route, never a site.** A project is a route table in which each route
carries its own mode; no build flag, output format or library type may assume the whole site is
static (or wholly dynamic). The modes compose inside one binary: `/docs/*` pre-rendered at build,
`/dashboard` from a frozen shell, `/invoices/:id` rendered per request, all sharing one client
bundle. `--static` is therefore an *export of the routes that need no request*, not a property of
the site; a route that needs a request is reported per route and the rest still export.

**The default and the optimisation target is the application, not the export.** An app's per-request
cost is the thing users feel, so the dynamic path is the one the pipeline is optimised for, and
static delivery is the degenerate case of it (a route whose render was already run). Concretely, v1
must make these true *for serving apps*, and no static-only convenience may be a prerequisite for
any of them:

| concern | requirement |
|---|---|
| per-request render | full SSR is a supported route mode with a measured per-request cost and no build-time work added to it |
| caching | per-route policy: `immutable` for hashed assets, `no-cache`+ETag for pre-rendered HTML, `no-store`/`private` for personalised HTML, `Vary` where the render depends on a request header |
| personalisation | shell + slots (§3.12 below), so a logged-in header does not cost a full render |
| correctness under load | one shared bundle per build (not per request), no per-request allocation of hydration ids, ids frozen with the shell so client and server agree |
| non-happy paths | 404, redirect and error pages are route classes with the same caching rules, not a fallback afterthought |
| sessions | cookie jar + CSRF (§Phase 5) work in the dynamic path first; the static export simply has none |

The middle mode does not exist today. The current pattern (`lang/compiled/playground/app/main.ch`)
is: render one `HtmlPage`, call `toString()` once, capture the string in a `send_sync_function`
handler, return `html_response(view)`. That is fast, but there is no way to inject anything
request-specific short of rebuilding the entire page — which is exactly what "we sometimes have
dynamic content per request" turns into. The interim bridge (landed 2026-11-18) is
`StaticSite.render_dynamic` / `StaticSite.dynamic_response`: a route rendered per request against
the site's already-frozen, content-hashed bundle, answered `no-store` with no ETag, while its
pre-rendered siblings keep serving from memory. That is the full-SSR mode of this section on the
current stack, and it is what a real app uses until `web::App` arrives.

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
job and the same chunk graph, with "serve" replaced by "write".

**This is an export step applied per route, not a build mode for the project.** The rules that keep
it honest:

- A route declares whether it can be exported; only those are rendered at build. A route that needs
  a request (session, path parameter, query) is **skipped with a warning naming it**, and the rest of
  the export proceeds — exportability is never inferred from "the site is static".
- The exported routes and the served routes are the *same build*: same bundle bytes, same hash, same
  component units. Exporting must not change the artifact set the dynamic routes serve, so a project
  can deploy the directory to a CDN for marketing pages and the binary for the app, and a browser
  moving between them stays cache-warm.
- `base_path`, `sitemap.xml`, `robots.txt` and prefetch describe the **exported subset**, so they are
  output-and-metadata features, and they are explicitly *not* allowed to gate dynamic serving in the
  sequencing (§9.1 puts them after `web::App`).

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
lets two static pages reference the *same* `app.<hash>.js` instead of inlining it, so the
"static pages share JS" story is partially available now — the gap that remains until
Phase 5 is automatic per-page chunks, minification and cross-page vendor computation, plus the
two-pass writer that knows the hash before writing the HTML.

The two-pass shape the writer will implement (and that an app can do by hand today):

```
render every page        -> pages in memory, sink collects the shared bytes
sink.freeze()            -> js_name() / css_name() are final and content-derived
write pages + hashed files -> each HTML references app.<hash>.js and page.<hash>.js
```

`is_stale()` exists because the hazard is real: rendering one more page after freezing appends new
component definitions to the sink, and publishing the old name would serve a bundle that does not
contain the component the page asks for.

### 3.15 Typed client code, and the end of "check the browser console"

Four pieces; the first two need no JS backend.

**(a) Typed `#universal` props (landed: declaration + name validation).** Props are untyped today,
so a typo becomes `undefined` in the browser. The declaration now names a struct:

```chemical
public struct CardProps {
    var title : std::string_view
    var count : int
}

#universal Card(props : CardProps) { /* props.title, props.count */ }
```

`<Card title="ok" counts={3} />` fails to compile with
`unknown prop 'counts' for component 'Card'; declared props: title, count` (both the `#html` path and
the JSX-inside-`#universal` path). Implicit props are always accepted: `children`, `class`,
`className`, `key`, `ref`, `style`, and `on*`. A props annotation whose type is missing or not a
struct is reported at the call site and at the component.

**Disambiguation rule (permanent):** a props annotation starting with an **uppercase** letter names
a type (`props : CardProps`); a **lowercase** one keeps the pre-typed meaning of naming required
props (`props : title`, `props : a, b?`). This keeps the existing required-prop diagnostic
(`missing required prop 'title' on <Good>`) working without a new keyword.

Still to land for typed props (Phase 2 remainder):

- **Value types** — landed for Chemical values on both paths. The check calls the type checker's own
  `BaseType::satisfies(value, /*assignment*/ true)`, so implicit conversions behave exactly as they
  do for a field assignment (no new conversion rules invented in a plugin). The diagnostic spells
  both types using `BaseType::representation()`:
  `prop 'count' of component 'Card' expects 'int' but got 'string'`.
  A **raw JS literal** inside a JSX body (`count={"nope"}`) has no Chemical type, so the type
  checker cannot judge it — the converter compares its *kind* with the declared field type instead
  (`string literal` / `number literal` / `boolean literal`). A string literal for an `int` prop is
  rejected (`prop 'count' of component 'Chip' expects 'int' but got a string literal`), numbers and
  booleans for text props are rejected, `null`/`undefined` always mean "absent", and struct/other
  field types are left unchecked. Anything that is not a plain literal (a call, a member access)
  is still only checked on the `#html` path. Covered by
  `neg_universal_prop_literal_kind_mismatch`.
- **Defaults — landed (2026-11-17).** `var count : int = 0` in a props struct is allowed, and a
  call site may omit the prop. The value is injected into both sides from one place per path:
  * **JSX path** (`converter_jsx.ch`): `resolve_attributes()` appends one `ResolvedAttr` per omitted
    defaulted field (`defaultValue : *mut Value`), and both consumers of that list —
    `build_ssr_attributes` (SSR attribute) and `emit_js_props_from_resolved` (client payload) —
    emit it. A spread attribute disables the injection, since it may supply the prop at runtime.
  * **`#html` path** (`html_cbi/.../component.ch`): the missing field is injected as a real
    `HtmlAttribute` carrying a `ChemicalAttributeValue` built with the plugin-side
    `builder.allocate<T>()`, so the SSR prop pass and the client payload both see it.
  Both default-rejection diagnostics are gone. Covered by
  `universal_typed_props_defaults_reach_ssr_and_payload` (renders the default server-side **and**
  asserts it in the client payload, for both paths).
  **The call site always wins:** the `#html` injection checks for an existing attribute *before*
  falling back to the default (the first version injected unconditionally, which duplicated an
  explicitly passed prop and let the default win — caught by
  `components_separator_orientation`, where `<Separator orientation="vertical" />` rendered
  `horizontal`).
- **`@optional` props — absence without a default (landed 2026-11-18).** The controlled half of a
  controlled/uncontrolled pair (`checked`, `value`, `open`) needs a third state: not required (the
  caller may omit it) and not defaulted (a default is injected at every call site, so the prop could
  never be absent). `@optional var checked : bool` is that state. Mechanism: the Rust-side
  `optional` marker annotation is registered in `AnnotationController`; `universal_cbi`'s symres and
  (because the `#html` converter runs in codegen where no controller is reachable) `html_cbi`'s
  symres collect the marked member names onto `ComponentSignature.optionalProps`, which both
  required-prop checks consult. Nothing else changes: no injection, the declared type still
  value-checks a passed value (including the raw-literal kind check), and the body keeps reading
  `props.x` — which is a *runtime* prop object, so an omitted field is genuinely `undefined` (this is
  why the feature needed no runtime work). Guarded: `@optional` on a defaulted field is a compile
  error. Covered by `universal_optional_props_are_absent_unless_passed` (both paths, SSR markup and
  client payload), `neg_universal_optional_prop_with_default`, and
  `neg_universal_optional_prop_value_checked`.
  A rule this surfaced in the components: **prefer `@optional` over a default when the body already
  falls back** (`props.size || "md"`). A declared default is injected and then forwarded by a
  `{...props}` spread as a bogus element attribute (`size="md"` on a `<label>`, `id=""` on an
  `<input>`); `@optional` keeps the attribute off the DOM and changes nothing else.

- **A default is a value, not a node — two bugs from the first typed batches (fixed 2026-11-18).**
  Injecting a default into the SSR attribute and the client payload reuses the *call-site* emitters,
  which are built for values that came from source. A freshly built literal has no attached type, so
  `appliedValue.getType()` was `null` and the attr-value converter picked the wrong variant for a
  **bool**: the payload path emitted `append_js_escaped_char_ptr(page, false)` — a `_Bool` passed as
  a `*char` — and SSR **segfaulted** (`page_HtmlPageappend_js_escaped_char_ptr` ← the *outer*
  component ← `main`) for any typed component with a bool default used inside another component's
  JSX. Both the crash and the quoting loss were structural, not per-component. The rules now:
  * `ResolvedAttr` carries `defaultValueType` — the **declared field type** — and that is what the
    SSR attr-value converter sees. The type of the literal is only useful for text detection
    (`string_view` is a *struct*, so a declared type of `string_view` does not say "text"; the
    payload emitted `"tone": plain` unquoted until `isStr` checked both types).
  * A **bool** default is written straight into the payload as the JS literal `true`/`false`; it is
    never routed through the generic value path. That needs the value, which a `BoolValue` node did
    not expose to a plugin, so the CBI gained `BoolValue::getValue()` (C++ binding + `ASTBuilder.ch`).
  Regression coverage: `components_dropdown_*` (bool default in a nested component) and
  `universal_typed_props_defaults_reach_ssr_and_payload` (the unquoted-string regression).

- **Serializer from the type** — replace the ad-hoc prop payload building with generated JSON.
  *Measured (2026-11-17): the single-quote/backslash case is already correct on both paths — a
  literal prop `text="it's a \\ backslash"` and a runtime `string_view` prop both emit valid JS
  (`{"text":"it's a \\ backslash"}` / `{"text":'it\'s a \\ backslash'}`, `node --check` clean),
  and defaults now work. What remains real is **struct/vector props**, which have no client
  representation yet, so the serializer is justified by those rather than by escaping or defaults.*

**(b) Hydration-safety analysis (landed).** SSR and the client must agree, so render bodies may not
read nondeterministic or browser-only state. The pass (`universal_cbi/sym_res/hydration_safety.ch`)
walks the component's parsed body and reports, once per distinct construct:

```
component 'Stamp' reads 'now' during render; SSR and hydration would disagree —
move it into an event handler or an effect
```

Flagged: `Date.now`, `Math.random`, `performance.now`, and reads of `document`, `window`,
`localStorage`, `sessionStorage`, `navigator`, `fetch`, `location`, `history`. **Exempt: anything
inside a lambda** — the framework only passes lambdas to event handlers and effects, so a lambda is
handler position by construction. Exempting all lambdas is deliberate: a false negative (someone
reads the clock in a `use_state` initializer expressed as a lambda) is far cheaper than failing
correct components, and the shipped library is the proof — the pass flagged exactly two of our own
fixtures (`ReturnGuardFetch` calling `fetch()` in render, `DollarIdentifierTest` reading `window`
globals), both genuine violations, both migrated rather than exempted.

It runs at symres over the JS body, so it costs nothing at runtime and the message carries the
component's source location.

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

**Readiness status (2026-11-17).** What a new user can rely on, and what gates each claim:

| Claim | Gate |
|---|---|
| A template app builds, runs and produces the documented output | `scripts/web-starter-test.sh` (14 checks) in CI |
| A static page ships no JS; a served page hydrates from one shared bundle | `web-golden-test.sh` (4 fixtures), `web-split-test.sh` (15) |
| Output invariants (no inline `<script>`, no duplicated component defs, hashed assets) | `web-probe.sh --strict` on the static output, in CI |
| A real module runs identically under native and JS | 5 differential suites + runtime checks + diagnostics |
| Components and the universal pipeline behave | `--plugins` (1314 tests) |
| Typed props: names, value types, defaults on both paths, typed `Head` | plugin tests (`universal_typed_props_*`, `page_typed_head_*`, `components_separator_*`) |
| The `--universal` WebView suite *builds* | compile check (running it needs GTK3 + a display) |

Still open, in the order I would take them: **(1)** the in-process `TestEnv` adapter so a core suite
subset runs under Node (closes Phase 1); **(2)** wiring `MIRTargetSupport` into the LSP (needs MIR
lowering in the server); **(3)** aggregate-props serialization; **(4)** typing the remaining ~180
components under the spread policy (a per-component body change, not a plugin feature).

**Getting started (2026-11-17).** `lang/docs/web-getting-started.md` is the short path for someone
building an app today: build commands, the four concepts (`#html`, `#css`, `#universal`, typed props +
defaults), typed `Head`, multiple pages, the component library, and an explicit "not available yet"
list so nobody designs around `chemical serve`, `web_build`, the derived bundle graph or client-side
Chemical. It is anchored by a real, verified app at `lang/compiled/starter/` (two pages, two hydrated
components with typed props and defaults, a `Head`, and a `#css` block) rather than by snippets.

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
| 1 | `#universal` props typed via a props struct; unknown props are errors — **declaration + name validation landed**, value-type/required checks next | biggest error class, and it changes the component signature | compiler lists the declared prop set |
| 2 | props payload becomes a JSON `<script type="application/json">` manifest; `window.$__uni_dispatch` and the global queue are removed | 42% of a page is dispatch text, and the global contract blocks ESM | automatic |
| 3 | hydration ids become per-instance counters (`useId`) — **landed**; the emitted dispatch is now `document.getElementById('u' + <n>)` | two instances of one node collided before | automatic |
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

### 3.18 Capability matrix: what the stack must make expressible

This is the matrix the **skills publish** (§3.21) so an app agent knows what it can write today,
what it must reach for an escape hatch for, and what is coming. The four target apps motivate the
rows; they are built by **other agents using these skills**, not by this plan.

| requirement | needed by | v1 | later |
|---|---|---|---|
| SSR + hydration of a large component tree | all | yes | — |
| route splitting, shared runtime/vendor chunks | all | yes | — |
| shell + slot injection for per-request content | all (auth bar, counts, badges) | yes | — |
| typed client modules (Chemical → JS, sync subset) | timeline editor, mind map, canvas | yes (§3.7) | async, `i64`, `dyn` forms |
| `dom` types: `Element`/`Event`/`localStorage`/`URL` | all | minimal slice | full 0008 |
| client → server calls | all | `web::Client` + completion handler | awaitable sibling |
| auth/session/cookies + CSRF | timeline, easytodo | **yes** (moved up) | richer session stores |
| forms with progressive enhancement | easytodo | yes | — |
| canvas 2D, pointer events, `requestAnimationFrame` | sketchable, 123mind | **escape hatch**: typed wrapper over `#js` | typed surface |
| file upload (multipart) + PNG encode | sketchable, timeline | escape hatch / server-side only | typed surface |
| offline/local cache | easytodo | `localStorage` via `dom` | cache API |
| **realtime transport** (WS/SSE, reconnect) | timeline sync, 123mind live | no — escape hatch via `#js` WS | `web::Realtime` |
| typed realtime messages over shared models | timeline, 123mind | with the transport | — |
| conflict resolution (CRDT/OT) | timeline sync | — | **not the stack's job**: a `sync` library, its own proposal |
| large-document rendering (virtual lists, streaming SSR) | notion-like editor | no | release 3 |
| perf budgets (per-route bytes, LCP/TBT) | all | yes (probe) | tighter budgets |

Two consequences: the stack must own **transport** (WS/SSE) and **types** (props, DOM), but must
**not** own a sync algorithm — CRDT belongs in a library so it can evolve without touching the
compiler or the bundler. And "insane performance" is only credible once the §3.19 harness exists
before apps are written against it.

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
| **reference app** | the example app *shipped inside the skill* (§3.21): build, serve, browser E2E | green, 0 console errors |
| **skill conformance** | every code block in the skills compiles; the skill's API list matches the compiler's actual diagnostics | 0 drift |
| **API quality** | negative tests: unknown prop, wrong prop type, `shared` importing `client`, DOM typo, unhandled `Result` in client, `i64` in client code | each fails with the documented diagnostic |
| **WebView** | `#universal_test` suite | unchanged |

The harness (`scripts/web-probe.sh` + fixtures) lands in Phase 0, before any of it can regress;
each later phase adds its probe there instead of to an ad-hoc script. App-side conformance is the
same harness: an app team runs `scripts/web-probe.sh` in their repo, so "app quality" is measured
with the toolchain's own criteria rather than by taste.

### 3.20 v1 scope and the freeze line (approved)

**What ships in v1** — the minimum on which a real app can be built, with a public surface that
will not change afterwards:

| in v1 | from phase |
|---|---|
| probe harness, fixtures, per-instance hydration ids | 0 |
| props JSON manifest; **typed props on all 196 components**; typed `Head`; frozen + content-hashed shared assets; hydration-safety analysis | 2 |
| **JS backend, "minimal but real"** (§3.7) + ESM emission + source maps + C-vs-JS differential | 1 |
| `shared`/`server`/`client` modules, `server`/`client`/`js` flags, `chemical build --web`, `MIRTargetSupport`, minimal `dom`, `chemical serve` + dev overlay, `web::Client` completion handlers | 3 |
| `web_build` graph, `web::App`/`Bundle`, `web::Shell` + slots, embedded assets, CSS chunks, closure/cache checks | 4 |
| per-route delivery mode (pre-rendered / shell+slots / per-request SSR) in one app, each with its own cache policy; mixed static+dynamic in one project | 4 (interim form landed on `StaticSite`, §3.12) |
| JS + CSS minifier, hashed names, `--static` **export of the exportable route subset** with cross-page sharing, `--profile` | 5 |
| sessions/cookies + CSRF, `web::Session`, `web::Form` | 5 (pulled forward from 7) |
| **skills + migration kit + the one-sentence bootstrap prompt** (§3.21) | 5 |

**Deferred, additive only** (no v1 API changes this forces): async/await lowering, `BigInt` for
`i64/u64`, `dyn`-heavy client patterns, `web::Realtime` (WS/SSE), canvas/pointer/rAF types,
multipart + PNG encode, route-table typed endpoints, streaming SSR, virtual lists, i18n/RTL, HMR,
brotli, islands, SPA shell mode, multi-app vendor sharing, SRI.

**The freeze line.**

| frozen at v1 (users write it) | fluid forever (internals) |
|---|---|
| `chemical.mod` shape, module names `shared`/`server`/`client` | chunking algorithm, vendor threshold, budget numbers |
| `web::App`, `web::Bundle`, `web::Shell`, `web::SlotFills`, `web::Client`, `web::Session` (landed), `web::Form`, `web::Static` signatures | minifier quality, mangling scheme, runtime size |
| `@props` syntax, `#universal` declaration form, hydration manifest **format** | props payload encoding details, id allocation strategy |
| build flags `--web`, `--static`, `--out`, `server`/`client`/`js` target flags | emitter optimisations, compile-time performance, cache layout |
| chunk **naming scheme** (`runtime.<h>.js`, `route/<id>.<h>.js`) and asset URL shape | which unit lands in which tier |
| `dom` module name and its v1 member set (additions are additive) | DOM handle representation |

If a v1 gap is discovered after release, the answer is an **additive** API (new function, new
module, new flag) — never a rename. That is the whole point of taking the §3.17 breaks now.

### 3.21 Adoption: skills, migration kit, and the bootstrap prompt (v1 deliverable)

The four apps are built by other agents, and the release is only as good as their ability to use it
correctly. So v1 ships **documentation as an executable artifact**: skills in `.agents/skills/` that
are compiled and tested like code.

**Skills to write (or rewrite):**

| skill | contents |
|---|---|
| `web_app` (rewrite of `design_web_app`) | the module split, `web::App`/`Bundle`/`Shell`, typed props, `web::Client` completion handlers, `--web`/`--static`, dev loop, **capability matrix + escape hatches** (§3.18), perf rules (what makes a page slow), and a complete reference app |
| `web_migration` (new) | old → new API table (§3.17 items 1–10), before/after diffs, "error message → what to do" index, the `#js` → typed-wrapper pattern, and the `string_view`/`unsafe` rules from `release-readiness.md` |
| `web_client` (new, thin) | `dom` v1 members, `web::Client`, `localStorage`, event handlers, and the wrapper-over-`#js` pattern for canvas/WS until they are typed |
| `universal` (update) | typed props, per-instance ids, no dispatch globals, slot rules (no component inside a slot fill) |
| `components_e2e` (update) | how an app team runs the shared probe harness against their app |
| `testing` (update) | the new fixtures/probes and how a suite asserts bundle budgets |
| `AGENTS.md` | `@props` in the annotation table, the three-module shape, `web`/`web_build` in the architecture map |

**The bootstrap prompt** — the exact single sentence to hand an agent that will work on one of the
apps:

> Read the `web_app`, `web_migration`, and `web_client` skills before writing any code, then migrate this app to the new web stack — typed `#universal` props, `shared`/`server`/`client` modules, `web::App`/`Bundle`/`Shell`, `web::Client` completion handlers, `--check`-clean typed `dom` — using only the documented escape hatches where v1 has no typed surface, and keep `scripts/web-probe.sh` green.

**Why this is in the plan and not an afterthought:** the migration kit is the mechanism that makes
"public stuff stable on day 1" testable. `skill conformance` (§3.19) compiles every snippet in the
skills, so a skill that drifts from the compiler fails CI — the doc cannot rot. The reference app
inside `web_app` doubles as the fourth §3.19 fixture, so the path an app agent follows is the path
the release is tested on.

---

## 4. Phases

**Phases 0–5 are v1** (§3.20). Phases 6–8 are post-v1 and must be additive only. Phase 2 runs in
parallel with Phase 1; everything else is ordered.

### Phase 0 — Truth, fixtures, and the probe harness  *(S)* — **complete**

- [x] **Probe harness**: `scripts/web-probe.py` + `scripts/web-probe.sh` measure per-page totals,
      component-definition bytes, hydration-runtime bytes, dispatch-text bytes/calls, exact
      comment+whitespace minify headroom, cross-page duplicated bytes, ESM/source-map presence,
      inline `<script>` count, unhashed assets; `--strict` gates the invariants that already hold.
      The §0 baseline is its output.
- [x] **Per-instance hydration ids**: `HtmlPage` carries a render-time counter
      (`next_hydration_id`, `begin_hydration_marker`, `begin_hydration_comment`); `html_cbi`
      allocates one id per instance and writes it into both the marker and the dispatch
      (`document.getElementById('u' + <n>)`). Five instances of three call sites now get u1…u5;
      all 1305 plugin tests pass. *Known gap: `route_snapshot_append` replays cached bytes without
      re-running the counter — Phase 2's manifest owns id allocation and removes that path.*
- [x] **First fixture**: `lang/tests/web/fixtures/universal` (5 instances from 3 call sites).
- [x] **Remaining fixtures + goldens**: `static` (pure `#html`, 0 JS bytes and no
      `<script>`), `router` (route selection by id, per-route `<title>`, the active
      wrapper, and `noindex` on a page marked route-missing), `shell` (one layout,
      two personalised renders — the byte-identical baseline a slot shell must
      reproduce), plus `lang/tests/web/golden/<fixture>` snapshots for all four,
      checked by `scripts/web-golden-test.sh` (22 checks; the ~100 KB shared
      runtime bundle is excluded from the goldens — its hash still appears in every
      page). The router fixture selects routes by id — URL deep-link selection
      stays covered by `lang/tests/libs/router/src/url.ch` — and it also renders a
      `<NavLink>`, so `import router` is part of the gate. *That import was
      broken when the hydration-safety analysis landed (see item 4); the links now
      apply their URL active state in an effect, and the golden pins the
      consequence: SSR has the `href` and no `aria-current`.*
- [x] Router cheap wins: `noindex` on missing/fallback routes (`emit_route_noindex`), the
      static-export route manifest is kept, the link-active render read is fixed (an effect, see
      above), and **typed `Head` is landed** — `Head { title, description, canonical, noindex }`
      with every field optional plus `HtmlPage.set_head(&Head)`, which **replaces** the previously
      emitted block instead of appending a second `<title>`, so a layout can set a default and a
      page can override it. Escaped (`<title>Widgets &lt;Docs&gt;</title>`), and covered by
      `page_typed_head_emits_and_replaces` (one title, no leftover `noindex` after an override).
- [x] **Probe in CI**: `.github/workflows/web-stack.yml` runs
      `js-backend-suites`, `static-site-test`, **the probe with `--strict` on the static output**,
      `web-golden-test` and `web-split-test` on the TCCCompiler build, uploading the logs on failure.
      The probe now separates **gates** (invariants that hold: no inline `<script>`, no component
      definition duplicated across pages) from **deferred** items tagged with the phase that closes
      them (unhashed asset names → Phase 2/4; duplicated component defs → Phase 4). `--strict` exits
      non-zero on gates only, which is why it is honest to run it today: `lang/compiled/static_site_out`
      passes (`assets/app.<hash>.js`, 0 inline scripts, 2 shared assets). The served components app
      still inlines its per-page tail by design (§3.13), so the probe reports those inline scripts as
      gates for *that* output — the plan records it as a Phase 4 item (externalize the per-page tail),
      and CI gates the static output where the invariant is already true.
- [x] **Gate:** golden files for the 4 fixtures + the probe green in CI. — **met**
      (both scripts green locally on a clean TCCCompiler build).

### Phase 1 — **JavaScript backend** (job #1)  *(XL)*

**Step 0 (blocking, before any emitter code): ABI spike — DONE.** Route **A, emulated linear
memory** (`js-backend-abi.md`), and the spike **passes**: `scripts/js-abi-spike.sh` compiles
`lang/compiled/js_abi_spike` with the native backend, runs `scripts/js_abi_spike/spike.mjs` against
the emulated ABI, and requires byte-identical output. It prints identical `sizeof` for a struct, a
`vector<T>`, and `std::string` (8 / 24 / 32 — layout derived from the same rules, not hard-coded),
exercises the SSO→heap transition at 16 bytes with `malloc`/`realloc` on the same heap, addresses
vector elements from the type table, and dispatches a variant on its tag. 8,163 bytes of JS.

The remaining risk moves to the emitter: the spike encodes the operations by hand, so the general
claim ("std's own MIR runs on this ABI") is proven by the differential C-vs-JS suite, which is the
next gate.

**Step 1 — the emitter and the first runnable programs: LANDED.** `compiler/mir/MIREmitterJs.{h,cpp}`
prints a program's MIR as ES modules; `--emit-js <dir>` runs the normal front end and swaps the back
end (`do_js_job`). The gate is now the differential suite, not a hand-written file:

```
./scripts/js-backend-test.sh js_backend       # ints, loops, recursion, i8/u32 wrap, div/rem, pointers
./scripts/js-backend-test.sh js_backend_std   # std::string SSO+heap, vector<Point>, struct, variant
./scripts/js-backend-test.sh js_backend_closure # closures, callbacks, owned captures, for-in
./scripts/js-backend-test.sh js_backend_json  # the json library end to end
./scripts/js-backend-suites.sh                # all of the above + the runtime's own checks
```

All compile with the native backend and with `--emit-js`, run both, and require **byte-identical
output**. Reachability means a bundle carries only what `main` can reach: the std fixture emits
**26 of 908** lowered functions, JSON **109**.

**Exit criterion met.** `js_backend_json` parses a document with `json::parse`, walks a `JsonValue`
variant (object/string/number/bool/array), sums an array, and re-encodes with `json::stringify` —
identical stdout under both backends, including the map's iteration order, which pins the hash
function too. That is a real library module, not a fixture-shaped subset.

**Closures work through the same gate.** A capturing lambda stored in `std::function` stores the
capture environment and a function address in a fat pointer; the call passes the environment as the
first argument (`(fn_address)(env, args…)`). The one change that made this possible is that a
function address is now a **symbol id (a number)** rather than a function object — a closure stores
it in a pointer-sized field and reads it back through memory, so it has to survive as a number;
`rt.indirect($fns, addr)` resolves it and reports an unresolved address instead of "undefined is not
a function".

What that first version proves and what it does not:

- **Proven:** the value model (places as frame slots released at function exit, aggregates as byte
  addresses, scalars as `number` with C integer semantics in the runtime), control flow, direct, sret
  and indirect calls, closures with owned captures, constructors/destructors, `switch`,
  field/index addressing, `sizeof`/`alignof`/`offsetof` folded from the type table, the libc subset
  std actually uses (`ctype`, `strtol`/`strtod`, `str*`/`mem*`), and C-vs-JS agreement on real std
  code and on the `json` library.
- **Not yet:** cross-module ESM splitting (a bundle is one program module today; the split is Phase 3
  with the module split, and per-module files need a symbol→module mapping MIR does not publish),
  interface dispatch (`dynamic_call`), async, atomics, inline asm, source maps, and minification.

Two design corrections the emitter forced, both now recorded in `js-backend-abi.md`:

1. **Reachability before emission.** Emitting every function of every linked module cannot work:
   `cstd` alone has thousands of unused platform externs. `reachable_functions()` walks calls and
   function-address references from `main` over the whole (shared) MIR program — which is also
   whole-program dead-code elimination, exactly what a bundle wants. It caught a real bug in the
   first version: `Init`/`Drop` carry the ctor/dtor symbol at operand 1, not 0.
2. **Unsigned 64-bit saturates.** `(size_t)-1` is not representable in a JS number, so
   `x >= (size_t)-1` compared against `-1` and fired std's own "string capacity overflow" guard.
   `u64` now saturates at 2^53 (signed stays exact to ±2^53), which keeps such guards false.
3. **A function address must be a number.** It is stored in a closure's pointer-sized field and read
   back through memory, so a JS function object cannot be one; it is the MIR symbol id, resolved by
   `rt.indirect` at the call.

Then, in order:

1. Emitter skeleton `compiler/mir/MIREmitterJs.{h,cpp}`: opcode → JS over the shared MIR
   traversal; no AST access; declaration printing extracted from `MIRDeclPrinter` behind a
   printer interface (so backend #3 does not copy 529 lines). — **landed** (declaration printing is
   not needed yet: the emitter prints functions, globals and function tables; no C spellings).
2. Value model per §4 of the ABI doc: scalars (`number`, masked on store), pointers as offsets,
   aggregates as heap bytes, `u64`/`i64` as `number` with the 2^53 bound — **landed** (u64 saturates).
3. Surface: scalars, control flow, `switch` on variants/ints, functions, recursion, structs,
   fixed arrays, unions, variants, `string`/`string_view`/`&str`, tuples, vectors/maps,
   `for-in`, closures (`std::function`, capturing lambdas). — **landed, including `for-in` over a
   custom `Iterable`/`Cursor`**: `lang/tests/web/fixtures/js_backend_iter` implements both `Iterable`
   and `ReversibleIterable` over a user struct with a pointer cursor and runs forward and `reversed`
   loops; native and JS output agree byte-for-byte, which pins the interface calls (`begin`/`valid`/
   `current`/`next`, `rbegin`/`previous`/`count`) dispatched by function address on the JS side.
   That fixture also documents a real trap: `valid` must bound **both** ends, because a `reversed`
   cursor walks below the first element (an upper-bound-only predicate loops forever, which is
   exactly how the fixture first hung).
4. Destructors/drop glue emitted as on C (`std::string` frees); borrow checking unchanged; the
   *ownership* decision for GC targets is still owned by proposals 0006/0004. — **landed**.
5. Interfaces/vtables/`dyn`; async, atomics, inline asm, `DynamicCall`, `Throw` are **diagnostics**
   through `MIRTargetSupport`, reporting the callee/type and the Chemical source location. — **landed**:
   the emitter rejects all of these with the function name, the opcode and the *source position*,
   reports **every** offending site in one build, and `--check --emit-js` runs the same rules without
   writing anything (so a CI job can ask "does this compile to JavaScript?").
   `scripts/js-backend-diagnostics.sh` is the gate, including the native-symbol case: a call to an
   `@extern` C function is reported as *"native symbol 'getenv' has no JavaScript equivalent
   (declared @extern); a client or shared module must not depend on it"*, not as a module problem.
   **The shared table now exists**: `compiler/mir/MIRTargetSupport.{h,cpp}` holds
   `MIRTargetKind { C, LLVM, JS }`, a `MIRTargetRule` array of (target, opcode, reason) gaps, and
   `mir_unsupported_reason(target, opcode)`. The emitter no longer carries its own list — its
   fall-through asks the table — so the emitter, `--check` and any future pre-emission check cannot
   disagree. The rules are data (reason strings are the user-facing diagnostics) and are unit-testable
   without standing up an emitter. Remaining: wiring it into the LSP, which does not lower to MIR yet.
6. ESM emission: one module per Chemical module; shared mangling. — **deferred to Phase 3**: a bundle
   is one program module plus `index.mjs` and `runtime.mjs` today.
7. Test harness: differential **C-vs-JS conformance** (same program, identical stdout) + a
   Node runner path (the current runner spawns child processes over Win32 pipes and prints
   via `printf`). — **differential harness landed** (`scripts/js-backend-test.sh`,
   `scripts/js_backend/run.mjs`; five fixtures: `js_backend`, `_std`, `_closure`, `_iter`,
   `_json`) plus the runtime's own checks and the diagnostics gate in
   `scripts/js-backend-suites.sh`.   **Remaining for Phase 1: the test-runner path** — making a core suite subset run under Node with
   the same pass/fail reporting as the native runner. Why it is not a script: `test_runner` dispatches
   each `@test` in a **child process** and `TestEnv` reports over a pipe (`error`/`info`/`success`
   carry the message to the parent), so under Node — where process spawning and those externs are
   unavailable — the runner needs an **in-process** `TestEnv` adapter plus a loop over
   `get_tests()` that reproduces the native line format and summary counts. That is the unit of work,
   not a wrapper around `node run.mjs`.

**Exit:** a real module (JSON/regex/pure logic) runs identically under `--tcc` and `--js`;
core suite subset passes in Node; `--plugins` and all other suites unchanged; compile
time/peak RSS for other backends unchanged.

### Phase 2 — Cheap wins + typed props, no dependency on Phase 1  *(M, parallel)* — **in progress**

The whole phase can run in a second workstream while Phase 1 is in progress, and it carries three
of the breaking changes in §3.17.

1. [~] **Hydration manifest** (§3.6): **step 1 landed** — compact `$__uni_c` dispatch + per-instance
   counter ids took dispatch text from 41.0% to 31.0% of `components.js` and page HTML down 13 KB
   (measured, §3.6). Step 2 (single payload + name table) belongs with the chunk graph in Phase 4,
   where a payload can name a chunk instead of a global function.
2. [~] **`SharedAssets` freeze + content-hashed names**: **API landed** — `freeze()`,
   `is_frozen()`, `is_stale()`, `js_name()`/`css_name()` (`<base>.<hash>.js`, FNV-1a over the final
   bytes, 12 hex chars) and `write_frozen(dir)`. A stale sink (bytes appended after freeze, i.e. a
   page rendered late) refuses to hand out a name, so a build cannot publish a filename that
   disagrees with its content. Tested: two pages share the runtime + one component definition, a
   page's own JS keeps only its dispatch, the hash is deterministic, and a late page marks the sink
   stale. **Remaining**: the app-level two-pass writer that injects `<script src="app.<hash>.js">`
   into each page's HTML (the hash is only known after every page has rendered) and the ETag/
   immutable header in `web::App` — both belong with `web::Static` in Phase 4/5.
3. [~] **Typed `#universal` props** (§3.15a): declaration form, struct resolution, attribute-name
   validation on both the `#html` and JSX paths, **required-prop enforcement**, **value-type
   checking** against the field type (via the type checker's own `satisfies`), and **props
   defaults** — landed 2026-11-17: a field initializer may be omitted at the call site and is
   injected into both the SSR attribute and the client payload on both paths (§3.15a), covered by
   `universal_typed_props_defaults_reach_ssr_and_payload`. Negative tests cover unknown prop,
   missing prop, props type not found/must be a struct, and value-type mismatch; a positive test
   proves declared props and implicit props render. Raw JS literals in a JSX body are
   now kind-checked too (`append_unknown_prop_errors`, negative test
   `neg_universal_prop_literal_kind_mismatch`). Remaining: the generated serializer for
   **struct/vector props** (no client representation yet) — which is also why Gap B's array props
   cannot be typed.
   **Rollout state (2026-11-18, end of batch): 172 of 184 components typed.** (The plan's "196"
   was an estimate; the library has 184 `#universal` declarations.) The 12 left are blocked on a
   feature, not on effort — see Gap B below. Coverage by family: Card, Button (12), Badge
   (15), Alert (13), Avatar (7), Separator, ErrorOverlay, Input/TextArea/NativeSelect + Field
   family + InputGroup/Icon/Addon + 7 wrappers (17), Typography (23 incl. aliases), Surface (35),
   Data (17), Utilities (14), ToggleGroupItem, RadioGroupItem, SelectItem, Sheet slots (7),
   Toast slots (2), Collapsible slots (2).

   **Gap A — controlled/uncontrolled axis props — CLOSED (2026-11-18).** `@optional` gives typed
   props the missing third state (see the §3.15a bullet), and the ten components it blocked are
   typed: `Sheet`, `Collapsible`*reverted to untyped for a different reason*, `Slider`, `RadioGroup`,
   `Checkbox`, `Radio`, `Switch`; `Select` and `ToggleGroup` moved to Gap B (array-valued props) and
   `Toast` to Gap B (`action` is an element). `Collapsible` also moved to Gap B: its `trigger` is a
   JSX element, not the `open` axis.

   **Gap B — JSX element and array props (12 components: `Popover`, `Dialog`, `Dropdown`, `Tooltip`
   (`trigger`/`content`), `AccordionItem` (`trigger`), `Collapsible` (`trigger`), `Suspense`
   (`fallback`), `Toast` (`action`), `Tabs` (`tabs`/`panels`), `Pagination` (`pages`), `Select`
   (`options`), `ToggleGroup` (`value`/`defaultValue` are arrays in "multiple" mode)).** A JSX element or an array of JSX/descriptors has
   no Chemical value type to declare, so there is nothing to type-check against. These need either
   an element type in the JSX surface or for the component to take those parts as `children`
   slots; not part of the prop-typing work.

   **Wrapper rule (applies to all 40+ `ButtonSuccess`-style delegates).** A wrapper renders
   `<Parent {...props} pinned="value">`, and a **defaulted** wrapper field is injected at the
   wrapper's own call site and then forwarded by `{...props}` — where it fights the pinned
   attribute. `ButtonDanger` rendered `data-variant="default"` and dropped its destructive styling
   until `variant` was removed from the wrapper struct. So every wrapper surface is *the parent's
   props minus the pinned dimension* (`ButtonWrapperProps`, `BadgeWrapperProps`,
   `AlertWrapperProps`, `AvatarWrapperProps`, `InputWrapperProps`). The remaining fields still
   give name- and value-checking (`<ButtonSm type="sumbit">` fails, `<ButtonSm disabled="yes">`
   fails) which the old untyped surface let through.

   **The 196-component rollout was unblocked on 2026-11-17.**
   `lang/libs/components/src/Card.ch` is typed end to end: `CardProps { size = "default" }`,
   `CardTitleProps { level = 3 }`, and `CardSlotProps {}` (an **empty** props struct) for the
   header/description/meta/action/content/body/footer slots, so `<CardHeader classNam="x">` is a
   compile error instead of a silently dropped attribute. Every optional prop's default is the
   fallback its body already had, so existing call sites compile unchanged — verified by building
   `lang/compiled/components-e2e/app` (the heaviest consumer: `data-testid`, `onClick`, `size`) and
   the plugins suite.

   Typed in the second batch: `Avatar` (`src`, `alt`, `fallback`, `size`, `bordered`), `AvatarMore`
   (`count`), `AvatarBadge` (`label`), `AvatarGroup` (`max`), `AvatarGroupCount` (empty). `AvatarSm`
   and `AvatarLg` stay untyped on purpose — they are thin wrappers that spread `{...props}` **into
   another component** (not onto the DOM), which is the forwarding case the JSX prop validation
   already allows without a declared surface.

   Also typed in this batch: `SeparatorProps { orientation = "horizontal" }` and
   `ErrorOverlayProps { title = "Application Error", max = 10 }` — both zero-spread, defaults equal to
   the body's existing fallbacks. **Slider and Suspense are deliberately not typed:** Slider's
   controlled/uncontrolled split is `props.value != null`, so a declared default would make every
   slider controlled (it needs an `Option<int>`-shaped prop, or the body rewritten around a sentinel),
   and Suspense takes a JSX element in `fallback`, which has no Chemical value type to declare.

   Two rollout rules learned from the first batch:
   - **`data-*` is implicit.** Both plugins' implicit-prop checks now accept it, like `on*`. Without
     that, typing any component breaks the universal fixtures, which tag elements with
     `data-testid` for querying. `aria-*` is *not* implicit yet — add it deliberately, per component,
     or extend the rule when a component needs it.
   - **A spreading component is typed without removing its spread** — superseded rule. Declaring a
     surface used to turn every undeclared attribute into a compile error, so the pass-through
     components looked untypeable. The fix is on the validation side: **universal DOM attributes are
     implicit** in both plugins, exactly like `class`/`className`/`children`/`on*`/`data-*`. The
     implicit set is `id`, `title`, `role`, `tabindex`, `hidden`, `dir`, `lang`, `slot`, `part`,
     `contenteditable`, `draggable`, `spellcheck`, `inert`, `translate`, `autofocus`,
     `ariaLabel` (the Chemical spelling of `aria-label`), plus every `aria-*`. So `{...props}` keeps forwarding `role`/`aria-label`/vendor attributes while the
     declared surface is still name- and value-checked. A **declared** prop of the same name is
     still value-checked: member lookup happens *before* the implicit check on both paths (a
     declared `id : string_view` rejects `id={3}`; the implicit rule would have accepted it).

   **Third batch — the form and text families (2026-11-18).** 36 components now carry a declared
   props struct. Button (12), Badge (15) and Alert (13) were the first spreading families; then
   `Input`/`TextArea`/`NativeSelect`, all thirteen Typography components, and the `Field` family
   (`Field`, `FieldLabel`, `FieldHint`, `FieldError`, `InputGroup`, `InputIcon`, `InputAddon`).
   Empty props structs (`CardSlotProps`, `SlotProps`, `PlainTextProps`) carry the win cheaply: they
   declare no knob but still reject a mistyped semantic prop at the call site.

   Three rules from this batch, all encoded in the component sources:
   - **A default must not change controlled/uncontrolled semantics.** `placeholder`, `value`,
     `href`, `target`, `rel`, `id`, `name` and `aria-label` must stay *absent* from the DOM when
     unset, so their bodies render `prop={props.x ? props.x : null}` instead of a bare `{props.x}`
     (an `""` default would otherwise emit `placeholder=""` and make an uncontrolled input
     controlled). `null` on an attribute means "omit" on both the SSR and the client side.
   - **Structural defaults beat sentinel defaults.** `TextAreaProps.rows` defaults to `3`, not `0`
     — `rows="0"` hides the control, and `rows={props.rows > 0 ? … : null}` is the unreliable
     comparison-in-attribute shape (see the prop-expression hazard below).
   - **`prop !== undefined` stops working once the field has a default.** `Field`'s
     `label`/`hint`/`error` and `NativeSelect`'s `placeholder` test truthiness (`props.label ? … : null`),
     which is what the injected `""` default means anyway.

   **Prop-expression hazard, now reported (warn-then-escalate).** A method call on a prop inside a
   universal JSX expression renders **nothing** for the whole expression, silently:
   `{props.src.size() > 0 ? <img …/> : …}` produced an empty child, while `{props.src ? … : …}` and
   `{props.s != "" ? … : …}` both evaluate correctly (a plain `{props.s}` renders fine, and numbers
   render fine). So truthiness and `!= ""` are the idioms that work today. Avatar keeps truthiness,
   and its injected `""` default is falsy on both sides, so SSR and hydration agree.

   The converter now **reports every fall-through** in `convert_jsx_ssr_expression`
   (`report_unrenderable_ssr_expression`): an unhandled call, or a ternary whose condition the SSR
   evaluator has no form for. JS AST nodes carry no location, so the converter tracks the enclosing
   element's Chemical location (`JsConverter.current_element_loc`) and reports there — e.g.
   `return_guard.ch:55:13`, not `1:1`.

   It is a **warning, not an error, on purpose**, and three real sites prove why an error would be
   wrong today: `return_guard.ch` (calls local lambdas in render to assert the emitted JS),
   `to_string.ch` (a `typeof` ternary, JS-emission test) and our own `ErrorOverlay` (a client-only
   dev overlay: `open=false`, `errors=[]` at SSR, so an empty render is *intended*). Escalation
   plan: re-baseline the two JS-emission fixtures so they assert the same JS without un-emittable
   render-position expressions, mark genuinely client-only components (an explicit "no SSR" surface),
   then turn this into an error.

   **Spread-surface policy (decided 2026-11-17).** A typed props surface is only allowed where the
   component **enumerates** its attributes; a component that forwards unknown attributes stays
   untyped. There is deliberately no "open surface" escape hatch: the CBI exposes no struct
   annotation binding, so marking a props struct `@props_open` would need new compiler↔plugin
   plumbing, and a surface that accepts any name cannot type-check names anyway (all it would add is
   value-checking for the declared subset). So the recipe for each remaining component is a body
   change, not a plugin feature:
   1. declare the props the body actually *reads* (with the body's existing fallback as the default);
   2. replace `{...props}` with explicit forwarding of the implicit kinds the component needs
      (`class`/`className`, `style`, `on*`, `data-*`);
   3. add `aria-*` to the implicit-prop rule if the component genuinely forwards it, since `aria-*`
      is not implicit today;
   4. prove it with that component's SSR assertions in
      `lang/tests/compiler_plugins/components` (and the universal WebView fixtures for interaction).
   Components whose spread is load-bearing for an open API (a `Card`-like surface is fine; a
   `Button` used as a link with arbitrary `role`/`aria` is not) should keep the spread and stay
   untyped rather than silently narrowing their public surface.
4. [x] **Hydration-safety analysis** (§3.15b): **landed** — render bodies may not read the clock, a
   random number, or browser-only state (`document`/`window`/`localStorage`/`fetch`/…); lambdas are
   exempt because the framework only passes them to handlers and effects. Negative tests cover a
   clock read, a browser-state read, and the handler case (must compile). It flagged two of our own
   fixtures, both genuine — they were migrated, not exempted. It also flagged the **router
   library's own** `RouterLink`/`NavLink`, which read `window.$__uni_link_active` during render and
   made `import router` fail symres for every app: SSR cannot reproduce that call, so the links now
   compute URL active state in an effect against the `$url` signal (`urlActive` state + `useEffect`
   deps `[cur, target, end]`). SSR emits the `href` without `aria-current`, the first client render
   matches it, and the effect applies the active state — no mismatch, and no exemption added to the
   analysis. Remaining refinement: state
   initializers expressed as lambdas are currently exempt (a documented false negative).

**Exit:** page 2 of a two-page app ships no runtime and no shared component bytes; the closure check
fails the build on an unreachable dispatch; a typo'd or missing prop fails compilation; a `Date.now`
in a render body fails compilation.

### Phase 3 — Client modules + dev server  *(L)*

1. New target flags `server`/`client`/`js` in `TargetData`, CLI `--web`, and the two-job build
   (§3.1). The split is **real modules** (`shared`, `server`, `client`, each with its own
   `chemical.mod`); `shared` is restricted to pure logic/models. Import-edge enforcement +
   per-job module-set check. — **landed** (`LabBuildCompiler::build_web_module_no_alloc`:
   client pass = `shared` + `client` as JS, server pass = `shared` + `server` + `src` as
   native, each with its own build directory; conditional `source`/`import … if server|client`
   picks the module set, so a module outside the pass is simply not in the job. Gated by
   `scripts/web-split-test.sh`, including the native-symbol-in-client diagnostic.)
2. `MIRTargetSupport`: per-symbol/type availability on `js`, with the first unsupported callee
   reported at its source location (replaces `MIRPortability`-as-gate). — opcode table landed;
   the per-*symbol* answer lives in the emitter (`callee_expr`: "native symbol 'x' has no
   JavaScript equivalent (declared @extern); a client or shared module must not depend on it",
   at the call's location). **Landed 2026-11-19:** the per-symbol table is in the shared header
   (`MIRTargetSymbolRule` + `mir_unsupported_symbol_reason`, matched by mangled-name **prefix** —
   one rule covers the 29 `chx_dom_*` host declarations), and the C emitter refuses a call or
   function address to a listed symbol with that reason, so `--check`/the LSP can ask the same
   question. The reason is user-facing: "the 'dom' host API is client-only (JavaScript); call it
   from a 'client' module, not from server/shared code". The JS side keeps its by-name refusal
   because it also carries the JS spelling; `MIRTargetKind::JS` symbol rules are where a
   client-only *Chemical* symbol would go.
   This exposed a real silent-drop bug: a failure in the **parallel** pre-lowering phase was
   swallowed, and the symbol name had already been claimed for the job, so the serial retry
   skipped it and emitted nothing — the function vanished from the object and the caller got a
   link error instead of a diagnostic. Workers now record failures and `lower_and_emit` reports
   them (`[MIR] lowering/emission failed (...)` plus the located `MIR lowering failed`
   diagnostic). Gated by `web-split-test.sh`'s `dom_in_server` fixture: a server module using
   `dom` fails **during translation** (reason present, no `unresolved reference`).
3. Minimal `dom` slice (proposal 0008): `Document`, `Element`, `Event`, `fetch`,
   `localStorage`, `URL` — enough for client modules to be useful; full 0008 later.
   **Landed 2026-11-18:** `lang/libs/dom` (`document`, `Document.get_element_by_id`/
   `query_selector`/`create_element`/`body`, `Element` text/attributes/value/children/
   `add_event_listener`, `Event.prevent_default`/`stop_propagation`/`target`/`key`,
   `fetch_text`, `storage_get`/`storage_set`/`storage_remove`, `url_origin`/
   `url_pathname`/`url_search_param`), all safe by default with the raw host bindings under
   `dom::raw`. Browser objects cannot live in the emulated heap, so a `Document`/
   `Element`/`Event` is a **handle**: an index into a host table in the JS runtime that is
   never reused (a stale handle names its original object, never a different element).
   Callbacks (a listener, a fetch completion) travel as MIR function addresses and are
   resolved through the emitted module's `$fns` table, which the module now registers with
   `rt.register_fns`. Text crosses as `string_view`/`*char` in the heap. Gated by
   `scripts/web-dom-test.sh` (16 checks): the fixture's `client/*.ch` interaction is
   compiled by the `--web` client job and then **run** by
   `scripts/js_backend/dom_harness.mjs` (a small DOM + `localStorage` + `fetch` on
   `globalThis`) — document queries, element text/attributes/value, two real click
   callbacks, a fetch completion, storage and `URL` all produce expected output, and the
   source map still decodes to the listener's line. Also fixed on the way: `rt.printf` now
   supports `%.*s` (the idiom every Chemical test uses to print a `string_view`), which had
   been printing `%*s` literally in the JS backend.
4. **Source maps** from the start of generated client code. — **landed in the emitter**
   (`--emit-js` writes `<program>.mjs.map` + a `sourceMappingURL` comment; one segment per
   instruction line, mapping to the Chemical `file:line:column` the instruction came from). The gate
   *decodes* the map (`scripts/js_backend/sourcemap_check.mjs`): every source index and line must be
   in range and one mapping must land on the line that produced a known statement. The first
   implementation was wrong in a way only a decoder catches — the line cursor advanced after writing
   a segment, so every mapping after the first was appended to the first group.
5. `chemical serve` (reload, no HMR): SSR + chunks from memory + the dev error overlay (§3.15d)
   and its ingestion endpoint. — **not started.** `TCCCompiler … --web` already produces the two
   jobs; what is missing is the CLI command that watches sources, rebuilds, restarts the server,
   and the library-side dev bits (in-memory assets, reload script, `/__chx/error` ingestion with a
   source-mapped terminal report).
6. Unhandled `Result` in client code is an error that must be reported (no-panic direction). —
   **landed 2026-11-19** in `TypeVerifier` (gated on `TargetData.client`, so server/native code is
   deliberately unpoliced): a discarded `std::Result` — a bare call/chain statement, a parenthesized
   expression statement, the value of an expression statement — is `WEB-CLIENT-001` at the value's
   location. Matched by declaration (the `Result` variant under the `std` namespace), so a user type
   named `Result` is unaffected. The deliberate-discard escape hatch is the existing idiom
   `var _ = expr` (no new syntax); `std`'s best-effort `print`/`println`/`eprint`/`eprintln` now use
   it, and `try_print` remains the checked form. Gated by `web-split-test.sh`'s `client_result`
   fixture: the client discard fails with `WEB-CLIENT-001`, the same discard in server-only code
   builds and runs, and rewriting it to `var _ = …` makes both jobs green.

**Exit:** an onboarding example where a route's interaction is written in `client/*.ch`,
compiled to JS, and works in the browser with source maps and a terminal-reported error path.

### Phase 4 — Bundle graph, shell + slots, in-memory serving  *(L)*

1. `web_build` library: unit graph, chunking by the derived route closures, hashing, manifest,
   and the **two-phase orchestration** (client build → `assets.ch` → server build).
   **Unit graph + chunking landed 2026-11-19**, in `page` rather than in a separate `web_build`
   library: the graph is a property of the sink that already owns the bytes, so `SharedAssets`
   records it (`SharedUnit`: the component's encoded location, its name, the byte range it occupies
   after hoisting) and `StaticSite.build()` turns it into chunks before freezing the bundle
   (items 3 and 7 below). Shared definitions go to their own **`vendor`** bundle
   (`<base>.vendor.<hash>.js`, one file every page loads and caches forever), so the runtime bundle
   is now only the runtime. **The two-phase mechanism landed too**: `StaticSite.write_assets_module`
   writes the built bytes as Chemical source (one escaped literal per 3 KB) and `page.Bundle`
   (`lang/libs/page/src/bundle.ch`) serves them, so a deployed program carries its site inside the
   binary. `scripts/web-embedded-test.sh` runs the whole loop — build → generate → compile a
   second program that contains no site, page or component → compare its responses with the files
   the first phase wrote — and it is byte-identical (12 checks). Still open: a `web_build` *command*
   that owns that orchestration (today a gate script does), and CSS chunks (item 6).
2. `web` module: `App` / `Bundle` — route table, in-memory render, `/assets/*` with ETag,
   embedded-or-disk loading. Requires making `Server.dispatch` public (or a listener-free `App`)
   and adding an in-memory asset map (none exists today).
   **Landed 2026-11-19**: `App` serves from memory, and the asset map is `page.Bundle` — every
   artifact with its bytes, `Content-Type`, strong ETag and immutable flag, serving `/`, directory
   forms (`/about` and `/about/`), 304 from `If-None-Match` and 404, with exactly `StaticSite`'s
   contract. It is `page.Bundle` rather than `web::Bundle` because the container is a built page
   site, and `web` imports `page` (a `web::Bundle` would be a second spelling of the same object).
   Still open: **disk** loading. `fs::read_dir` exists but `DirEntry.path()`/`file_name()`/`is_dir()`
   are internal to `fs`, so a loader would have to widen that public surface; deferred rather than
   half-done.
3. **Closure derivation**: render each route once at build/first start to collect `require_*` ids,
   map ids → units, assign shared units to `vendor`, freeze the graph.
   **Landed 2026-11-19** — derived at emission time instead of by scraping the rendered output, so
   it is exact by construction rather than a text guess:
   * `HtmlPage.require_component(hash)` already runs once per component **instance**, i.e. exactly
     once per route that renders it, so it now reports `(hash, route token)` to the sink
     (`SharedAssets.note_usage`). The token is the route's ordinal for a pre-rendered page
     (`StaticSite.add`) or `unit_owner_dynamic()` for a live/shell render (`attach_dynamic`), which
     is what pins a shared component in the bundle — a per-request page dispatches from the bundle
     and never from a static route's chunk.
   * The universal component emitter reports the definition's byte range through
     `page.record_js_unit(name, hash, start, length)` at the hoist site
     (`universal_cbi/react/ast_replace.ch`); `SharedAssets` stores it, and `take_unit` removes a
     range and shifts the ones after it.
   * `StaticSite.plan_chunks` moves a unit into a route's chunk only when exactly one pre-rendered
     route uses it **and** the range is one self-contained definition
     (`unit_is_self_contained`: exactly one `function ` in the range, and no `\nfunction `), which
     is the guard against moving bytes a nested definition needs. Two routes, or any per-request
     route, keep it in the bundle. A component with no recorded range (an SSR-only component, or a
     definition the planner never hoists) is simply never moved — its bytes stay in the bundle's
     tail, which the plan does not touch; `unbacked_unit_count()` reports how many there are.
     Everything unclassifiable stays in the bundle: the split can be conservative, never wrong.
   * A **hoist-position bug had to be fixed** for this: `HtmlPage.js_hoist_pos` is per page but the
     hoist writes into the *sink's* buffer, so a second page started at 0 and inserted its
     definitions in front of the first page's, invalidating ranges already recorded. A page now
     seeds `js_hoist_pos` from `SharedAssets.hoist_end()` when it attaches.
   Measured on `fixtures/router` (three pages, `NavLink` on one route and `RouterApp` on all):
   bundle **141,625 B → 139,837 B**, the home chunk **231 B → 2,019 B**, and only the home chunk's
   hash changes — `about`/`missing` keep theirs, so a route that did not change stays cached.
   `fixtures/app` shows both directions at once (`chunked=1 retained=1`): `PricingNote` moves into
   `/pricing`'s chunk, `DashBody` (rendered by the shell and live routes) stays in the bundle.
   `unit_report()`/`chunked_unit_count()`/`retained_unit_count()`/`split_note()` expose the graph
   for the gates.
   * **`vendor` landed (2026-11-19)**: `plan_vendor` takes every definition that more than one
     pre-rendered route uses (or that a per-request route uses) out of the runtime bundle into
     `<base>.vendor.<hash>.js`, which every page loads *before* the runtime bundle. So the runtime
     bundle now holds only the runtime, `vendor` holds the shared component definitions, and each
     route chunk holds what only that route uses — three tiers, each content-hashed, so a change to
     one page invalidates that page's chunk and nothing else. `vendor_unit_count()` and
     `bundle_vendor_url()` expose it; `web-graph.sh` accepts a definition in the runtime bundle or
     in `vendor`, and `web-golden-test.sh` asserts the runtime bundle carries **no** component
     definition at all. Still open: CSS units and lazy chunking.
4. **`web::Shell` + slots** (§3.12): byte-range slots, per-route frozen buffers, per-variant
   shells, `shell.emit` with no component work. Delivery mode is chosen per route: static,
   shell+slots, or full SSR — one `web::App` can mix them.
   **Landed 2026-11-19** (`lang/libs/web/src/shell.ch`, `app.shell(...)`): `page.slot("name")`
   marks a region (an HTML-comment marker, so an uncompiled page is still valid HTML);
   `App.build()` renders each shell route **after** the bundle is frozen — the same call a live
   route makes per request — and records one `ShellSegment` per literal range and per marker;
   `shell.emit(fills)` walks those segments once, appending literals from the frozen buffer and
   one fill per slot. No component call, no CSS hashing, no hydration-id allocation happens per
   request, and the ids stay frozen so client and server agree. `web::SlotFills.set` splices HTML
   verbatim while `set_text` escapes (`&`, `<`, `>`, `"`, `'`), so an untrusted value cannot break
   out of its region; a fill for a slot the shell does not have is ignored (the shell decides the
   positions), and an unset slot emits nothing. A route is served `private, no-cache` with no ETag
   (the same URL returns different bytes per user) and is never exported as a pre-rendered page.
   A malformed marker or a route the frozen bundle cannot render fails the **build** with the
   route pattern, not the first visitor.
   Measured on `fixtures/app` (a header slot around a universal-component body), 2000 requests:
   **`emit` 8.4 ms vs full SSR 811 ms — ~95x (91–97x across runs)**, and `emit` is byte-identical to the full-SSR twin
   (`web-app-test.sh`, which also asserts the escape and the cache policy). Per-variant shells are
   just one `app.shell` per variant pattern — no branch inside a frozen buffer.
   **Variants are first-class now (2026-11-19)**: `app.shell_variant(pattern, handler, fills, when)`
   registers the same pattern with a `(req) => bool` predicate; dispatch picks the first variant
   whose predicate accepts the request and falls back to a plain `app.shell` with that pattern. Each
   variant is warmed and frozen separately, so a request is still one splice per slot and the
   choice costs one predicate call (`fixtures/app` serves an admin and a guest document from one
   `/variant` pattern, gated by `web-app-test.sh`). Still open: CSS chunking per route and
   `modulepreload` (item 6).
5. Runtime extracted from `page.ch` into `lang/libs/page/runtime/*.js`, embedded at build time
   (a LabBuildCompiler asset-embed step — owned here, with a byte budget). Also removes a constant
   compile-time cost from every `page` consumer (§3.16). — **partially landed, by a different route**:
   `page.StaticSite` (new, `lang/libs/page/src/static_site.ch`) writes the shared runtime, component
   definitions and global CSS as **one content-hashed asset** and gives every page a
   `<script src>`/`<link>` to it, so no page inlines the runtime any more. That removes the *serving*
   cost (the reason item 5 exists) without the extraction step; the runtime is still a Rust-free
   Chemical string that the compiler must build into every `page` consumer, so §3.16's compile-time
   saving is still open.
   Measured on a matched pair (the same two pages, `fixtures/static_site` vs
   `fixtures/static_site_baseline`, `scripts/static-site-test.sh`):
   per-page HTML **109,317 B → 424 B / 409 B**, one bundle of 108,816 B served once and cached by the
   browser, **2 inline `<script>` per page → 0**, CSP clean. Nested routes get `../assets/…`.
   The build is available **in memory**: `build()` produces the whole artifact set (pages, bundle,
   per-page chunks) with no filesystem access and `respond(path)` answers from it — `/` and `/about/`
   resolve to their `index.html` (bare `/about` too), missing paths and traversal attempts get a 404
   page, have no `Cache-Control` for HTML and `public, max-age=31536000, immutable` for the hashed
   bundle. `respond_if_none_match(path, if_none_match)` is the conditional form: every artifact
   carries a strong `ETag` derived from its bytes (the bundle's validator is the same hash as its
   file name), a matching validator returns **304 with no body** and the response's cache policy, and
   a stale one returns the full body — measured on the fixture: bundle `304 | 0 B | immutable`, page
   `304 | 0 B | no-cache`, stale `200 | 409 B`. `finish()` is `build()` plus writing the same list.
   `build()` also refuses a page that dispatches hydration but has no runtime in the bundle (the
   `defaultUniversalSetup()` step skipped) — that failure is otherwise a browser TypeError nobody
   sees until production.
   `artifact_count()`/`artifact_path()`/`artifact_bytes()` expose the manifest, which is what a
   `web::App` will serve from. Until then the bridge is `webserve` (new,
   `lang/libs/webserve/src/serve.ch`, imports `http` + `page` so neither library depends on the
   other): `webserve.install(site)` once at startup and a catch-all route
   (`router.add("GET", "/:path*", ||(req) => webserve.respond_to_request(req))` — the pattern is
   `/:name*` and the handler must be a lambda) serves the site with `Content-Type`,
   `Cache-Control`, `ETag` and 304 support, copying the body into the response. It is a pure
   request→response mapping, so it is testable without a socket: the fixture checks the mapping *and*
   that `web::Router` dispatches a catch-all into it. One fixture then repeats the whole path over
   TCP (fixed port, like the http framing tests): a real `GET /` is `200` with the framework's own
   headers, the bundle is immutable, an unknown route is `404`, and `If-None-Match` is a bare `304`
   with no body.
6. CSS chunks + `<link>` (CSP-safe), `@layer` ordering, asset manifest, tier ordering and
   `modulepreload` per §3.13. **Still open**: component CSS is still one global stylesheet in the
   bundle (`SharedAssets.css_data` has no per-component ranges), and `@layer` ordering changes
   cascade semantics, so it needs its own design pass rather than riding on this item.
7. Closure / duplicate / frozen-chunk / static-vs-shell checks in CI.
   **Closure + duplicate landed 2026-11-19** (`scripts/web-graph.sh`, sourced by `web-app-test.sh`
   and `web-golden-test.sh`): for every built site it reads only the artifacts and asserts that
   every `$__uni_c(id,'name',…)` a file dispatches is defined in that file or in the shared bundle,
   and that no `function name(props)` is defined in two files. It deliberately does not consult the
   compiler, so the check still holds if the derivation changes. `frozen-chunk` is structural (a
   chunk's name is the hash of its bytes, and the split happens *before* `freeze()`, so a published
   name can never describe other bytes); `static-vs-shell` byte-identity is already gated by
   `web-app-test.sh`.

**Exit:** the components app is served by `web::App` from an embedded bundle; two pages share
runtime + vendor; each route loads only its chunk; a personalised route is served from a slot shell
byte-identically to full SSR; no inline `<script>`.

Status against that exit (2026-11-19): **served by `web::App`** from the in-memory artifact set —
landed, and a program can now be compiled *without* the site at all: `StaticSite.write_assets_module`
emits the bytes as Chemical source and `page.Bundle` serves them, byte-identically
(`web-embedded-test.sh`, 12 checks, runs the two builds back to back); **two pages share the
runtime** — landed (`StaticSite`, one content-hashed bundle); **each route loads only its chunk, and
shared definitions are their own `vendor` bundle** — landed (item 3); **a personalised route is a
slot shell** — landed, ~95x cheaper than full SSR and byte-identical (`web-app-test.sh`), with
per-request **variants** selected by predicate; **no inline `<script>`** — true for pre-rendered pages,
while a per-request/shell page still inlines its own dispatch tail (there is no per-request chunk
name to cache, and that tail carries this request's values).

Still open, in the order they should be taken: **(a)** a `web_build` command that owns the two-phase
orchestration the gate script performs today (build → `write_assets_module` → build the server);
**(b)** CSS chunks + `@layer` + `modulepreload` (item 6 — needs its own cascade-semantics pass);
**(c)** the runtime extraction (item 5's compile-time half); **(d)** `Bundle` loading a prebuilt tree
from disk, which needs `fs` to publish `DirEntry` accessors; **(e)** lazy chunking. None of these
block the exit contract above; each is recorded with the reason it was not taken now.

### Phase 5 — Production output, static sites, sessions, and the skills kit  *(M)* — **last v1 phase**

1. Minifier (whitespace + emitter identifier mangling), CSS minifier, tree-shaking, gzip
   precompression or CDN delegation, budgets from Phase 0 enforced on minified bytes, `--profile`
   (per-route bytes, chunk composition, navigation cost).
   **Whitespace/comment minifier for JS and CSS landed 2026-11-20** (see §9.1 item 10):
   `page.minify_js`/`minify_css`, applied by `SharedAssets.freeze()` when `set_minify(true)` is
   set, unit ranges recomputed and self-checked. Measured −55% on the `app` runtime bundle.
   **gzip + budgets + `--profile` landed 2026-11-20:** `compression::gzip`/`gunzip` are a
   pure-Chemical fixed-Huffman + greedy-LZ77 deflate (no new native dependency); opt-in
   `StaticSite.set_gzip(true)` adds `<path>.gz` sidecars in `finish()` (they are extras, not
   artifacts), keeps a variant only when it is smaller, gives each representation its own ETag,
   and `respond_negotiated(path, if_none_match, accept_encoding)` picks by `Accept-Encoding`
   (`q=0`, `identity`, `*` handled) while sending `Vary: Accept-Encoding`. `webserve` and
   `web::App` pass the header through. Measured on the gate fixture: minified bundle 108,520 →
   48,283 B, gzipped 13,122 B (node's `gzip -9` on the same bytes: 10,981 B — fixed-Huffman is
   ~20% larger, so dynamic-Huffman is not yet worth it); a page 466 → 311 B gzipped. Budgets are
   now asserted by `static-site-test.sh` (`BUDGET_BUNDLE_MIN=60000`, `BUDGET_BUNDLE_GZ=20000`,
   `BUDGET_PAGE=2048`, `BUDGET_CHUNK=8192`). `StaticSite.profile_report()` and
   `export_profile()` (`profile.json`) report per-route html/chunk bytes, unit counts and
   first-load vs navigation cost, enabled either by `set_profile(true)` or the new
   `--profile` build flag (`intrinsics::defined("profile")`). Still open in this item:
   **emitter identifier mangling** and **tree-shaking** — both change every emitted name or
   drop units, so they need their own verification pass rather than riding this one.
2. **`--static` MPA build** (§3.14): shared chunks across all pages, zero-JS pages stay zero-JS,
   `base_path`, `sitemap.xml`/`robots.txt`, optional hover prefetch, `web::Static.build` replacing
   `writeToDirectory` (deprecated shim for one release).
   **Export extras landed (2026-11-18):** `page.StaticSite.set_site_url` +
   `export_sitemap()`/`export_robots()` emit the sitemap for the **pre-rendered** routes only
   (never a per-request route), and an absolute `asset_prefix` (`/app/assets`) is used as-is by
   every page, so `web::App.set_base_path("/app")` serves the same build under a mount path
   (`web-app-test.sh` checks the mounted page keeps `/app/assets/...`). **`--static` landed
   2026-11-20:** `lang/libs/web/src/static.ch` (`web::Static` — `add`/`set_minify`/`set_gzip`/
   `set_profile`/`build`/`finish`/`export_sitemap`/`respond`/`respond_negotiated`) is a thin
   MPA wrapper over `page.StaticSite`, so one bundle is shared by every page; a page whose own
   chunk is empty emits **no `<script>` tag at all** (CSS link kept), so an `#html`-only page is
   really zero-JS; the CLI flag `--static` sets the `static` build definition. Gate: the MPA
   fixture exports two pages, `plain_scripts=0 plain_runtime=0 rich_scripts=2 bundle=1`, and the
   nested page reaches the bundle through `../assets/`. `HtmlPage.writeToDirectory` is
   `@deprecated` with `web::Static.build` as the replacement. Still open here: **hover
   prefetch** (needs a browser to verify the hover→fetch→cache path, which this environment
   cannot do).
3. **Sessions** (pulled forward from Phase 7): cookie jar, `Set-Cookie` parsing, `web::Session`
   with a server-side store interface, CSRF tokens for state-changing endpoints, same-origin
   default. No real app ships without these, so they gate v1.
   **Landed (2026-11-18):** `lang/libs/http/src/cookies.ch` — `http.CookieJar.of(&req)` /
   `get`/`has`/`len`, `http.CookieOptions` (Path, Domain, `SameSite`, `HttpOnly`, `Secure`,
   `Max-Age`), `set_cookie_value` (rejects unsafe names/values instead of emitting a broken
   header) and `Response.cookie`/`delete_cookie`; `lang/libs/web/src/session.ch` —
   `web::SessionStore` (`load`/`create`/`get`/`set`/`remove`/`csrf_token`/`verify_csrf`/
   `rotate_csrf`/`attach`/`destroy`), an in-memory record store, 48-hex random ids from
   `osrand`, `HttpOnly` + `SameSite=Lax` + `Path=/` on the session cookie, and constant-time
   CSRF comparison. `web-app-test.sh` covers the flow (13 session/cookie checks).
   **`Vary: Cookie` guard landed 2026-11-20:** `App.push_route` refuses a pre-rendered route
   whose `vary` contains `Cookie` (a shared cache would serve one user's render to another) and
   names `live()` as the fix.
   **Persistent store landed 2026-11-21:** `SessionStore.save_to(path)` writes the whole store
   (id, CSRF token and every key/value as hex) and `load_from(path)` reads it back, so a session
   survives the process that created it. It was blocked until then by a compiler bug, not by the
   store: a pattern-bound variant payload (`var Some(v) = o`) is an alias into the variant, and
   moving it into an aggregate field copied the bytes without clearing the variant's drop flag,
   which freed the container twice. `MIRLowerer` now clears the owning variant from aggregate
   initializers too (`mark_init_moved`/`is_pattern_bound_value`; regression test
   `lang/tests/src/basic/variants/destructure_move.ch`). A truncated snapshot (a half-written
   file) keeps the complete records before it and a missing file loads an empty store;
   `web-app-test.sh` gates the round-trip, the damaged/missing cases, and the restore in a
   second process. A store backed by a real database is still the documented extension point
   behind the existing `load`/`get`/`set` interface.
4. **Skills + migration kit** (§3.21): `web_app` (rewrite), `web_migration`, `web_client` (new),
   updates to `universal`/`components_e2e`/`testing`/`AGENTS.md`, the reference app inside
   `web_app`, and the one-sentence bootstrap prompt. Wire `skill conformance` into CI so every
   snippet in the skills compiles — **partially landed**: `design_web_app` now documents the typed
   props contract, `page.StaticSite` (shared hashed bundle, per-page chunks, `build()`/`respond()`),
   the measured numbers, and an explicit **today vs planned** capability matrix plus the bootstrap
   prompt, so app agents stop inventing `web::App` APIs that do not exist yet; `universal` gained
   the typed-props diagnostics and a pointer from `SharedAssets` to `StaticSite`. `web_migration`
   and `web_client` are still blocked on the APIs they would document (module split, typed `dom`),
   and skill conformance is not wired into CI.

**Exit (v1 gate):** budgets green; `chemical build --web` emits a deployable directory *or* an
embedded `assets.ch`; `--static` output shares runtime + component units across pages
(probe-verified); a form-based flow with sessions and CSRF works without JS; every skill snippet
compiles in CI; the reference app builds, serves and passes the browser E2E. **This is the
release.**

### Phase 6 — SPA behaviour and realtime *(L, post-v1)*

1. Route-level `import()` + prefetch (port the existing `remote`-route fragment machinery and its
   race handling, rather than reinventing), typed `Head` output (`meta`/canonical), optional
   route-table typed endpoints, client-side navigation without document reload.
2. **Realtime transport** (`web::Realtime`): WebSocket client + server upgrade, SSE as the
   fallback, reconnect with backoff, typed messages via `#json` on `shared` models. No sync
   algorithm in-tree — CRDT/OT stays a library (§3.18).
3. **Async lowering** in the JS backend, so `web::Client` gains its awaitable sibling and the
   completion-handler form keeps working.

**Exit:** route change loads one chunk, updates head, fetches typed data, no reload; a two-client
WebSocket sync example runs on server and client with typed messages; the same client code runs
with `await` and with handlers.

### Phase 7 — Full DOM types, canvas, uploads  *(L, post-v1)*

1. Complete proposal 0008 (`dom` submodules, host-handle borrow rule — a language change, not a
   library), DOM event typing in `#universal`, `--check` rejects unknown DOM members.
2. Canvas/pointer surface: `CanvasRenderingContext2D`, `PointerEvent`, `requestAnimationFrame`,
   `ImageData`, transforms — the sketchable/123mind requirement; the typed wrapper that apps wrote
   over `#js` in v1 becomes a thin alias, so no call-site migration.
3. Uploads: multipart parsing in `http`, `image` PNG **encode** (today it decodes only).
4. `BigInt` for `i64/u64` in client code; `dyn`-heavy patterns.

**Exit:** a drawing surface and a note editor are written in Chemical, not `#js`; every v1-era
wrapper over `#js` still compiles.

### Phase 8 — Hardening  *(M, ongoing)*

Streaming SSR and virtual lists for large documents; sampled hydration-mismatch reporting (needs
the error endpoint); `http::FileServer` cache headers/ETag (needed by disk mode); a11y linting;
i18n/l10n + RTL (design in Phase 6 so the manifest can reserve a locale dimension); perf budgets
(LCP/TBT) in `components-e2e`; `--check`-integrated audit of escape-hatch usage across the app
corpus (a `#js` block without a typed wrapper is a lint finding).

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
| Sessions | `lang/libs/http/src/types.ch` (cookie jar, `Set-Cookie`), `lang/libs/web/src/session.ch`, CSRF middleware |
| Dev | `compiler/core/main/*` (`serve`), `lang/libs/web_build/dev.*`, error-ingestion endpoint |
| Probes | `scripts/web-probe.sh`, `lang/tests/web/*`, `compiler_plugins/universal/*`, `components-e2e` |
| Skills | `.agents/skills/web_app/*` (rewrite), `.agents/skills/web_migration/*` (new), `.agents/skills/web_client/*` (new), updates to `universal`, `components_e2e`, `testing`, and `AGENTS.md` |

---

## 6. Gates

| layer | gate |
|---|---|
| JS emitter | golden `.mjs` snapshots; `MIRTests` target-support assertions |
| JS ABI | `scripts/js-abi-spike.sh`: native vs hand-written JS output must be byte-identical (the Phase 1 step-0 gate) |
| JS semantics | **differential C-vs-JS conformance**: `scripts/js-backend-test.sh <fixture>` compiles the fixture with both backends and requires identical stdout (`js_backend`, `js_backend_std`, `js_backend_closure`, `js_backend_json`; `scripts/js-backend-suites.sh` runs them all) |
| JS diagnostics | `scripts/js-backend-diagnostics.sh`: an unsupported construct fails the build with `file:line:column` for **every** site; a native-only `@extern` is explained as such; `--check` validates without writing |
| JS source maps | `scripts/js_backend/sourcemap_check.mjs` (run per fixture): decodes `mappings`, requires every source line in range and a mapping on a known statement's line |
| Modules | module-split violations fail (shared→client import, client→native symbol) |
| `dom` | `scripts/web-dom-test.sh`: `client/*.ch` using typed `dom` compiles in the `--web` client job, the emitted bundle calls `rt.dom.*` (never the raw extern names), registers `$fns`, and under `scripts/js_backend/dom_harness.mjs` produces the expected output for document/element/event/fetch/storage/URL; server-only HTML is absent from the client bundle and client-only symbols from the server executable; the source map decodes to the listener's line |
| Hydration | ids unique per instance; closure check (unreachable id = build error) |
| Bundles | duplicate-unit = 0; frozen chunk cannot grow; every chunk hashed |
| Shell | `shell.emit(empty)` byte-identical to full SSR; no component may render into a slot |
| Size | `scripts/web-probe.sh` regression gate + Phase 5 budgets |
| Cache | second page of a 2-page app: 0 duplicated runtime/component bytes |
| Static site | `scripts/static-site-test.sh`: both pages reference the same hashed bundle, nested pages use `../`, 0 inline `<script>`, per-page chunks hashed, probe `--strict` clean; `build()` writes nothing and `respond()` serves 200/304/404 with the right cache policy; the `webserve` bridge maps that onto `http::Response` (status, headers, 304, 503 for no site, body) and a fixed-port fixture repeats it over a real socket. 37 checks; `@test`s in the universal plugin suite cover the same contract |
| Server time | shell ≥ 5× full SSR; static ≥ 50× full SSR (same route, p50) |
| CSP | production output contains no inline `<script>` |
| Static | `--static` output shares runtime + vendor chunks across pages; `#html`-only pages emit 0 JS |
| Props | unknown/missing/mistyped prop = compile error with the declared prop set; **all 196 components typed** (a grep-based count in CI) |
| App | `scripts/web-app-test.sh`: one route table mixing pre-rendered and per-request routes on one bundle — pre-rendered `200`/`no-cache`/ETag/`304`, per-request `200`/`no-store`/no ETag with per-request data, `{id}` and `{path*}` captures, `308` redirects, a rendered 404 route, traversal refused, sitemap lists only pre-rendered routes, `robots.txt` points at it, mount path served with `/app/assets/...`, `web::serve_request` serves the installed app, `finish()` writes only the exportable routes, and the session/cookie/CSRF flow. 46 checks |
| Sessions | a form flow with cookies + CSRF works with JavaScript disabled (cookie jar + `web::Session` + CSRF are in; the no-JS form fixture is the remaining half) |
| Skills | every code block in `.agents/skills/web_*` compiles in CI; the skill's API list matches the compiler's diagnostics (0 drift); the reference app builds, serves and passes browser E2E |
| Determinism | render body reading time/random/DOM state = compile error; hydration-mismatch overlay green in E2E |
| Browser | `components-e2e`: route split, in-memory serving, head update, typed client, assets |
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
- **No HMR** (reload-only dev server); revisit after the stack is stable.
- **No async in the v1 JS backend.** The completion-handler form of `web::Client` is the permanent
  API; async is added later as an awaitable sibling, never as a replacement (Phase 6).
- **No exact 64-bit arithmetic in v1.** `u64`/`i64`/`size_t` are `number`s, exact to 2^53 — the
  stdlib needs them and `BigInt` would slow every length/index operation. Values past 2^53 are the
  one documented soundness bound, surfaced by a `--check` warning where reachable (Phase 7 adds
  exactness).
- **No typed canvas/pointer/WS in v1.** Apps use a typed Chemical wrapper over `#js`; the typed
  surface arrives in Phase 7 and the wrapper becomes an alias, so call sites never migrate.
- **No `@dispose`**, but destructors **do** run on the JS target (`std::string` frees heap memory).
- **No islands, SPA shell mode, multi-app vendor sharing, SRI, service worker/PWA** in v1.
- **No image transcoding** (PNG/BMP/PPM only; PNG *encode* lands in Phase 7) or WOFF2 in-tree.
- **No React compatibility layer.**
- **No streaming SSR** until Phase 8.
- **No ownership-on-GC decision** made here: proposals 0006/0004 own it.
- **No user-facing "chunk config".** Placement is derived and measured; if it needs hand-tuning,
  the derivation is wrong.
- **No v1-only convenience APIs.** If a design is temporary, it does not ship: it either becomes
  permanent or it is cut. This is the discipline that makes "stable on day 1" mean something.

---

## 8. Decisions

**Answered in the approval round** (2026-11-15), and now part of the plan:

| # | question | answer |
|---|---|---|
| 1 | JS backend scope in v1 | **Minimal but real** — sync subset only; async/`BigInt`/`dyn` deferred (§3.7). No v1 API taken on a bet. |
| 2 | Who builds the four apps | **Not this plan.** v1 ships **skills + migration kit + a one-sentence bootstrap prompt** so app agents adopt the stack correctly (§3.21). |
| 3 | Typed props rollout | **All 196 components typed in this release**; untyped props are a compile error from day 1. |
| 4 | Client network API without async | **Completion-handler `web::Client`**, designed so the awaitable form is added later without changing a call site (§3.10). |
| 5 | Asset delivery default | **Embedded by default**, disk supported (`--out dist`, `--static`). |

Settled earlier (kept so they can still be vetoed):

6. **Real modules** `shared`/`server`/`client`, each with its own `chemical.mod`; `shared` is pure
   logic only (no `page`/`dom`/macros). No `@server`/`@client` annotations.
7. **Route → bundle closures are derived by rendering each route once** and freezing the result.
8. **`assets.ch` uses one string literal per text chunk** (base64 for binary) and a
   **two-phase build**; no byte-array modules.
9. **Three delivery modes** (static / shell+slots / full SSR), per route, one pipeline. Slot fills
   are plain HTML; structural variation is a per-variant shell.
10. **Props typed via `@props`**, with the JSON props payload replacing dispatch text.
11. **Chunk placement is derived, never configured.**
12. **Sessions/cookies/CSRF are v1**, pulled forward from Phase 7.

Still open, to be settled during implementation (none block starting):

13. ~~**ABI route**~~ → **decided: Route A, emulated linear memory** (`js-backend-abi.md`).
    The remaining gate is the hand-written spike (§9 of that doc), not a choice.
14. **Bundler location**: `web_build` library/build script (recommended) vs inside the compiler.
15. **Runtime mangling scope**: budget 45 KB min until a shared JS parser lands.
16. **`--static` inference**: static-by-default when there is no `server/` module, or always two
    jobs. Recommendation: infer from the module set.

---

## 9. Sequencing

```
── v1 ──────────────────────────────────────────────────────────────
Phase 0  truth + fixtures + probe harness + useId fix        (S)
Phase 1  JS backend (minimal): ABI spike → emitter → diff     (XL, critical path)
Phase 2  manifest + typed props (196) + freeze/hash  (M, parallel)  ← starts immediately
Phase 3  module split + client modules + maps + serve         (L, needs Phase 1 ABI)
Phase 4  bundle graph + web::App + shell/slots + runtime      (L, needs 2+3)
Phase 5  minify + --static MPA + sessions + skills kit        (M)   ← release gate
── post-v1 (additive) ──────────────────────────────────────────────
Phase 6  SPA routes/prefetch/head + realtime + async           (L)
Phase 7  full DOM + canvas/pointer + uploads + i64            (L)
Phase 8  hardening + streaming + a11y + escape-hatch audit     (M)
```

### 9.1 Where v1 actually stands (2026-11-18)

Nothing here is a guess: each line names the artefact that proves the claim, and the open items are
ordered so the next agent can take the top one without re-reading the whole plan.

**Done and gated.** Phase 0 (fixtures, goldens, probe with gates/deferred split, CI);
Phase 1 steps 0–5 (ABI, emitter, differential suites, diagnostics through `MIRTargetSupport`,
source maps); Phase 2 (typed props on 172/184 components, defaults and `@optional` on both paths, implicit
universal attributes, per-instance hydration ids, `SharedAssets` freeze/hash, typed `Head`,
`page.StaticSite` + `webserve`, `templates/web-starter` + its gate). Gates:
`web-golden-test.sh`, `web-split-test.sh`, `web-probe.sh analyze … --strict`,
`static-site-test.sh`, `js-backend-suites.sh`, `web-starter-test.sh`, `web-app-test.sh`,
`test.sh --tcc --plugins` (1315), `test.sh --tcc --negative` (900). `static-site-test.sh` now also
covers the **mixed** case: one build serving pre-rendered pages from memory and a route rendered per
request against the same content-hashed bundle (both answered with their own cache policy).
**Phase 3 is largely in:** the two-job `--web` build with real `shared`/`server`/`client`
modules, source maps, and the minimal typed `dom` slice (`lang/libs/dom`) now runs a
`client/*.ch` interaction against a host DOM under Node (`web-dom-test.sh`).
`web-app-test.sh` (46 checks) covers the route table on top: per-route cache policy, `{id}`/`{path*}`
captures, redirects, the rendered 404 route, the mount path, sitemap/robots export, sessions,
cookies and CSRF.

**Open, in order of value per unit of work.** The order is application-first by decision (§3.12):
nothing that only the static export needs outranks work that makes a served app faster or more
capable, and the route-mode work (#2, then #5–#7) precedes the export extras (#3).

1. ~~**Optional-without-default props**~~ — **done 2026-11-18** (`@optional`, §3.15a). The control
   components it unblocked are typed; what remains untyped is Gap B (JSX element / array props),
   which needs an element type in the JSX surface rather than a props feature.
2. ~~**Per-request routes on the current stack**~~ — **done 2026-11-18** (the §3.12 bridge:
   `StaticSite.attach_dynamic` / `render_dynamic` / `dynamic_response` / `dynamic_error_message`). A project that pre-renders some routes and renders others per request — the
   normal shape of a real app — could not be expressed before this: `StaticSite` could only serve
   frozen bytes, and the playground pattern could only render everything per request with no shared
   bundle. Now a dynamic route renders into a fresh `HtmlPage` at request time, gets the site's
   frozen `<script src>`/`<link>` head tags (so the client bundle is byte-identical to the static
   routes'), and is answered `no-store` with no ETag, while its pre-rendered siblings keep their
   `no-cache` + ETag path. Guarded by `scripts/static-site-test.sh` (same bundle hash on both route
   kinds, two requests produce different bodies, no inline script, no validator on the dynamic
   answer). What is deliberately *not* here, and belongs to `web::App`: a route table that dispatches
   per route (the app still matches paths itself), `Vary: Cookie`, and a session-aware render hook.
3. **`web::Static` export output** (Phase 5 item 2) — **export extras done 2026-11-18**
   (`StaticSite.set_site_url` + `export_sitemap()`/`export_robots()`, absolute `asset_prefix` for a
   mount path, both gated by `web-app-test.sh`). Still open: the `--static` MPA command that calls
   them, hover prefetch, and the deprecated `writeToDirectory` shim.
4. **Node test-runner** (Phase 1's last item). `test_runner` spawns a child per `@test` and
   `TestEnv` reports over a pipe; under Node that needs an in-process `TestEnv` and a loop over
   `get_tests()`. Blocked on interface dispatch: `TestEnv` is an interface, and `DynamicCall` is a
   diagnostic on the JS target. Either bind the runner's env statically (a `NodeTestEnv` struct
   passed by value — no vtable) or land `DynamicCall` in the emitter. The former is smaller.
5. ~~**Phase 3 module split + `--web` + typed `dom`**~~ — **done 2026-11-18** (items 1–4 of
   Phase 3: real `shared`/`server`/`client` modules and the two-job build, `MIRTargetSupport`,
   source maps, and the minimal `dom` slice, gated by `web-split-test.sh` and
   `web-dom-test.sh`). **Also done 2026-11-19:** Phase 3 item 6 (`WEB-CLIENT-001`: a discarded
   `Result` in client code is an error) and the per-symbol target table under item 2
   (`MIRTargetSymbolRule`; native calls to the `dom` host API now fail during translation, and
   a swallowed parallel-emission failure that produced link errors instead of diagnostics is
   fixed). What remains from Phase 3 is item 5, `chemical serve`.
6. **`chemical serve`** (Phase 3 step 5): reload, in-memory chunks, dev error overlay, and the
   error-ingestion endpoint that turns a client exception into a source-mapped terminal report.
7. **Phase 4 bundle graph + `web::App`** (steps 1–4, 6) — **`web::App` done 2026-11-18**
   (`lang/libs/web/src/app.ch`): one route table mixing pre-rendered (`page`) and per-request
   (`live`) routes over one frozen bundle, per-route cache policy + `Vary`, `{id}`/`{path*}`
   captures filled into `req.route_params`, `not_found` and `redirect` classes, `set_base_path`,
   `build()`/`finish()` with the mandatory warm pass (a live route needing an un-bundled component
   is refused with 503, never served), and `install`/`serve_request` for the `http::Server` router.
   **Third mode done 2026-11-19:** `app.shell(pattern, handler, fills)` + `web::Shell`/
   `web::SlotFills` (byte-range slots, `page.slot("name")`, one splice per slot per request,
   ~95x faster than full SSR and byte-identical to it on the `app` fixture, gated by
   `web-app-test.sh`). **Unit graph + route-closure chunking done 2026-11-19** (Phase 4 items 1/3/7
   as recorded there: ranges + per-route usage recorded in `SharedAssets`, `plan_chunks` moves a
   route-only definition into that route's chunk, `scripts/web-graph.sh` asserts closure and no
   duplicates;   `fixtures/router` bundle 141,625 B → 139,837 B and `fixtures/app` shows
   `chunked=1 retained=1`, i.e. a per-request component stays shared). **Phase 4 is now complete
   apart from CSS:** `vendor` is its own chunk, `page.Bundle` serves an embedded artifact set,
   `StaticSite.write_assets_module` emits it as source (two-phase build proven end-to-end by
   `web-embedded-test.sh`), and `app.shell_variant(..., when)` picks between frozen shells per
   request. Still open: a `web_build` *command* for that orchestration, CSS chunks +
   `modulepreload`, `Bundle` loading from disk, lazy chunking, and item 5's runtime extraction. Step 5 and 7's static half
   are satisfied by `page.StaticSite` and the probe script.
8. ~~**Sessions + CSRF**~~ — **done 2026-11-18** (`http.CookieJar`/`CookieOptions`/
   `set_cookie_value`/`Response.cookie`, `web::SessionStore` + constant-time CSRF, gated by
   `web-app-test.sh`). The durable store landed 2026-11-21 (see Phase 5 item 3 above); the
   `Vary: Cookie` render hook remains.
9. **Per-request serving cost** — **zero-copy bodies done 2026-11-20**: `http::ResponseBody.View`
   holds a borrowed payload and the encoder writes it straight from the view, so an immutable
   asset no longer costs a `memcpy` of its own size on every request. `webserve.serve` and
   `web::App.serve_artifact` use it via `response_of_borrowed` (`public unsafe func`: it asserts
   the bytes outlive the response — true for a frozen artifact, false for a rendered body).
   `response_of` stays the copying default for exactly that reason. Measured A/B on the
   `static_site` fixture (300 requests per path, one process, three runs each, same binary
   and fixture, only that one call site changed): a 108,520 B bundle asset **4,417 → 2,389
   ns/req (1.85x)** while the 466 B page stayed 1,307 → 1,316 ns — serving is now per request,
   not per byte (the copy cost 3,110 ns for 108,520 B, i.e. ~35 GB/s of `memcpy`; nothing is left
   that scales with body size). `static-site-test.sh` gates the structural fact
   (`body.is_view()`), the ratio (asset < 2.5x a small page; the copy measured 3.4x), and that a
   borrowed body is written whole (`Content-Length` == bytes received). Next on this line: a
   cached response head, then precompressed (`gzip`) immutable assets with `Vary: Accept-Encoding`.
10. **Minifier** (Phase 5 item 1) — **JS + CSS whitespace/comments done 2026-11-20**:
    `lang/libs/page/src/minify.ch` (`minify_js`/`minify_css`, deterministic, no
    dependencies), wired through `SharedAssets.set_minify`, `StaticSite.set_minify` and
    `web::App.set_minify` (opt-in). The bundle graph is why this is more than a string pass:
    `freeze()` minifies the buffer **in pieces** — each unit's bytes and the gaps between them
    — and recomputes every range as it writes, carrying whitespace state across the seams; it
    then self-checks that each range still holds exactly one definition and restores the raw
    bytes if not. A regular-expression literal, an unterminated string or comment makes that
    piece (and therefore the bundle) keep its original bytes instead of guessing. Measured on
    the `app` fixture's runtime bundle: **108,520 → 48,283 B (−55%)**, `node --check` clean,
    chunk plan identical (`graph chunked=1` still passes on minified bytes). Unit tests live in
    `lang/tests/libs/page/src/minify.ch` (13 tests: ASI newlines, token-separating spaces,
    strings/templates, regex fallback, division, comments, CSS last-`;`). **Still open from item
    1:** emitter identifier mangling, tree-shaking, gzip precompression, numeric budgets in the
    gate, `--profile`. Then the **skills kit** (item 4) and `skill conformance` in CI.

**Explicitly not started, and why:** Phase 6 (SPA navigation, realtime, async in the JS backend)
needs `web_build`'s route-closure chunk graph; Phase 7 (the rest of `dom`: canvas, uploads,
pointer/rAF, `WebSocket`) builds on the slice that now exists; Phase 8 is ongoing hardening.

Phase 2 is independent of Phase 1 and delivers the headline sharing and typing wins in one
workstream while the backend is built in the other. If a phase slips, the *cut* is always taken
from a later release, never by weakening the v1 public surface: a half-designed API shipping in v1
is worse than a feature shipping a release late.

---

### 9.2 Survey: five probe apps against the published API (2026-11-21)

Before freezing v1 we wrote five apps using **only** the public API and measured them
(`lang/tests/web/probes/`, gate `scripts/web-probes-test.sh`, full findings incl. numbers in
[`web-stack-gaps.md`](web-stack-gaps.md)):

| probe | shape | result |
|---|---|---|
| `shop` | storefront: frozen pages + per-request renders + shell, sessions, CSRF, POST, PRG, JSON, user input in HTML | 22 checks green; ~0.36 ms/SSR render (5,500 in 2.03 s, 7.5 MB RSS); 48 KB bundle on pages with no component (**now 25 checks, no bundle**) |
| `blog` | content site: 47 pre-rendered pages, tag pages, one shared island | 0.16 s build+write, 3.8 MB RSS, 1.8 KB avg HTML, 0 `<script>` on component-free pages, 40 identical chunks → 40 URLs |
| `spa` | single-page app: `--web` split, shell, JSON endpoint, typed client | 7 checks green; the emitted `web-client.mjs` (217 KB / 27.9 KB gz) is **not referenced by the served page** |
| `tasks` | authenticated multi-user app: `page`+`live`+`shell`+`endpoint` on shared paths, sessions, form POST→303, per-user lists, CSRF | 31 checks green; **~15 µs/render**, 337 B shell, **0 B JS** on a component-free page, 3 artifacts |
| `docs` | documentation site: 43 generated pages, sidebar, prev/next, sitemap/robots, JSON search index, gzip | 11 checks green; build **~50 ms**, 136,565 B HTML, biggest page 3,688 B, **0 scripts / 0 B JS** |

The survey also found a real parser bug, fixed here: `@{ ... }` could not span lines — and, in the
second round, a **silent use-after-free in `Static.add`** (pages registered from a loop produced wrong
HTML with no error), fixed by making the site own its pages: `Static.page(path) : &mut HtmlPage`.

**What it changes in this plan.** The v1 surface is missing four things an app cannot work around
(ordered; each is additive plus one deliberate break):

1. **`App.endpoint(method, pattern, (req) => Response)`** and public `web::json`/`web::redirect`. No
   route can answer a status, a header or JSON today, and `handle` ignores `req.method`, so every
   mutating app hand-writes dispatch in front of `App.handle` (the `shop` probe's ~120 lines).
2. **Form parsing** (`application/x-www-form-urlencoded`, then `multipart`) and `web::Form`, which
   the frozen list already promises.
3. **Emit the hydration runtime only when a component was used** and content-address per-route
   chunks, so a component-free page serves 0 B of JS and the blog's 40 identical chunks become one
   artifact. Both are measurable wins on the common shape.
4. **Connect the client job to the page bundle** (`App.client_bundle(...)` or a module marker) —
   without it Phase 3's typed client work cannot ship in a page.

Then one design decision that must be taken with them: **escape `{}` by default** with an explicit
raw marker for markup (item 4 in `web-stack-gaps.md` §P0-5). It is the only finding that is a
security bug rather than friction, and it is cheap now, expensive after apps exist.

**Progress (2026-11-21):** items 1–3 are implemented and green (`web-stack-gaps.md` carries the
measured deltas: `shop` 22 → 25 checks, `/` 432 B → 270 B with no `<script>`, `blog` 100 → 57
artifacts), and so is P1-6/P1-7 — routes now receive a `web::RequestContext` that carries the
request and the session the app resolved, instead of recovering both from globals. Item 4
(`App.client_bundle`) and the `{}` escape-by-default decision are the remaining P0 work.
The next tier (a `RequestContext` instead of handler-side globals + `unsafe`, a `Session` handle,
`Static.add` taking ownership of its path, `Request.make`/`now_ms` for tests, and the missing `dom`
members for an SPA) is listed as P1 there.

**Second round (2026-11-21), after `tasks` and `docs`:** the two shapes the first three could not
cover (per-user mutable state over a session; ~50 similar routes) changed the API in four places,
all landed and gated by the probes:

- **`web::Static.page(path) : &mut HtmlPage`** replaces `add(&mut page, path)` as the way to create a
  route. The site owns the pages (vectors reserved to `PAGE_CAPACITY = 512`) and copies the path, so
  loop-built routes are safe; `add` remains for a page the caller already owns. This closes P1-8 and
  fixes a silent use-after-free that had already corrupted the `blog` probe's output.
- **`add_artifact(path, bytes, content_type)`** on `StaticSite`/`web::Static`, plus `artifact_bytes`
  and `has_gzip_variants`, so a search index / feed / manifest is part of the set (this was a P2 item
  "not expressible" and is now three lines).
- **Endpoint/page dispatch on a shared path** answers GET from the page route and 405 only for methods
  no route lists (a real bug: `app.live("/todos")` + `app.endpoint("POST", "/todos")` returned 405
  to GET).
- **`RequestContext` reaches every route shape except a shell fill** — `SlotValues` is still
  `(req : &Request) => FillSet`, so a shell cannot render the signed-in user from the session. That,
  `App.state<T>` (app-owned state instead of a global behind `unsafe`), `http::Request.make`, the
  client-bundle wiring (P0-2) and the `{}` escape decision are the six items in the next-round table at
  the end of `web-stack-gaps.md`.

Measured on the two new shapes, the plan's two headline claims hold: a precomputed app answers a
per-user render in **~15 µs** with a 337 B frozen shell, and a 43-page content site ships **0 bytes of
JS** while building in **~50 ms**.

## 10. How we will know it worked

**v1 — the release bar:**

- Two pages sharing components: runtime + vendor + 63 components load **once**; duplicate unit
  count = 0 (measured by `web-probe.sh`, not by eye).
- A page's initial JS ≤ 50 KB min; runtime ≤ 45 KB min (35 KB after runtime mangling).
- A personalised route is served from a frozen shell in one allocation plus one `memcpy` per slot,
  byte-identical to full SSR, and ≥ 5× faster (measured).
- **An application, not just an export:** the same binary serves pre-rendered routes from memory
  (`no-cache` + ETag), a personalised route per request (`private`/`no-store`), and a shell route
  from the frozen buffer — all referencing one content-hashed bundle, and a route can move between
  modes without changing a client URL. Per-request SSR of the components app is measured and
  budgeted, not assumed.
- All 196 components have typed props; a typo'd prop, a typo'd DOM member, a nondeterministic read
  in a render body, and an `i64` in client code are all compile errors with a fix.
- A form flow with sessions and CSRF works with JavaScript disabled; the same flow works with
  client modules when JS is on.
- `chemical build --web` yields one embedded binary; `chemical build --web --static` yields a site
  whose pages share runtime and component units, where `#html`-only pages ship 0 bytes of JS, with
  a `base_path`, a sitemap, and hashed assets.
- Same program under `--tcc` and `--js` produces identical output (differential gate).
- Dev: edit → reload < 1 s; every chunk has a source map; a client error appears in the terminal
  with a Chemical source location.
- **An agent that has never seen this repo can take the one-sentence prompt (§3.21), load the
  skills, and migrate a real page to the new API without asking a question** — and every snippet
  it copied still compiles, because CI compiles the skills.

**Evidence that later releases stayed additive:** after v1, no user-visible rename, no changed
signature, no changed `chemical.mod` shape. A later release that needs one is a bug in this plan.
