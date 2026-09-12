# Universal Runtime Professionalization Plan

**Status:** Proposed architecture and implementation roadmap

**Progress:** Phase 1 foundational runtime contracts implemented (see §8).

**Scope:** `lang/libs/page`, `lang/libs/universal`, `lang/libs/universal_cbi`,
`lang/libs/universal_parser`, `lang/libs/js_cbi`, `lang/libs/js_parser`, and the
universal component test suites.

This plan is intentionally broader than a bug list. The current problems are
coupled: server rendering, client code generation, hydration, and the JavaScript
front end each have their own partial model of the same component. The goal is a
single, testable contract with a production-quality output model and a small,
specialized runtime. The runtime must retain the complete supported Universal
feature set; reduction means removing unreachable implementation, duplication,
legacy paths, and accidental API surface, not removing features.

The existing `lang/docs/universal-cbi-improvements.md` contains useful detailed
findings and individual fixes. This document is the architectural sequencing
plan for the three concerns that need to be solved together:

1. HTML currently being copied into the JavaScript bundle.
2. Bugs caused by duplicated and implicit behavior, especially in `page` and
   `universal_cbi`.
3. Two JavaScript parser implementations that have already diverged.

---

## 1. Assessment

### 1.1 What is good

The current system has the foundations of a viable SSR framework:

- `HtmlPage` separates head, HTML, CSS, and JavaScript output.
- Universal components have server functions and client component functions.
- Hydration can adopt existing DOM nodes instead of always replacing them.
- Signals, computed signals, effects, portals, refs, error boundaries, and
  hydration mismatch warnings exist in the runtime.
- `universal_parser` is already used by the runtime `universal` package, so the
  parser is not limited to compiler-time use.
- There are compiler-plugin regression tests and a browser E2E direction.

This is a promising prototype-to-framework transition point. It is not yet a
professional framework boundary because the contracts are implicit and several
correctness decisions are made by string manipulation.

### 1.2 Main architectural weaknesses

#### HTML is transported through JavaScript

`universal_cbi/src/converter/converter_jsx.ch` renders a component on the server,
captures the resulting HTML with `capture_html_delta_to_js`, and emits a template
literal passed to `$_uc_h(html, name, props)`. The runtime then assigns that HTML
to a temporary container with `innerHTML` before dispatching hydration.

This causes:

- duplicated bytes: the same markup exists in the HTML response and JS;
- duplicated parsing: the browser parses the markup once as HTML and again as a
  JavaScript string/template;
- difficult escaping rules for backticks, `${`, backslashes, control bytes,
  quotes, and `</script>`;
- a transport-level dependency between `page`, `html_cbi`, and `universal_cbi`;
- an awkward distinction between root hydration and child hydration;
- poor CSP and caching characteristics when page-specific markup is embedded in
  executable code.

The current escaping in `capture_html_delta_to_js` is defensive but is treating
the symptom. The professional design should not send SSR HTML through JS at all.

#### Runtime behavior is an untestable embedded string

`HtmlPage::defaultUniversalSetup()` appends a large JavaScript program directly
from a Chemical string literal. This makes normal JavaScript tooling, linting,
coverage, unit testing, source maps, and browser debugging difficult. Runtime
state, hydration, effects, event handling, portals, and serialization are all
coupled to one page method.

The current size is also partly self-inflicted. `page.ch` contains public-looking
globals, compatibility helpers, error helpers, portal helpers, inner-HTML
handling, context handling, hook shims, and DOM operations in one monolithic
bundle. The important question is not whether a function exists in `page.ch`;
it is whether generated code can reach it. Today that reachability is not
declared or checked.

#### SSR and hydration do not share one render model

The CBI converter emits server calls, client source, reactive wrappers, and
hydration boundaries in separate paths. SSR expression handling and client
expression handling have different supported subsets. An expression can
therefore produce valid client output while producing empty or incorrect SSR
output. This is the root class behind empty initial content, `null` attributes,
and hydration mismatch bugs.

#### The runtime has hidden global and positional assumptions

The runtime uses global names such as `$__uni_dispatch`, `$_ur`, and
`$__uni_current_instance`. Hydration is largely positional. Component discovery
and JS ordering are repaired with `pageJs` string range movement. These choices
make nested components, reordered lists, multiple pages, and failures during
hydration harder to reason about.

#### The parser split has already become semantic drift

`js_parser` and `universal_parser` both define `JsTokenType`, lexer behavior,
parser logic, and related AST concepts. The universal version adds JSX/state and
new operators, while the JS version has a different enum ordering and feature
set. `universal` also has a separate source re-emitter in
`lang/libs/universal/src/converter.ch`.

This is not merely duplicated code. It creates incompatible AST/token contracts
and means a syntax fix can work in `#universal` but fail in `#js`, or vice versa.

### 1.3 Professionalism verdict

