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
| minify headroom (comments + whitespace, exact) | **8.0%** raw | **14.2%** raw | no minifier exists; identifier mangling is on top |
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
  Limitation: a **raw JS literal** inside a JSX body (`count={"nope"}` in a `#universal` body) has
  no Chemical type, so it is not value-checked there; the `#html` path is fully checked.
- **Defaults** — `var count : int = 0` in a props struct is **rejected** in v1 rather than
  half-applied: the value would have to be injected into both the SSR attrs and the client props
  payload, and applying it in only one makes SSR and hydration disagree. The CBI now exposes
  `BaseDefMember.getDefaultValue()` so the generated serializer can own defaults as one change.
  Until then every declared prop is required.
- **Serializer from the type** — replace the ad-hoc prop payload building with generated JSON, which
  removes the documented escaping footguns (single quotes/backslashes, struct/vector props)
  by construction.

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
| JS + CSS minifier, hashed names, `--static` MPA with cross-page sharing, `--profile` | 5 |
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
| `web::App`, `web::Bundle`, `web::Shell`, `web::SlotFills`, `web::Client`, `web::Session`, `web::Form`, `web::Static` signatures | minifier quality, mangling scheme, runtime size |
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

### Phase 0 — Truth, fixtures, and the probe harness  *(S)* — **in progress**

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
- [ ] Remaining fixtures: `static` (pure `#html`, asserts 0 JS bytes), `router` (2 routes),
      `shell` (one route with slots), plus golden HTML/JS/CSS snapshots for all four.
- [ ] Router cheap wins: `noindex` on missing/fallback routes; typed `Head`; keep the existing
      static-export route manifest.
- [ ] Wire the probe into CI (a `web-probe` job) so a bundle regression fails the build.
- **Gate:** golden files for the 4 fixtures + the probe green in CI.

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
   `for-in`, closures (`std::function`, capturing lambdas). — **landed** for everything except
   `for-in` over a *custom* `Iterable`/`Cursor` implementation (std's own iterators work; the shape
   is the same `CallIndirect` path, so this is untested rather than unimplemented).
4. Destructors/drop glue emitted as on C (`std::string` frees); borrow checking unchanged; the
   *ownership* decision for GC targets is still owned by proposals 0006/0004. — **landed**.
5. Interfaces/vtables/`dyn`; async, atomics, inline asm, `DynamicCall`, `Throw` are **diagnostics**
   through `MIRTargetSupport`, reporting the callee/type and the Chemical source location. — **partially
   landed**: the emitter rejects all of these with the function name, the opcode and the *source
   position*, reports **every** offending site in one build, and `--check --emit-js` runs the same
   rules without writing anything (so a CI job can ask "does this compile to JavaScript?").
   `scripts/js-backend-diagnostics.sh` is the gate, including the native-symbol case: a call to an
   `@extern` C function is reported as *"native symbol 'getenv' has no JavaScript equivalent
   (declared @extern); a client or shared module must not depend on it"*, not as a module problem.
   The shared `MIRTargetSupport` table is still not written, so the LSP cannot report these before
   emission.
6. ESM emission: one module per Chemical module; shared mangling. — **deferred to Phase 3**: a bundle
   is one program module plus `index.mjs` and `runtime.mjs` today.
7. Test harness: differential **C-vs-JS conformance** (same program, identical stdout) + a
   Node runner path (the current runner spawns child processes over Win32 pipes and prints
   via `printf`). — **differential harness landed** (`scripts/js-backend-test.sh`,
   `scripts/js_backend/run.mjs`); the test-runner path comes with the suites.

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
   checking** against the field type (via the type checker's own `satisfies`), and an explicit
   rejection of default values. Negative tests cover unknown prop, missing prop, props type not
   found/must be a struct, value-type mismatch, and defaults; a positive test proves declared props
   and implicit props render. Remaining: the generated props serializer (which also brings defaults
   back), and value-type checking for raw JS literals inside a JSX body (they carry no Chemical type
   — the `#html` path is fully checked).
4. [x] **Hydration-safety analysis** (§3.15b): **landed** — render bodies may not read the clock, a
   random number, or browser-only state (`document`/`window`/`localStorage`/`fetch`/…); lambdas are
   exempt because the framework only passes them to handlers and effects. Negative tests cover a
   clock read, a browser-state read, and the handler case (must compile). It flagged two of our own
   fixtures, both genuine — they were migrated, not exempted. Remaining refinement: state
   initializers expressed as lambdas are currently exempt (a documented false negative).

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
4. **Source maps** from the start of generated client code. — **landed in the emitter**
   (`--emit-js` writes `<program>.mjs.map` + a `sourceMappingURL` comment; one segment per
   instruction line, mapping to the Chemical `file:line:column` the instruction came from). The gate
   *decodes* the map (`scripts/js_backend/sourcemap_check.mjs`): every source index and line must be
   in range and one mapping must land on the line that produced a known statement. The first
   implementation was wrong in a way only a decoder catches — the line cursor advanced after writing
   a segment, so every mapping after the first was appended to the first group.
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
   `modulepreload` per §3.13.
7. Closure / duplicate / frozen-chunk / static-vs-shell checks in CI.

**Exit:** the components app is served by `web::App` from an embedded bundle; two pages share
runtime + vendor; each route loads only its chunk; a personalised route is served from a slot shell
byte-identically to full SSR; no inline `<script>`.

### Phase 5 — Production output, static sites, sessions, and the skills kit  *(M)* — **last v1 phase**

1. Minifier (whitespace + emitter identifier mangling), CSS minifier, tree-shaking, gzip
   precompression or CDN delegation, budgets from Phase 0 enforced on minified bytes, `--profile`
   (per-route bytes, chunk composition, navigation cost).
2. **`--static` MPA build** (§3.14): shared chunks across all pages, zero-JS pages stay zero-JS,
   `base_path`, `sitemap.xml`/`robots.txt`, optional hover prefetch, `web::Static.build` replacing
   `writeToDirectory` (deprecated shim for one release).
3. **Sessions** (pulled forward from Phase 7): cookie jar, `Set-Cookie` parsing, `web::Session`
   with a server-side store interface, CSRF tokens for state-changing endpoints, same-origin
   default. No real app ships without these, so they gate v1.
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
| Sessions | a form flow with cookies + CSRF works with JavaScript disabled |
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

Phase 2 is independent of Phase 1 and delivers the headline sharing and typing wins in one
workstream while the backend is built in the other. If a phase slips, the *cut* is always taken
from a later release, never by weakening the v1 public surface: a half-designed API shipping in v1
is worse than a feature shipping a release late.

---

## 10. How we will know it worked

**v1 — the release bar:**

- Two pages sharing components: runtime + vendor + 63 components load **once**; duplicate unit
  count = 0 (measured by `web-probe.sh`, not by eye).
- A page's initial JS ≤ 50 KB min; runtime ≤ 45 KB min (35 KB after runtime mangling).
- A personalised route is served from a frozen shell in one allocation plus one `memcpy` per slot,
  byte-identical to full SSR, and ≥ 5× faster (measured).
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
