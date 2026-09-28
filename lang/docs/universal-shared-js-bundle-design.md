# Shared JS/CSS Bundles across Pages — Design

**Status:** Implemented (Phases 1–3). Phase 0 (`toStringJsOnly` page-level split)
and Phase 4 (freeze/diagnostics, content-hashed names) are pending.

**Scope:** `lang/libs/page` (`HtmlPage` buffers, routing of appends),
`lang/libs/universal_cbi` (runtime/definition/registry emission),
`lang/libs/html_cbi` (dispatch emission).

**Related:** `lang/docs/universal-runtime-professionalization-plan.md`
(§2.1–§2.2: segmented JS buffer, shared runtime),
`lang/libs/page/src/shared_assets.ch` (sink),
`lang/tests/compiler_plugins/universal/src/shared_assets.ch` (tests).

---

## 1. Summary

When several pages use the same components, each rendered page carries its own
copy of the hydration runtime, the component definitions it uses, and the
component CSS. There is no supported way to pull the **page-independent** JS and
CSS out of a set of pages into shared bundles that the user can serve separately
and cache once.

This document specifies one small, low-level utility to do that:

- a shared sink (`SharedAssets`) for page-independent bytes — runtime, component
  definitions, router registry, component CSS;
- a page keeps the page-specific remainder — SSR HTML, dispatch statements,
  page JS/CSS tail;
- the user creates pages manually, attaches them to the shared sink, renders
  them, and then takes the bundle and per-page remainder out however they like.

There is deliberately **no** site abstraction, no route management, no serving,
and no page classification. It is a bundling utility; the user wires the rest.

---

## 2. Problem

`HtmlPage` holds flat buffers:

| Buffer | Contents |
|---|---|
| `pageHead` / `pageHeadJs` | head runtime (dispatch/queue/error) |
| `pageJs` | body runtime (signals, vnode, hooks, mount, hydrate), component definitions, router registry, dispatches |
| `pageCss` | all classes used by the page |
| `pageHtml` | SSR markup |
| `pageJsEnd` | `$__universal_flush()`, router activation tail |

Component definitions are emitted during rendering, gated by
`page.require_component(hash)` and de-duplicated **per page**
(`lang/libs/page/src/page.ch:287`, `doneComponents`). Rendering N pages that use
the same components produces N copies of the runtime and N copies of every shared
definition and class.

`page.toStringJsOnly()` (`page.ch:818`) returns `pageJs`, which still contains
the page's own dispatches, so it cannot be served as a cross-page bundle. This is
the bug guarded by
`component_features.ch::feature_external_bundle_has_no_page_specific_dispatch`.

---

## 3. Scope

**What this is:** a low-level way to route the page-independent part of rendered
pages into a shared bundle, and to obtain the per-page remainder.

**What this is not:**

- not a site/framework feature — no page registry, no routes, no serving;
- not page classification — nothing inspects whether a page is "static";
- not applicable to request-time bundling — if a page is rendered per request,
  the user owns how (or whether) its bytes are shared.

**Assumption:** a user who builds pages at request time understands that, and
does not need us to share those bytes. This design leaves that path untouched.

**Hard guarantee:** a page that never attaches a shared sink behaves
byte-for-byte and allocation-for-allocation as it does today.

---

## 4. Design

Two sinks for JS and two for CSS:

| Sink | JS | CSS |
|---|---|---|
| **Shared** | runtime + component definitions + router registry | component classes |
| **Page** | dispatch statements + `pageJsEnd` tail | page-specific classes |

`HtmlPage` gains an optional pointer to a `SharedAssets`. While attached,
page-independent appends are routed to the shared sink; explicitly-local appends
stay on the page. While not attached, everything stays on the page exactly as
today.

The user controls when this happens: create a shared sink, attach it to each page
before rendering, render all of them, then read the bundle and the pages. Because
definitions are de-duplicated when they are emitted, the bundle accumulates the
union of the set with no extra pass.

```
shared = shared_assets()
  page A: attach → render → A.local = dispatches, A.html = SSR
  page B: attach → render → B.local = dispatches, B.html = SSR
  page C: attach → render → C.local = dispatches, C.html = SSR
shared.js / shared.css = runtime + A∪B∪C definitions/classes
```