The system is a capable experimental SSR/hydration implementation, not yet a
professional production framework. The most important gaps are correctness
contracts, output duplication, safe serialization, parser ownership, and tests
that execute the generated result. Adding more components before resolving those
gaps will increase the bug surface faster than it increases capability.

---

## 2. Target architecture

### 2.1 Server HTML remains the source of truth

The target output is:

```text
HTML response:        SSR markup + compact hydration metadata
External JS assets:   shared runtime + component definitions
Inline page JS:       instance mount records only, if required
```

No SSR HTML should be embedded in JavaScript. A component instance should be
represented by a stable hydration boundary in the HTML, for example:

```html
<!--$uni:component=components_Button;id=42-->
<button data-uni-id="42">Save</button>
<!--/$uni:component=42-->
```

The exact marker format is an implementation choice, but it must be:

- valid in element, text, table, and fragment contexts;
- deterministic within a page;
- safe to emit without user data;
- removable or inert after hydration;
- sufficient to identify the component and its serialized props;
- compatible with CSP and external JavaScript.

Prefer a single compact `data-uni`/comment protocol over large inline JSON
attributes. Props should be serialized into a page manifest or inert JSON script
block, not executable JavaScript. The runtime finds the existing boundary and
hydrates it in place.

### 2.2 Separate runtime, component definitions, and instance data

`HtmlPage` should collect typed output sections rather than treating all JS as a
single mutable string:

- `head`: metadata and explicitly requested head assets;
- `html`: server markup;
- `css`: deduplicated styles;
- `runtimeAssets`: shared universal runtime reference;
- `componentAssets`: deduplicated client component definitions;
- `hydrationManifest`: component IDs, names, and serialized props;
- `inlineBootstrap`: a small bootstrap that loads/starts hydration.

The first migration may serialize these sections back into the current API, but
the internal representation must stop requiring `move_js_range` and HTML-to-JS
capture.

### 2.3 One component intermediate representation

`universal_cbi` should produce a component IR with explicit artifacts:

- server render operations;
- client component function/closure;
- reactive bindings and dependency reads;
- event/property bindings;
- hydration boundary metadata;
- source locations and diagnostics.

SSR output, client output, and hydration metadata should be generated from this
IR. This does not require building a full JavaScript compiler immediately. It
does require that all three outputs come from the same resolved node model and
that unsupported constructs produce diagnostics instead of being silently
dropped.

### 2.4 One parser family with JSX as a mode

Create one owned package, preferably `js_parser` renamed or promoted to a shared
JavaScript syntax package. It should provide:

- one token enum and lexer;
- one AST hierarchy;
- one expression/statement parser;
- one source-location and diagnostic model;
- one printer/emitter interface;
- feature flags or parser modes for JSX, Chemical interpolation, and the
  restricted `#js` surface.

Recommended modes:

- `JavaScript`: ordinary `#js` source;
- `JavaScriptWithJsx`: universal component source;
- `JavaScriptWithJsxAndChemicalEscapes`: compiler macro input where `${...}`
  escapes into Chemical expressions;
- `JavaScriptFragment`: runtime parsing of a fragment or expression.

JSX should be an extension in the lexer/parser state machine, not a second
parser. `universal_parser` should become a compatibility facade during
migration, then be deleted or reduced to mode configuration. `universal` should
use the shared AST printer/emitter rather than maintaining a third handwritten
re-emitter.

The enum must have one authoritative definition. Do not copy token enums into
bindings. If a CBI-facing enum remains, add an automated synchronization check.

### 2.5 Runtime ABI and size policy

The generated client program is the only supported caller of the Universal
runtime. Component authors may use the documented component API, but they must
not call runtime globals directly. `universal_cbi` therefore owns a small,
versioned runtime ABI.

The ABI must be generated from the converter, not maintained by manually reading
`page.ch`:

1. Every CBI emission site registers the runtime symbol it emits, including
   symbols emitted indirectly by JSX, hooks, portals, refs, context, and
   hydration.
2. The CBI produces a `RuntimeRequirements` manifest for each compiled page.
3. The page linker selects the runtime modules required by that manifest and
   rejects an undeclared runtime symbol.
4. A production build fails if a runtime module is unreachable, duplicated, or
   emitted more than once.

This preserves all supported features while allowing the implementation to be
split into small internal modules:

- `core`: value unwrapping, lifecycle context, error reporting;
- `signals`: state and computed subscriptions;
- `render`: vnode creation, fragments, text, and DOM creation;
- `hydrate`: boundary lookup and DOM adoption;
- `props`: property, attribute, event, class, style, and ref application;
- `effects`: effect registration, dependency comparison, cleanup;
- `portal`: portal creation, movement, floating placement, and inert modal
  behavior;
- `context`: named context/provider behavior;
- `reconcile`: keyed and unkeyed child updates;
- `bootstrap`: manifest loading and mount scheduling.

