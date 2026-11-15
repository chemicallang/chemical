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
an in-memory app/serve model instead of "build an executable that dumps a site"; corrected
measurements; the JS-backend ABI decided before the emitter; the review's cheap wins moved
earlier; a cut list.

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

---

## 4. Phases

### Phase 0 — Truth, fixtures, and the duplicate-id fix  *(S)*

- Re-measure output with **content-based** metrics (runtime bytes/share, dispatch-text bytes,
  duplicate-unit bytes) and publish the table in §0 as the baseline.
- Add `scripts/web-size-report.sh` (CI: fail on regression) and 3 golden fixture apps:
  static, universal, router.
- Fix hydration ids to instance counters (today they are source-location based, so two
  instances of one node share an id — a correctness bug independent of everything else).
- Take the router's cheap wins: `noindex` on missing/fallback routes; keep the existing
  static-export route manifest.
- **Gate:** golden HTML/JS/CSS for fixtures + the size report.

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

### Phase 2 — Cheap wins, no dependency on Phase 1  *(M, parallel)*

Can run in a second workstream while Phase 1 is in progress.

1. **Hydration manifest** (§3.6): replaces dispatch text; 42% smaller pages; removes
   `window[fnName]`; props serialized once.
2. **`SharedAssets` freeze + content-hashed names + ETag + `toStringJsOnly`**: the
   shared-bundle design's Phase 4. This alone makes two pages share the runtime and the 63
   components today.
3. Router: per-route `<title>` already exists; add `meta`/canonical **only if** the head
   manifest design is accepted (otherwise keep it a documented non-goal).

**Exit:** page 2 of a two-page app ships no runtime and no shared component bytes; the
closure check fails the build on an unreachable dispatch.

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
5. `chemical serve` (reload, no HMR): SSR + chunks from memory + error overlay.

**Exit:** an onboarding example where a route's interaction is written in `client/*.ch`,
compiled to JS, and works in the browser with source maps.

### Phase 4 — Bundle graph, in-memory serving, assets  *(L)*

1. `web_build` library: unit graph, chunking by the derived route closures, hashing, manifest,
   and the **two-phase orchestration** (client build → `assets.ch` → server build).
2. `web` module: `App` / `Bundle` — route table, in-memory render, `/assets/*` with ETag,
   embedded-or-disk loading. Requires making `Server.dispatch` public (or a listener-free
   `App`) and adding an in-memory asset map (none exists today).
3. **Closure derivation**: render each route once at build/first start to collect
   `require_*` ids, map ids → units, assign shared units to `vendor`, freeze the graph.
3. Runtime extracted from `page.ch` into `lang/libs/page/runtime/*.js`, embedded at build time
   (a LabBuildCompiler asset-embed step — owned here, with a byte budget).
4. CSS chunks + `<link>` (CSP-safe), `@layer` ordering, asset manifest.
5. Closure/duplicate/frozen-chunk checks in CI.

**Exit:** the components app is served by `web::App` from an embedded bundle; two pages share
runtime + vendor; each route loads only its chunk; no inline `<script>`.

### Phase 5 — Production output  *(M)*

Minifier (whitespace + emitter identifier mangling), CSS minifier, tree-shaking, gzip
precompression or CDN delegation, budgets from Phase 0 enforced on minified bytes, a
`--profile` report (per-route bytes, chunk composition).

**Exit:** budgets green; `chemical build --web` emits a deployable directory *or* an embedded
`assets.ch`.

### Phase 6 — SPA behaviour and data  *(L)*

Route-level `import()` + prefetch (port the existing `remote`-route fragment machinery and its
race handling, rather than reinventing), head `meta`/canonical (if accepted), `web::Client` +
shared models, optional route-table typed endpoints, progressive forms, client-side
navigation without document reload.

**Exit:** route change loads one chunk, updates title, fetches typed data, no reload.

### Phase 7 — Full DOM types and security defaults  *(L)*

Complete proposal 0008 (`dom` submodules, host-handle borrow rule — a language change, not a
library), DOM event typing in `#universal`, `--check` rejects unknown DOM members.
Security defaults: same-origin by default, CSRF tokens for state-changing endpoints, cookies
(absent from `http` today), CSP-friendly output, secrets from env at build/serve time.

### Phase 8 — Hardening  *(M, ongoing)*

Streaming SSR; sampled hydration-mismatch reporting (needs the error endpoint); `http::FileServer`
cache headers/ETag (needed by disk mode); a11y linting; i18n/l10n + RTL (design in Phase 6 so
the manifest can reserve a locale dimension); perf budgets (LCP/TBT) in `components-e2e`.

---

## 5. File-level work map

| area | files |
|---|---|
| JS emitter | `compiler/mir/MIREmitterJs.{h,cpp}`, `MIRDeclPrinter` (printer interface), `compiler/lab` (js job), `TargetData` (`js`/`client`/`server`) |
| ABI | `lang/docs/js-backend-abi.md`, MIR type facts for runtime-provided types |
| Target support | `compiler/mir/MIRTargetSupport.*` (new) |
| Module split | `compiler/processor/*` (conditional sources already exist), `ModToLabConverter`, CLI `--web` |
| Bundle build | `lang/libs/web_build/*` (new module, Chemical: graph, manifest, minifier, two-phase build) |
| App/serve | `lang/libs/web/*` (**new module**: `App`, `Bundle`, `Client`, asset map) — do not graft onto `http`'s `web` namespace |