---

## 5. Routing rules (exactly which bytes go where)

When `page.shared != null`:

| Emission | Target | Why |
|---|---|---|
| `defaultUniversalSetup` runtime | shared JS (once) | page-independent |
| component definition (`function Name(props){…}`) | shared JS | keyed by declaration location; identical on every page |
| component `style { … }` CSS | shared CSS | follows the component |
| router runtime / registry / match table | shared JS | compile-time, page-independent |
| `#globalcss { … }` | shared CSS (page fallback) | app-wide global CSS |
| `#globaljs { … }` | shared JS (page fallback, once) | page-independent app-wide JS |
| dispatch statements (`window.$__uni_dispatch(...)`) | **page** JS | keyed by call-site location; page-specific |
| `$__universal_flush()`, router activation tail | **page** `pageJsEnd` | page/request tail |
| `#css { … }` | **page** CSS always | pairs with the page's own `#html` |
| `#js { … }` | **page** JS always | pairs with the page's own `#html` |
| `#html` SSR markup | **page** `pageHtml` | always page-specific |
| head/meta | **page** `pageHead` | page-specific |

Mechanically:

- `append_js*`: `page.shared != null && local_js_depth == 0` → shared JS, else
  page JS. `append_css*`: same with `local_css_depth` / shared CSS.
- `require_component` / `set_component_hash` and `require_css_hash` /
  `set_css_hash` / the random-class helpers use the shared maps, so definitions
  and classes are de-duplicated across every attached page.
- `begin_local_js()` / `end_local_js()` bracket `#js` and dispatch emission, so
  neither reaches the shared sink.
- `begin_local_css()` / `end_local_css()` bracket `#css`, so page-level CSS never
  reaches the shared sink. `#globalcss` and component `style { }` are not
  bracketed and therefore prefer the sink (page fallback when unattached).
- `#globalcss` reuses the `css` embedded value with `CSSOM.shared = true`;
  component `style { }` sets the same flag (`universal_cbi/src/main.ch`).
- `#globaljs` sets `JsRoot.shared = true` and wraps its emission in
  `if(page.require_js_hash(loc)) { page.set_js_hash(loc); … }`, so it prefers the
  sink and is emitted once per source location (`js_cbi/src/main.ch`,
  `page/src/shared_assets.ch` `done_js`).

`js_hoist_pos` / `move_js_range` need no change: when definitions are routed to
the shared sink, the range moved on `pageJs` is empty and the move is a no-op.

---

## 6. API (low-level only)

Implemented in `lang/libs/page/src/shared_assets.ch` and `page.ch`.

```chemical
// lang/libs/page/src/shared_assets.ch

public struct SharedAssets {
    var name : std::string
    var js_data : std::string             // runtime + definitions + registry
    var css_data : std::string            // component classes
    var done_components : std::unordered_map<ubigint, bool>
    var done_classes : std::unordered_map<ubigint, bool>
    var done_random_classes : std::unordered_map<ubigint, bool>
    var runtime_emitted : bool = false
}

// Heap-allocated so all pages share one instance by pointer.
public func shared_assets(name : std::string_view = "") : *mut SharedAssets

// Buffers, for the user to serve however they want.
public func (s : &SharedAssets) js(&self)  : std::string_view
public func (s : &SharedAssets) css(&self) : std::string_view
public func (s : &SharedAssets) js_size(&self)  : ubigint
public func (s : &SharedAssets) css_size(&self) : ubigint

// Optional convenience: writes "<dir>/<base_name>.js" and "<dir>/<base_name>.css".
// The user may instead hash/serve js()/css() themselves.
public func (s : &SharedAssets) write_to(dir : &std::string_view,
                                         base_name : &std::string_view) : void
```

On `HtmlPage`:

```chemical
public func attach_shared(&mut self, s : *mut SharedAssets)
public func begin_local_js(&mut self)
public func end_local_js(&mut self)
public func begin_local_css(&mut self)
public func end_local_css(&mut self)

// Page-specific remainder.
public func local_js(&self)  : std::string   // dispatches + pageJsEnd
public func local_css(&self) : std::string
```