The final bundle may still include all modules for a page using all features,
but the code should have one implementation per behavior and no unused public
aliases. The runtime ABI should expose only the symbols generated by
`universal_cbi`; internal functions should be lexical/module-local where the
bundling format permits it.

#### Runtime reduction rules

- Delete dormant hydration designs and code paths that are not selected by the
  current compiler. Do not keep a second template hydration engine “for later”.
- Do not ship compatibility aliases after the migration window. Compatibility
  belongs in a separately opt-in legacy asset.
- Do not emit both a generic and specialized implementation for the same DOM
  operation. Centralize property/event/style semantics in `props`.
- Do not expose helpers that are only called by hand-written component code.
  Replace raw runtime calls with CBI-generated operations or a typed component
  API.
- Use one error path, one value-unwrapping path, one subscription path, and one
  mount scheduler.
- Keep development diagnostics in a development runtime or compile-time output;
  do not include verbose warnings, protocol assertions, and source metadata in
  the production runtime unless explicitly enabled.
- Minify only after structural validation. Minification is not a substitute for
  eliminating dead code or duplicate behavior.

#### Size and performance gates

The plan must establish measured budgets before implementation. The exact values
should be chosen from a baseline build, but the CI gates must include:

- production runtime bytes, minified and compressed;
- per-component definition bytes;
- per-instance hydration metadata bytes;
- number of DOM nodes visited during hydration;
- first hydration duration for small, medium, and large fixtures;
- state update duration and number of DOM writes;
- mount queue latency and memory retained after unmount.

Budgets are regression gates, not aspirational documentation. A feature that
increases the runtime must identify the required ABI symbols and include a
measured justification.

### 2.6 Professional generated JavaScript

The output should look like a deliberate compiler product, not a trace of the
converter's internal string operations. The target shape is:

```js
// shared asset
const { mount, state, ... } = UniversalRuntime;

// generated component asset
const Component = (props, ctx) => { /* generated body */ };
export { Component };

// page bootstrap/manifest
UniversalRuntime.hydrate(manifest, { Component });
```

The exact module format depends on the deployment target, but generated output
must have these properties:

- deterministic component and instance ordering;
- stable source-oriented names in development and compact names only in a
  validated production build;
- no repeated `window.` lookup for every operation;
- no repeated runtime implementation per component;
- no SSR HTML template literals;
- no debug comments, diagnostics, or source metadata in production unless
  requested;
- no generated code that relies on accidental global declaration order;
- one bootstrap call and one manifest registration per page;
- component definitions emitted once per page or shared asset;
- valid syntax before minification and after minification;
- source locations preserved through a source-map or generated-location table.

The compiler should emit direct operations for statically known elements and
properties where possible, while retaining the generic runtime path for dynamic
elements, spreads, portals, refs, and keyed lists. This is the same principle
used by mature UI compilers: preserve the expressive feature surface, but avoid
paying the fully generic runtime cost for every static node.

Do not optimize by changing semantics. For every specialized operation, compare
its result with the generic reference implementation in the compiler-plugin
tests before enabling it by default.

---

## 3. Implementation phases

### Phase 0: Freeze the contract and establish baselines

**Deliverables**

- Document the current generated-output contract and supported hook/syntax
  surface.
- Add golden fixtures containing SSR HTML, hydration metadata, client output,
  and final DOM output.
- Record current failures from `lang/compiled/universal_failures.md` and the
  universal compiler-plugin tests as named regression IDs.
- Add a generated-output validator that parses every emitted JS artifact before
  it is accepted.
- Add a diagnostic for every AST node that the converter cannot emit. No
  `default => {}` path may silently discard executable statements.

**Exit criteria**

- The baseline suite reports known failures explicitly.
- Invalid or partially emitted JavaScript is a compile failure, not a browser
  failure.
- Every later phase can compare SSR, generated client code, and hydrated DOM.

### Phase 1: Define, extract, and reduce the runtime

**Deliverables**

- Inventory every runtime symbol currently emitted by `universal_cbi`, including
  symbols emitted by `converter_core`, `converter_jsx`, hook conversion, portal
  conversion, and hydration bootstrap.
- Mark dormant paths such as the unused template-builder hydration design and
  remove them or explicitly move them to a separate experimental target.
- Move the runtime from `page.ch` into a real modular `.js` source asset.
- Introduce the generated `RuntimeRequirements` manifest and fail on undeclared
  or unresolved runtime symbols.
- Keep every currently supported feature, but implement each behavior once and
  eliminate aliases, duplicate prop paths, duplicate mount paths, and debug-only
  code from the production asset.
- Replace the single current-instance global with an explicit mount context or a
  stack-safe context API. Preserve hook/effect semantics while removing hidden
  mutable ownership.
- Define and enforce the supported hook surface. Implement every emitted hook or
  reject it at CBI compile time; never emit a call to an optional function that
  may not exist.