| Runtime | `lang/libs/page/runtime/*.js` + build-time embedding |
| Components | `universal_cbi/src/react/ast_replace.ch`, `emit_js.ch`, `converter/*` (units instead of `pageJs`) |
| Hydration | `lang/libs/page/src/page.ch`, `ssr.ch`, `html_cbi/.../component.ch`, `main.ch` |
| Router | `lang/libs/router/*`, `page/src/router_runtime.ch` |
| DOM | `lang/libs/dom/*` (per 0008) |
| Dev | `compiler/core/main/*` (`serve`), `lang/libs/web_build/dev.*` |
| Tests | `lang/tests/web/*`, `compiler_plugins/universal/*`, `components-e2e` |

---

## 6. Gates

| layer | gate |
|---|---|
| JS emitter | golden `.mjs` snapshots; `MIRTests` target-support assertions |
| JS semantics | **differential C-vs-JS conformance** (same program, identical stdout) |
| Modules | module-split violations fail (shared→client import, client→native symbol) |
| Hydration | ids unique per instance; closure check (unreachable id = build error) |
| Bundles | duplicate-unit = 0; frozen chunk cannot grow; every chunk hashed |
| Size | `web-size-report.sh` regression gate + Phase 5 budgets |
| CSP | production output contains no inline `<script>` |
| Browser | `components-e2e`: route split, in-memory serving, head update, typed client, assets |
| Engine | `#universal_test` WebView suite unchanged |
| Compiler | ≤ 5 MB release, per-PR size delta; compile-time/RSS deltas for `--tcc` and `--llvm`; no `ast/` include in `MIREmitterJs`/`web_build` |

---

## 7. Non-goals and cuts (deliberate)

- **No `@server`/`@client` annotations.** The boundary is the module split (§3.1).
- **No external bundler** (esbuild/rollup/webpack): the graph and minifier are ours, in a
  build script, not in the compiler.
- **No brotli/deflate in-tree**; `.gz` at most, CDN for the rest.
- **HMR cut** (reload-only dev server); revisit after the stack is stable.
- **Async in the JS backend deferred** (stubbed with a diagnostic) — not needed for SSR,
  routing, or client DOM code.
- **`BigInt` for `i64/u64` deferred**: reject in client code initially.
- **No `@dispose`**; destructors are no-ops on JS.
- **No islands, SPA shell mode, multi-app vendor sharing, SRI, service worker/PWA** in v1.
- **No image transcoding** (PNG/BMP/PPM only) or WOFF2 in-tree.
- **No React compatibility layer.**
- **No streaming SSR** until Phase 8.
- **No ownership-on-GC decision** made here: proposals 0006/0004 own it.

---

## 8. Decisions needed for approval

Settled by this revision (from the design review), listed so they can be vetoed:

1. ~~`@server`/`@client` annotations~~ → **real modules** `shared`/`server`/`client`, each with
   its own `chemical.mod`; `shared` is pure logic only (no `page`/`dom`/macros).
2. **Route → bundle closures are derived by rendering each route once** (build or first start)
   and freezing the result, not by a native route table (which has no render function).
3. **`assets.ch` uses one string literal per text chunk** (base64 for binary) and a
   **two-phase build**; no byte-array modules.

Still open:

4. **ABI route** (A) emulated heap vs (B) place→value promotion — Phase 1 step 0 decides with a
   spike; approve the spike as the gate.
5. **Embedded assets by default** (`assets.ch` in the binary) vs disk-first. Recommendation:
   embedded default, disk supported.
6. **Bundler location**: `web_build` library/build script (recommended) vs inside the compiler.
7. **Runtime mangling scope**: budget 45 KB min until a shared JS parser lands.
8. **Head meta/canonical**: accept the head-manifest design or keep it a documented non-goal.
9. **Typed endpoint route table** (optional convenience) in or out of v1.


---

## 9. Sequencing

```
Phase 0  truth + fixtures + useId fix                     (S)
Phase 1  JS backend: ABI spike → emitter → conformance     (XL, critical path)
Phase 2  manifest + freeze/hash            (M, parallel)  ← can start immediately
Phase 3  module split + client modules + maps + serve      (L, needs Phase 1 ABI)
Phase 4  bundle graph + web::App + in-memory/embed         (L, needs 2+3)
Phase 5  minify/tree-shake/budgets                         (M)
Phase 6  SPA routes/prefetch/head/data                     (L)
Phase 7  full DOM types + security defaults                (L)
Phase 8  hardening                                         (M)
```

Phase 2 is independent of Phase 1 and delivers the headline sharing win in one workstream
while the backend is built in the other.

---

## 10. How we will know it worked

- Two pages sharing components: runtime + vendor + 63 components load **once**; duplicate
  unit count = 0 (measured).
- A page's initial JS ≤ 50 KB min; runtime ≤ 45 KB min (35 KB after runtime mangling).
- Route change loads only that route's chunk, updates the title, fetches typed data, no reload.
- Client logic is Chemical in `client/`, and `--check` catches DOM typos.
- `chemical build --web` yields either one embedded binary or a hashed `dist/` with a manifest;
  `web::App` serves both.
- Same program under `--tcc` and `--js` produces identical output (differential gate).
- Dev: edit → reload < 1 s on the components app; every chunk has a source map.