Existing accessors (`getHtml`, `getHead`, `getHeadJs`, …) remain available so a
user can assemble their own responses. `toString()` is unchanged and still used
for pages that never attach a shared sink.

The user is expected to hash and serve `shared.js()` / `shared.css()` however
they like; `write_to()` is a convenience, not required.

---

## 7. Example

```chemical
var shared = shared_assets()
var home = HtmlPage()
home.attach_shared(shared)
BuildHome(&mut home)

var about = HtmlPage()
about.attach_shared(shared)
BuildAbout(&mut about)

// all pages rendered ...

var js  = shared.js()          // serve this, cached
var css = shared.css()
var h   = home.getHtml()       // page-specific
var hj  = home.local_js()      // home's dispatch + tail
```

Serving the bundle with the one optional helper:

```chemical
shared.write_to("output", "app")   // output/app.js + output/app.css
```

---

## 8. Performance: zero impact on the unattached path

- **No extra render pass.** The user renders the pages anyway; the shared sink is
  filled as a side effect. Nothing is rendered to warm it.
- **All routing is behind one pointer check.** `append_js*`, `append_css*`,
  `require_component`, `set_component_hash`, the CSS dedup helpers, and
  `defaultUniversalSetup` test `shared != null`. A page that never calls
  `attach_shared()` takes exactly today's code path, with no extra allocation or
  copy.
- **`begin_local_js` / `end_local_js` are counter increments** (and can be
  no-ops when `shared == null`); the dispatch emission gains two calls.
- **`defaultUniversalSetup` keeps its current behaviour when unattached.** When
  attached, only the page-independent half is routed to the shared sink; the
  per-page `$__universal_flush()` still goes to `pageJsEnd`.
- **`toString()` is untouched**, so request-time pages keep inlining exactly what
  they inline today.

The dynamic-path regression suite must produce byte-identical output before and
after (§11).

---

## 9. Invariants

- **INV-1.** The shared JS contains no dispatch statements and no page-specific
  HTML.
- **INV-2.** Attached pages share one sink instance; definitions and classes are
  de-duplicated across the whole set.
- **INV-3.** The shared sink is complete before the user takes the buffers out;
  once no page is attached it is not mutated.
- **INV-4.** A page that never calls `attach_shared` behaves byte-for-byte as
  today.
- **INV-5.** Shared JS serializes before page-local JS, so definitions precede
  dispatch execution.
- **INV-6.** `pageHtml` and dispatch statements are always page-local.

---

## 10. Correctness and misuse

| Failure mode | Guard |
|---|---|
| Reading the shared buffers before all pages rendered | documented order "render all, then read"; optional `freeze()` that makes later attaches a diagnostic |
| Page-specific JS leaking into the shared sink | dispatches are explicitly `begin_local_js`; the same escape hatch is available to user `#js` |
| Attached page serialized with `toString()` (missing shared bytes) | `local_js()` / `local_css()` / `write_page()` are the attached-page accessors; `toString()` is documented as the unattached path |
| A page holds a dangling sink pointer | `shared_assets()` returns a heap pointer the user owns for the page set's lifetime |
| Duplicate runtime when `defaultUniversalSetup` is called twice | `runtime_emitted` latch on the sink |

---

## 11. Testing

- **Unattached path unchanged:** render a representative request-time page and
  assert identical `toString()` bytes before/after; assert no new allocations on
  the unattached path.
- **Lean pages:** an attached page's `local_css()` is empty (when no local
  styles) and its `local_js()` contains no `function ` definitions.
- **Bundle has no dispatches:** `shared.js()` does not contain
  `window.$__uni_dispatch(` emitted for call sites.
- **De-duplication:** three pages using the same `Header`/`Footer` emit those
  definitions exactly once in the shared JS and once in the shared CSS.
- **Correct hydration:** a page set served with the shared bundle hydrates every
  page (extend the existing WebView/E2E suites).