- Add runtime versioning and development-only protocol assertions.
- Emit an external, cacheable runtime/component asset in production, with an
  inline mode retained only for development or explicitly requested deployment
  targets.

**Exit criteria**

- Every runtime symbol is either reachable from generated CBI output or absent
  from the production bundle.
- Runtime source can be linted, parsed, profiled, and debugged as JavaScript.
- The full supported feature matrix passes the `lang/tests` compiler-plugin
  suite.
- A component error cannot abort unrelated queued mounts.
- The production runtime meets the agreed byte, hydration, update, and memory
  budgets.

### Phase 2: Remove HTML from the JavaScript bundle

Implement this before optimizing the component library.

**Deliverables**

- Replace `capture_html_delta_to_js()` and `$_uc_h(html, ...)` as the normal
  hydration path.
- Emit stable component boundary markers and an instance manifest during SSR.
- Make the client locate and hydrate existing DOM ranges.
- Define behavior for fragments, text roots, nested components, portals, tables,
  and components whose SSR output is empty.
- Keep a temporary compatibility mode for old generated pages, guarded by an
  explicit version marker. Remove it after all compiled examples migrate.
- Use inert JSON or a safe data channel for props; centralize HTML and JS
  escaping in serializers with hostile-input tests.
- Emit one page bootstrap and a manifest instead of one dispatch snippet per
  server-rendered component instance where batching is possible.

**Important design rule**

If a value cannot be safely and deterministically represented in SSR metadata,
do not emit a partial value such as `null` or an unescaped string. Emit a
diagnostic or defer the binding with an explicit marker.

**Exit criteria**

- No generated `pageJs` contains SSR component HTML.
- Browser network/output inspection shows markup is present once, in HTML.
- Hydration does not assign `innerHTML` merely to reconstruct SSR output.
- CSP-compatible external runtime/component assets work.

### Phase 3: Unify SSR and client conversion around the component IR

**Deliverables**

- Introduce one SSR evaluator for attributes, text, children, and conditions.
- Introduce one attribute renderer parameterized by output target and escaping
  policy.
- Introduce explicit conversion contexts instead of mutable flags such as
  attribute mode and reactive-dereference mode.
- Generate reactive bindings from dependency analysis, not ad-hoc checks of
  expression text.
- Make spreads, event handlers, style, boolean properties, and children obey
  documented SSR/client rules.
- Add keyed list support and define reconciliation semantics before adding more
  data-heavy components.
- Replace fixed-size attribute scratch arrays with bounds-checked growable
  storage.
- Make the IR the source of the `RuntimeRequirements` manifest so runtime calls
  cannot be introduced by one converter path without being accounted for.
- Add a static/dynamic lowering decision: static nodes use compact direct
  creation/patch operations; dynamic nodes use the shared runtime primitives.
- Make generated code deterministic so byte diffs identify semantic changes,
  not hash-map or traversal-order noise.

**Exit criteria**

- The same fixture has equivalent initial SSR and post-hydration DOM.
- State-derived text and attributes either SSR correctly or are intentionally
  marked as client-only.
- Reordering a keyed list preserves node identity, focus, and input values.

### Phase 4: Consolidate the JavaScript parser

**Deliverables**

- Select `js_parser` as the migration base, or rename it to a neutral shared
  package such as `js_syntax`.
- Port JSX, `state`, Chemical escape handling, and universal-specific syntax as
  parser modes/features.
- Make `js_cbi`, `universal_cbi`, `universal`, and both IDE integrations consume
  the same token and AST packages.
- Replace `universal/src/converter.ch` with the shared printer/emitter.
- Add differential tests that parse the same source in each permitted mode and
  compare AST shape and diagnostics.
- Keep one token enum and add a CI check preventing a second `JsTokenType`.

**Migration order**

1. Make shared AST/token APIs source-compatible with both current users.
2. Port the universal parser tests to the shared parser.
3. Switch `js_cbi` to the shared implementation.
4. Switch `universal_cbi` and runtime `universal` to parser modes.
5. Switch IDE features to the shared AST.
6. Delete duplicate parser/lexer/emitter files and remove compatibility aliases.

**Exit criteria**

- A grammar fix is made in one implementation.
- `#js`, `#universal`, runtime `universal`, and IDE parsing agree on tokens,
  precedence, locations, and errors.
- JSX support remains available without retaining a second JavaScript parser.

### Phase 5: Production output and operational quality

**Deliverables**

- External shared runtime asset with content hashing and cache headers.
- Deduplicated component definitions shared across pages.
- Optional compression/minification performed after structural validation.
- Production code splitting for shared runtime, component definitions, and page
  manifests.
- A readable development output mode and a minified production output mode that
  are generated from the same validated IR.
- Source maps or stable generated-source locations for component errors.
- Development diagnostics for SSR/hydration mismatch, duplicate IDs, missing
  component definitions, unsupported props, and protocol version mismatch.
- Production diagnostics that are observable without exposing user data.
- Release-size and runtime-performance reports attached to the compiler-plugin
  test run.

**Exit criteria**

- Multi-page applications do not duplicate the runtime per page.
- Generated output is valid under CSP and can be inspected with normal browser
  tooling.
- A production hydration failure identifies the component and source location.

---

## 4. Testing strategy

The test strategy is centered in `lang/tests`, especially
`lang/tests/compiler_plugins/universal`. Tests must exercise the complete
artifact, not only converter strings. The runtime is not a separately supported
application API, so its behavior is tested through generated Universal output
and through focused compiler-plugin fixtures that compile and execute the
generated page.

### Parser and AST tests

- Token and AST conformance for common JavaScript syntax.
- JSX elements, fragments, spreads, namespaced/member tags, and nested braces.
- Chemical interpolation and template literals, including ambiguous backticks.
- Error locations and recovery behavior.
- Same-input differential tests across parser modes.
- Token/AST enum synchronization and a test that rejects a second authoritative
  JavaScript token definition.

### CBI compile tests

- Every supported statement and expression emits valid JavaScript.
- Unsupported syntax produces a diagnostic.
- SSR/client expression parity.
- Props, spreads, escaping, event handlers, style, children, refs, and keys.
- Component boundary and manifest generation without HTML in JS.
- Runtime requirement manifest: every generated symbol resolves, and unused
  runtime modules are not emitted.

### Runtime behavior tests in `lang/tests`

Add focused fixtures under
`lang/tests/compiler_plugins/universal/src/` for each runtime capability. The
fixtures should compile the component, execute the generated page, and assert
the resulting DOM/state. If a helper is difficult to reach, that is evidence it
should be made an internal runtime helper rather than a public ABI symbol. Cover:

- mount queue ordering and missing definitions;
- state subscriber mutation during notification;
- nested mounts and effect ownership;
- component, event, and effect error isolation;
- hydration adoption, mismatch recovery, and empty roots;
- keyed insert/delete/move;
- portal movement and modal inertness;
- property semantics for `checked`, `value`, `selected`, `class`, and `style`;
- hostile strings and safe prop transport.

Organize the fixtures by contract rather than by implementation file:

- `runtime_signals.ch`: state, computed values, dependency tracking, disposal;
- `runtime_mounting.ch`: nested components, queue ordering, empty roots;
- `runtime_props.ch`: boolean/value/property/style/class/event semantics;
- `runtime_hydration.ch`: SSR adoption, mismatch recovery, fragments, tables;
- `runtime_lists.ch`: keyed insert/delete/move and focus preservation;
- `runtime_portals.ch`: portal movement, floating placement, modal inertness;
- `runtime_errors.ch`: render/effect/event failures and recovery;
- `runtime_output.ch`: no HTML in JS, valid JS, manifest and size budgets;
- `runtime_leaks.ch`: unmount cleanup, listener disposal, subscriber disposal.

Each regression test should state the layer that failed: parser, CBI conversion,
SSR, manifest, runtime, or component source. This prevents “fixes” in component
code from masking a runtime or converter defect.

### Browser E2E tests

Use the existing components E2E direction for real SSR-to-hydration behavior.
Every interactive component should have at least:

- an SSR assertion before JavaScript runs;
- a hydration assertion after JavaScript runs;
- a one-interaction assertion;
- a repeated interaction assertion;
- a reorder/unmount assertion where applicable;
- a console-error and uncaught-rejection assertion.

### Output budgets and invariants

CI should fail when:

- SSR HTML is present inside generated component JS;
- the same universal runtime is emitted more than once in a production bundle;
- generated JavaScript cannot be parsed;
- hydration changes static SSR unnecessarily;
- unsafe raw dynamic HTML or attribute values bypass the approved serializer;
- a generated component contains an unhandled AST construct.
- a generated runtime symbol is not present in the requirements manifest;
- the production runtime exceeds its byte budget without an approved baseline
  update;
- hydration performs more DOM writes or visits more nodes than the fixture
  budget permits;
- unmount leaves event listeners, subscriptions, effects, or portal containers
  reachable.

### Required test matrix

Every supported feature should be tested across these dimensions where
applicable:

| Dimension | Required cases |
|---|---|
| Rendering | static SSR, dynamic SSR, client-only value, empty output |
| Structure | element, fragment, text root, nested component, portal |
| State | initial value, one update, repeated updates, disposal |
| Props | literal, state-derived, spread, missing, invalid, hostile string |
| Events | one handler, replacement, removal, throwing handler |
| Lists | append, delete, insert, reorder, duplicate/missing key |
| Failures | parser diagnostic, CBI diagnostic, SSR mismatch, runtime exception |
| Output | readable development output, minified production output, CSP mode |

The compiler-plugin test suite should run this matrix against both the TCC and
LLVM paths when the generated page can be built by both. Browser-only behavior
should additionally run through the existing browser E2E setup, but the
authoritative regression fixtures remain in `lang/tests`.