- **Shared JS is valid standalone:** the bundle defines everything a page's
  `local_js()` references (checked by loading it before the page's script).

---

## 12. Implementation status

**Phase 0 — page buffer split (`toStringJsOnly()` with no dispatches).**
- [ ] Separate page-independent and page-local JS and CSS regions in `HtmlPage`.
      *Not implemented*: `toStringJsOnly()` still returns `pageJs`, so a lone
      unattached page's `toStringJsOnly()` contains its dispatch. The shipped
      feature exposes the dispatch-free artifact as `shared.js()` instead (see
      Phase 1).

**Phase 1 — shared sink.** *Implemented.*
- [x] `SharedAssets` + `shared_assets()`; `HtmlPage.shared` pointer.
- [x] Route page-independent appends (runtime, definitions, registry, CSS) to the
      sink; keep dispatches and `pageJsEnd` on the page.
- [x] Sink-backed `require_component` / `set_component_hash` / CSS dedup helpers.
- [x] `begin_local_js` / `end_local_js` around dispatch emission
      (`html_cbi` `emit_universal_queue`).
- [x] Hoisting bookkeeping moved **inside** the `require_component` guard
      (`universal_cbi/src/react/ast_replace.ch`), so a component already in the
      sink costs one lookup and nothing else.

**Phase 2 — taking the output out.** *Implemented.*
- [x] `js()` / `css()` / `js_size()` / `css_size()`.
- [x] `local_js()` / `local_css()`.
- [x] `write_to(dir, base_name)` (no content hash; callers can hash and serve the
      buffers themselves).
- [x] `defaultUniversalSetup` split (shared runtime once + per-page flush).

**Phase 3 — `#globalcss`/`#globaljs` + page-local `#css`/`#js`.** *Implemented.*
- [x] `#css` / `#js` always page-local (`begin_local_css` / `begin_local_js`).
- [x] `#globalcss { }` macro (css_cbi) + `CSSOM.shared`; sink-first, page fallback.
- [x] Component `style { }` sets `CSSOM.shared = true` (sink-first).
- [x] `#globaljs { }` macro (js_cbi) + `JsRoot.shared`; sink-first, page fallback,
      once per source location (`require_js_hash` / `set_js_hash`).

**Phase 4 — polish.** *Not implemented.*
- [ ] Optional `freeze()` and diagnostics. The probe
      (`lang/compiled/shared_probe`) asserts the intended contract — after a warm
      pass, request renders must not grow the sink — and demonstrates the ways it
      can still grow: an **incomplete warm set**, and a component `style { }` /
      `#globalcss` / `#globaljs` block first emitted at request time.
- [ ] Content-hash file names; ETag/precompression helpers.
- [ ] `toStringJsOnly()` page-level split (Phase 0), if still wanted.

---

## 13. Open questions

1. **Buffer access shape.** Expose `std::string_view` (`js()` / `css()`) or the
   raw fields? Views keep the sink immutable to callers. *Current: views.*
2. **Hashing location.** `write_to` does not hash; callers can hash and serve the
   buffers. Should a hashing variant be added?
3. **`toStringJsOnly()` semantics.** Keep it as-is (Phase 0 page-level split) or
   leave it and direct callers to `shared.js()`? *Current: unchanged; the shared
   sink is the dispatch-free artifact.*
4. **`#js`/`#css` routing.** *Resolved:* `#js` and `#css` are always page-local
   (bracketed with `begin_local_js`/`begin_local_css`); `#globalcss { }`,
   `#globaljs { }` and component `style { }` are the shared routes (sink-first,
   page fallback).
5. **Multiple sinks.** One sink per page; multiple sinks would need a rule for
   where new definitions go. Left out until needed.
6. **Frozen-sink enforcement.** The intended contract is "warm once, then sink
   size never changes". Two gaps remain: an incomplete warm set silently grows the
   sink on first request, and a component `style { }` / `#globalcss` / `#globaljs`
   block first rendered at request time grows it. Options: an explicit `freeze()`
   that makes later appends a diagnostic, or a dev-only completeness assertion at
   startup. The probe (`lang/compiled/shared_probe`) pins the *desired* invariant
   and demonstrates both failure modes.