---

## 5. Ownership boundaries after the migration

| Concern | Owner | Must not own |
|---|---|---|
| HTML document assembly, assets, manifest | `page` | JSX parsing or component semantics |
| Signals, hydration, DOM operations | external/shared page runtime | SSR conversion or Chemical AST resolution |
| JS/JSX syntax and AST | shared JS syntax package | page-specific output policy |
| Chemical-to-component conversion | `universal_cbi` | a private parser or private runtime |
| Runtime source parsing/printer API | `universal` | a second AST/printer implementation |
| Generic HTML macro | `html_cbi` | universal hydration internals |
| Component APIs | `components` | workarounds that inject raw page JS |

Raw `page.pageJs.append_*` calls from components should be treated as a design
smell. New behavior should use a runtime capability or a typed component
binding, so the page runtime remains the only owner of the browser protocol.

---

## 6. Priorities and sequencing

The recommended order is:

1. Baseline validation and hard diagnostics.
2. Extract/test the runtime.
3. Remove SSR HTML from JS and introduce the manifest/boundary protocol.
4. Unify SSR/client conversion through a component IR.
5. Consolidate the parser and delete duplicate implementations.
6. Add keyed reconciliation, shared assets, performance work, and component
   library expansion.

Do not begin with a large rewrite of every component. First migrate one small,
one nested, one stateful, one portal, and one list component through the new
protocol. Keep old and new protocols selectable during that period and compare
their SSR HTML, hydrated DOM, events, and console output. Then migrate the
remaining component library mechanically.

## 7. Definition of professional readiness

Universal is ready for serious application use when all of the following are
true:

- SSR markup is sent once as HTML and is hydrated in place.
- The runtime is a normal, tested JavaScript asset rather than an opaque string.
- `page`, `universal_cbi`, and the runtime share a versioned protocol.
- SSR and client output are produced from one component model.
- Unsupported syntax and props fail during compilation with source locations.
- There is exactly one JavaScript/JSX parser implementation.
- Reordered lists, portals, nested components, and errors have defined behavior.
- Generated JavaScript is parsed and structurally validated in CI.
- Browser tests cover SSR, hydration, interaction, cleanup, and console errors.
- Production output supports CSP, caching, source diagnostics, and measurable
  bundle budgets.

Until these invariants hold, adding more hooks or visual components should be
considered feature work on top of an unstable platform, not framework maturity.

---

## 8. Progress log

### Phase 1 (partial) — runtime correctness and ownership

Implemented in `lang/libs/page/src/page.ch`:

- **Correct effect dependency comparison.** `$__uni_run_effects` now resolves
  the deps array through `$__uni_value` before comparing against `lastDeps`
  (previously it compared raw signal objects against resolved primitives, so
  `changed` was always true and every effect re-ran on every instance flush).
- **Ownership-driven resource tracking.** State signals (`$_us`) and computeds
  (`$_ucs`) created during a component render register with the owning instance
  via `$__uni_register_resource`. Each exposes `$_dispose`, and
  `$__uni_dispose` releases `inst._resources` so a long-lived signal can no
  longer retain computeds/effects from removed components.
- **Ownership-driven remount.** `$__uni_mount` disposes a previous instance
  tracked on the same host before mounting a replacement.
- **Portal move no longer looks like an unmount.** Hydration-time portal moves
  are recorded in `window.$__uni_moving_nodes` so the cleanup MutationObserver
  does not dispose the freshly-hydrated owner (this bug silently disabled every
  effect in a component, including focus traps and inert backgrounds).
- **Dev diagnostics toggle.** `window.$__uni_dev` (default on, disable with
  `window.$__uni_prod`) bounds hydration warnings and exposes
  `$__uni_dev_assert`.
- **Keyed hydration adoption.** `$__uni_hydrate_node` now adopts the
  SSR-rendered list range in place (recording `__uni_vnode_key` on adopted
  element nodes) instead of discarding and re-rendering it. Both the hydration
  and fresh-render paths share one keyed reconciler, `$__uni_reconcile_list`.
  Regression test: `runtime.spec.ts::keyed list: hydration adopts SSR nodes
  without removing them` (asserts zero SSR list-node removals during hydration).
- **Reconciler-driven disposal.** The reconciler now disposes component
  instances in a removed subtree via `$__uni_dispose_subtree` /
  `$__uni_clear_range` before removing DOM, so teardown is triggered by the
  operation that removes nodes; the MutationObserver remains only as a safety
  net. Contract test: `runtime_contracts.ch::universal_reconciler_driven_disposal`.

- **Runtime unit-test layer.** Added `lang/compiled/components-e2e/tests/runtime-unit.spec.ts`
  (10 tests) exercising runtime primitives directly against a real DOM,
  independent of components: `$_us` subscribe/dedupe/unsubscribe, `$_ucs`
  dependency tracking and `$_dispose`, `$__uni_run_effects` resolved-value dep
  comparison and cleanup ordering, `$__uni_reconcile_list` keyed reorder
  identity/removal, `$__uni_dispose` cleanup ordering, `$__uni_register_resource`
  attribution, and `$__uni_shallow_equal`. This is the plan's Phase 1 "runtime
  can be tested" exit criterion in pragmatic form (the source is still embedded
  in `page.ch`; full extraction remains open).

- **Reactive reads inside callbacks (derived arrays).** `expr_references_reactive_var`
  and `jsx_expr_needs_reactive_wrapper` now descend into arrow-function bodies
  (`universal_cbi/src/converter/converter_utils.ch`), so
  `var filtered = items.filter(it => it.includes(query))` becomes a `$_ucs`
  computed and `{filtered.map(...)}` stays live. Bare function expressions are
  explicitly excluded from JSX-expression wrapping in `convert_jsx_runtime_expr`
  so event handlers/callbacks are emitted as functions, not signals. This
  unlocks filtering/sorting/data-table/search UIs. E2E:
  `runtime.spec.ts::derived list: filtered array recomputes reactively`,
  `::derived list: SSR renders the unfiltered initial value`.
- **SSR renders object-element `.map()` lists.** `emit_ssr_map_children`
  previously skipped object-literal elements (`ssr_js_eval_from_text` cannot
  represent objects), so any keyed list of objects rendered **empty** in the
  server HTML and only appeared after hydration. The unroller now parses object
  elements (`parse_js_object_properties`) and binds the element text, and
  `convert_jsx_ssr_expression` resolves `item.<prop>` reads against it.
  Verified: keyed list SSR is now `<li>Alpha</li><li>Beta</li><li>Gamma</li>`
  before JavaScript runs.
- **SSR assertion suite.** Added
  `lang/compiled/components-e2e/tests/ssr.spec.ts`, which loads the app with
  `javaScriptEnabled: false` and asserts server-rendered content directly. This
  is the plan's "SSR assertion before JavaScript runs" layer; it caught that
  the E2E suite was only ever exercising post-hydration DOM.

- **Props-derived reactive values (with reassignment safety).**
  `expr_references_reactive_var` treats prop reads as reactive for top-level
  locals, so `var visible = props.items.filter(...)` becomes a computed that
  tracks parent updates. Two guards keep this safe: `collect_assigned_names`
  (a block pre-scan) prevents wrapping any local that is reassigned (Stack's
  `out = out + ...` accumulator), and `is_hook_function_name` prevents wrapping
  hook-returning locals (`createContext`, `useRef`, ...). The context-assign
  path only skips reactive deref for value RHS, not function RHS
  (`ctx.write = (v) => { if(disabled) ... }`). E2E:
  `runtime.spec.ts::derived list from props: parent state updates propagate`.
- **SSR resolves computed aliases of SSR locals.** `emit_ssr_map_children`
  now falls back to `find_ssr_local` for a reactive/computed identifier with no
  static state init, so `var items = props.items; {items.map(...)}` renders at
  SSR via the runtime for-loop instead of rendering nothing. Plugin test:
  `to_string.ch::universal_ssr_map_local_var`.

### Phase 2 (first slice) — nested component SSR HTML removed from the JS bundle

Implemented across `lang/libs/universal_cbi/src/converter/converter_jsx.ch` and
`lang/libs/page/src/page.ch`:

- **Nested components emit a function-reference vnode, not an HTML snapshot.**
  When a component body renders another universal component (`<Button .../>`),
  the client function previously emitted
  `(() => { const html = \`<SSR markup>\`; return $_uc_h(html, "Name", props) })()`.
  The same markup already appears in the page HTML, so it was duplicated in the
  response and reparsed via `innerHTML` during hydration. It now emits
  `$_uc_c(ComponentFn, props)` — a `__uni_uc` vnode carrying the component
  function directly. The client locates the server-rendered element and hydrates
  it in place.
- **Runtime adoption.** `$_urn`, `$__uni_hydrate_node`, and the SSR'd-state path
  now handle a `__uni_uc` vnode whose `p.comp` is a function by calling
  `$__uni_mount(dom, comp, props, "root")` (adopt existing DOM) or
  `$__uni_mount(container, comp, props)` (fresh client render) instead of
  dispatching through a name lookup + `innerHTML`. The legacy `$_uc_h` html path
  is retained for backward compatibility.
- **Client-only components still get their client function.** The child's server
  function is still invoked during the client-JS pass because its
  `if(require_component(hash))` guard is what emits the child's client JS; its
  SSR output is now discarded with `truncate_html` rather than captured into JS.
  Without this, a component that only renders inside a runtime conditional
  (e.g. a lazy error-boundary child) would be undefined at hydration.

Measured on the components E2E app: `index.js` shrank from 283,437 to 197,918
bytes (**-30.2%**), `index.html` is byte-identical, and `capture_html_delta`
references in the page JS dropped to zero. `$_uc_h(html, ...)` uses dropped from
341 to zero (remaining occurrences are the runtime definition only).

Tests:

- `to_string.ch::universal_component_child` updated to assert the
  `$_uc_c(ComponentFn, {})` emission.
- `to_string.ch::universal_component_child_does_not_embed_ssr_html` added: asserts
  the page JS contains no `const html =` / `$_uc_h(html`, references the child by
  function, and that the SSR markup is still present in the HTML response.
- Full E2E: 353/353 pass. Compiler-plugin suite: 1093/1095 (the 2 failures are the
  pre-existing, unrelated `css_cbi` and `json_cbi` tests).

Still open in Phase 2: stable component boundary markers + instance manifest
(replacing positional adoption), external/hashed runtime assets, and removing the
now-unused `capture_html_delta_to_js` path and dormant template-builder design.

### Phase 2 (second slice) — multi-node nested component roots hydrate correctly

The runtime previously assumed every nested universal component rendered a single
root element: `$__uni_mount(dom, ..., "root")` hydrated the component and returned
`dom.nextSibling`. That is wrong for a component whose root is a **fragment**
(`<>{a}{b}</>`) or otherwise multi-node — the parent's sibling hydration then
shifted and patched the wrong nodes (observed as duplicate/consumed siblings).

Implemented in `lang/libs/page/src/page.ch`:

- **`$__uni_mount` returns the node after the component's entire SSR range.**
  In `"root"` mode it returns the value from `$__uni_hydrate_node` (the node after
  the hydrated range) instead of the caller assuming one element.
- **Hydration no longer requires the component's first SSR node to be an
  element.** `$__uni_hydrate_node`'s `__uni_uc` branch now calls
  `$__uni_mount(dom, comp, props, "root")` for any non-null `dom` (text nodes
  included) and returns the computed end node, falling back to `dom.nextSibling`.
- **Instance tracking for text-leading roots.** When the component's range starts
  with a text node, `$__uni_mount` tracks the first element inside the range
  (falling back to the parent) instead of attempting to track a text node.

This also **removed a pre-existing hydration duplication** of reactive state
elements (the controlled dialog `<p>` previously appeared twice after hydration;
it is now adopted in place). The stale test workaround
(`components.spec.ts::dialog controlled` used `nth(1)`) was updated to assert the
single, correct node.

Tests: `lang/compiled/components-e2e/tests/root-shapes.spec.ts` (3 tests) —
SSR-before-JS, hydration adoption with sibling alignment, and fresh client mount
after a toggle, all against a fragment-root nested component.

Full E2E: 356/356 pass. Compiler-plugin suite: 1093/1095 (same 2 unrelated
failures).

### Known remaining SSR parity gap

A computed local whose source is a **`.filter()` over runtime props**
(e.g. `var visible = props.items.filter(it => ...)`) is reactive on the client
but renders **empty at SSR**, because the generated server function cannot
evaluate a JavaScript predicate. A **static** source is now handled: see
"Static `.filter()` SSR" below. Closing the runtime-prop case requires
predicate codegen / evaluator unification (plan Phase 3).

- **Static `.filter()` SSR.** `emit_ssr_map_children` and `emit_ssr_array_count`
  resolve a derived local's initializer (`resolve_static_array_elements`) and,
  for `var filtered = items.filter(pred)` over a static state/array-literal
  source, statically evaluate the predicate per element with a conservative
  evaluator (`ssr_filter_predicate`: `includes`/`startsWith`/`endsWith`,
  `==`/`!=`, `!`, case-folding via `toLowerCase`). `append_js_node_text` now
  emits arrow functions so the initializer text is available. E2E:
  `ssr.spec.ts::SSR: derived .filter() list renders before JS`.
  Runtime-prop sources remain client-only (the predicate needs a JS evaluator).

Tests:

- `lang/tests/compiler_plugins/universal/src/runtime_contracts.ch` — the
  formerly bug-pinning tests now assert correct behavior; added
  `universal_effect_deps_compared_by_value`, `universal_layout_effects_are_ever_run`,
  `universal_unmount_cleanup_exists`, `universal_keyed_reconciliation_exists`,
  `universal_reconciler_driven_disposal`.
- `lang/compiled/components-e2e/tests/runtime.spec.ts` — added
  `effect deps: unrelated state change does not re-run effect`,
  `keyed list: hydration adopts SSR nodes without removing them`, and the
  derived-list tests (349 E2E tests pass).

### Still open

Everything else in Phases 0–5: runtime extraction to a real `.js` asset,
`RuntimeRequirements` manifest, removal of SSR HTML from the JS bundle,
single SSR evaluator/IR, parser consolidation, emitted-JS semantic validation,
external/hashed runtime assets, streaming SSR, and the component platform work
(forms, virtualization, dynamic context, i18n, animation, devtools).

