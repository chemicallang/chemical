---
name: universal
description: Diagnose, fix, and implement features in the Chemical universal component pipeline. Use when working on `#universal` components, `universal_cbi`, generated SSR/hydration output, `HtmlPage` runtime behavior, or bugs where compiled HTML/CSS/JS disagree with source components.
---

# Universal

Universal is the SSR + hydration component system: `universal_cbi` (compiler plugin for the
`#universal` macro), `lang/libs/page` (SSR value model + client hydration runtime), and
`lang/libs/components` (shipped component library). Universal components work inside `#html`
blocks (processed by `html_cbi`) and are the only supported component model (React/Preact/Solid
bridges were removed).

> ⚠️ **Verify before applying old fixes.** The pipeline moved fast: statement emission, escaping,
> hooks, error isolation, and SSR children rendering were all rebuilt recently. Many failure modes
> documented in older notes/issues are already fixed — check the regression tests under
> `lang/tests/compiler_plugins/universal/` first.

## Pipeline map (what generates what)

| Stage | Files |
|---|---|
| Lexing (JS + JSX hybrid, Chemical `${}` escapes) | `lang/libs/universal_cbi/src/main.ch` (`getNextToken`), `universal_parser` lib |
| Parsing (statements, expressions, JSX, `state` decls) | `lang/libs/universal_parser/src/*` |
| `#universal Name(props : a, b?)` macro parse | `universal_cbi/src/react/macro.ch` |
| Sym-res (server function creation, required-param checks) | `universal_cbi/src/sym_res/*` |
| JS→native SSR emission (server function body) | `universal_cbi/src/react/ast_replace.ch`, `converter/*` |
| Client JS emission (`function Name(props){...}` + dispatches) | `universal_cbi/src/react/emit_js.ch`, `converter/converter_jsx.ch` |
| SSR attribute/child value model | `lang/libs/page/src/ssr.ch` |
| Hydration runtime (inline JS in `page.ch`) | `lang/libs/page/src/page.ch` → `defaultUniversalSetup()` |
| `#html` ↔ universal integration (boundary span + dispatch) | `lang/libs/html_cbi/src/converter/language/component.ch`, `main.ch` |
| Component library | `lang/libs/components/src/*` |

### The generated server function

Each `#universal Button(props) { ... }` generates a native function (built by `sym_res.ch`):

```chemical
func components_Button(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void
```

Its body (built in `react/ast_replace.ch` → `universal_replacementNode`) does, in order:

1. Guard: `if(page.require_component(hash)) { page.set_component_hash(hash); ... }` where
   `hash = funcNode.getEncodedLocation()` — component JS is emitted **once per page**.
2. Inside the guard: append the client function `function <module_scoped_name>(props) { ... }`
   to `pageJs`, then **hoist** it above previously emitted dispatch lines using
   `page.get_js_pos()` / `page.move_js_range()` / `page.js_hoist_pos` (memmove-based range
   move — component functions must be defined before any `$_uc_h` dispatch that references them).
3. Always: convert the component body with `target = BufferType.HTML` — emits SSR markup into
   `pageHtml` via `page.append_html*` calls (batched through `converter.str` + `put_chain_in()`).

### The generated client runtime

`page.defaultUniversalSetup()` installs (all inside `pageHeadJs`/`pageJs` string literals):

- **Head JS** (`pageHeadJs`): `window.$__uni_hydration_queue`, `window.$__uni_error`,
  `window.$__uni_dispatch(fnName, target, props, mode)`. Dispatch mounts immediately if
  `window[fnName]` exists (wrapped in try/catch), else queues. `pageJsEnd` gets
  `window.$__universal_flush();` which drains the queue (per-item try/catch).
- **pageJs**: signals (`$_us`, `$_ucs`), vnode factory (`$_ur`, `$_um` props merge,
  `$_uc_h`, `$_uc`), full hook surface (`$_r.*`), portal helpers (`$__uni_tag_portal`,
  `$__uni_inert_scan`, `$__uni_floating`), hydration (`$__uni_hydrate_node`,
  `$__uni_hydrate_children`, `$_urn` fresh render), props (`$__uni_set_prop`,
  `$__uni_apply_prop`), mount (`$__uni_mount`), error fallbacks.
- It also appends `[data-chx-i]{display:contents}` to pageCss — hydration boundary spans
  (see below) must not affect layout.

### `#html` integration (html_cbi side)

`html_cbi` renders a universal component usage as:

1. SSR: `<span id="u{id}" data-chx-i>` + call the server function + `</span>` — the
   hydration boundary (inline-safe via `display:contents`; works inside tables).
2. JS: `window.$__uni_dispatch('Module_Name', document.getElementById('u{id}'), {props})`.
   Props are collected from the element's attributes; event attrs are kept **only** if their
   value is a Chemical lambda (`has_non_ssr_attr_value`). The component function referenced by
   the dispatch is emitted into `pageJs` by universal_cbi when the component definition
   renders; until defined, dispatch queues.
3. Styled wrap over a universal component (`signature.hydrateFunctionNode`) dispatches the
   **inner** universal component (it owns the client JS); the styled wrapper is SSR-only.

## Buffers and output files (`HtmlPage`)

| Field | Becomes | In `writeToDirectory` |
|---|---|---|
| `pageHead` | `<head>` content | inline in `{name}.html` |
| `pageHtml` | SSR body markup | inline in `{name}.html` |
| `pageCss` | `<style>` / `.css` | `{name}.css` |
| `pageHeadJs` | head `<script>` / `{name}_head.js` (dispatch + queue) | `{name}_head.js` |
| `pageJs` | body `<script>` / `{name}.js` (runtime + components + dispatches) | `{name}.js` |
| `pageJsEnd` | appended at the very end of pageJs (`$__universal_flush()`) | — |

- `getFinalizedPageJs()` = `pageJs + pageJsEnd`. `toString()` inlines everything;
  `htmlPageToString()` references external assets via `<link>`/`<script src>`.
- Floats/doubles appended via the helpers use precision 3 (`append_double(value, 3)`).
- Dedup maps: `doneClasses`, `doneRandomClasses`, `doneComponents` (`require_*`/`set_*_hash`).

## Client runtime contract (what generated code may call)

Only generated code and the components library use these; component authors should stick to
JSX + hooks. Key globals and their jobs:

- `$_us(initial)` → state signal `{value get/set, subscribe}`. Set notifies a **snapshot** of
  subscribers (mutation-safe) and schedules the instance's effects on a microtask.
- `$_ucs(fn)` → computed signal with automatic dependency tracking: reads inside `fn` are
  captured via `window.$__uni_current_tracker`; the computed resubscribes to its deps on each
  recompute, tracks nested computeds via `window.$__uni_child_tracker`, and disposes stale
  subscriptions (`signal.$_uc_dispose`).
- `$_ur.createElement(t, p, ...c)` → vnode `{t, p, c}`; `$_um(...parts)` merges prop objects
  (class merge with space; keeps state signals unwrapped so reactive bindings survive).
- `$_uc_h(html, name, props)` → `{t:"__uni_uc", p:{html,name,props}}` — a vnode that mounts
  `html` into a container and dispatches hydration for `name`.
- `$_urn(v)` → fresh client render of any vnode/signal/array/HTML blob into real DOM.
  State values render between `<!--s-->`/`<!--e-->` marker comments and re-render on change.
- `$__uni_set_prop(el, key, v)` handles: `class`/`className`, `htmlFor`/`for`, `style`
  (string → cssText, object → per-key, null/false → removed), `on*` events (wrapped in
  try/catch, previous listener replaced), `ref` (stored, assigned later via
  `$__uni_assign_ref`), `dangerouslySetInnerHTML` (with contenteditable guard: pending HTML
  is flushed on blur when the element is actively edited), boolean DOM props, and
  property-set with `setAttribute` fallback.
- `$__uni_hydrate_node(parent, dom, v)` adopts SSR'd DOM in place; mismatches are warned
  **once** (`$__uni_warn_hydration` sets `window.$__uni_hydration_warned`) and self-corrected —
  never fatal.
- `$__uni_mount(host, comp, props, mode)`: `mode` is `"children"` (default, hydrate into host's
  children) or `"root"` (host itself is the SSR'd root element, used when hydrating an
  existing element from a `__uni_uc` state vnode). Renders `comp(props)` with instance
  tracking (`window.$__uni_current_instance`), then runs effects.

### Hooks (`$_r.*`) — full surface, all implemented

`useState`, `useEffect`, `useLayoutEffect`, `useRef`, `useMemo`, `useCallback`, `useReducer`,
`useContext`, `createContext`, `createPortal`, `useErrorBoundary`.

- `useEffect(fn, deps)`: runs after mount and re-runs when a dep **signal's** value changes —
  deps that are state/computed signals are subscribed directly, so parent-controlled props
  (e.g. `isOpen = props.open != null ? props.open : open`) trigger the effect. Dep comparison
  unwraps signals via `$__uni_value`. Cleanup return values are invoked before re-run; both
  cleanup and body are try/catch'd.
- `useErrorBoundary(fallback)`: registers a fallback for the component's own render. A throwing
  render is caught by `$__uni_mount` → `$__uni_render_fallback(inst, props, err)` → the
  component's fallback or `$__uni_default_fallback` (`.chx-error-boundary` styled div).
  **Boundaries are per-component**: each universal component mounts independently, so a
  parent's boundary does not catch a child's render error. Event handlers and effect
  bodies/cleanups are also wrapped and contained.

### Portals (`createPortal`)

`createPortal(children, opts)` returns `{t:"__uni_portal", p: opts, c}`.

- **SSR** renders portal children inline where the portal sits (no body on the server).
- **Hydration**: `$__uni_hydrate_node` hydrates the SSR'd nodes in place, then MOVES the
  `[startDom, cur)` range into a container appended to `document.body` (tagged via
  `$__uni_tag_portal`, `data-uni-portal` + optional `data-uni-modal`).
- **Fresh render** (`$_urn`): creates the body container directly.
- `$__uni_floating(trigger, menu, opts)` anchors a portaled menu with `position:fixed` under
  the trigger's rect, flips above when `spaceBelow < menuHeight + gap` (and there is more
  room above), clamps `maxHeight`, re-measures on scroll (capture) + resize, and returns a
  cleanup suitable as an effect return.
- **Modal inertness**: `$__uni_inert_scan()` marks a modal active only if a visible
  `[data-uni-modal]` container exists (`display !== none && visibility !== hidden`), then sets
  `inert` on every direct child of `<body>` that is not a portal container; non-modal portals
  stay interactive. Call it from a `useEffect([isOpen])` (Dialog/Sheet do this). Note
  `el.inert` only reflects the element's own attribute — use `el.closest("[inert]")` to test
  effective inertness.
- Component rules: never toggle a portaled menu's visibility with an inline reactive `style`
  (a style subscription sets `cssText`, wiping the fixed `top/left`) — use
  `data-open={open ? "true" : "false"}` + CSS `&[data-open="true"] { display: grid; }`.
  Pass `{modal: true}` only for true modals.

### Context (`createContext` / `useContext`)

Component JS functions are emitted only when used, so there are no module-level declarations;
context is a **name-keyed registry** `window.$__uni_ctx` in the runtime:

```chemical
// provider: key must be derivable on both sides from a shared prop
const ctx = createContext("rg-" + (props.name || "default"), "")
ctx.value = value                  // publish: raw signal emitted, runtime wires it
ctx.write = (v) => { value = v; if(props.onValueChange) { props.onValueChange(v) } }

// consumer:
const ctx = useContext("rg-" + (props.__rgName || props.name || "default"))
<input checked={ctx.value == props.value} />   // ctx.* reads are reactive ($_ucs-wrapped)
<button onClick={() => { if(ctx.write) { ctx.write(props.value) } }} />
```

- `createContext(name, default)` is idempotent; assigning a **signal** to `ctx.value` wires the
  entry to follow it; assigning a plain value sets it directly.
- **SSR ordering**: a provider's SSR function runs AFTER its children render (children HTML is
  pre-rendered and passed as the 3rd arg), so a consumer can never observe a published value at
  SSR — `ctx.value` resolves to the static `createContext` default (or `None` for useContext).
  `ctx.value = x` / `ctx.write = fn` are no-ops at SSR. Groups render children
  unpressed/unchecked at SSR; hydration applies selection.
- **Name threading**: groups inject their name into child vnodes
  (`props.__rgName`) before mounting; items spread `{...props}`, so injected keys appear as
  harmless DOM attributes.
- **No provider**: useContext creates an entry with undefined value; comparisons are false,
  `ctx.write` is absent (guarded calls no-op).

## Converter behavior (`universal_cbi`)

### `state` and computed variables

- `state x = <init>` → `const x = $_us(<init>)`, recorded in `state_inits` (name + init text)
  so SSR can evaluate state-derived expressions from the initial value.
- A top-level `var x = <expr referencing state/computed/context>` is wrapped:
  `const x = $_ucs(() => <expr>)` and `x` becomes reactive (`computed_vars`).
- Reads of reactive vars in JS output become `x.value` (assignment targets `x.value = ...`,
  `x++`/`x--`, hook last-args skip deref via `skip_reactive_deref`).
- `props` reads in JSX become `window.$__uni_value(props.x)` (unwraps signals transparently);
  assignments `props.x = expr` emit the raw property access (the getter wrapper is not a valid
  assignment target).

### Reactivity rules (the #1 source of "it doesn't update" bugs)

- Attribute/child expressions that directly read state/props/context are wrapped in
  `$_ucs(() => ...)` by `jsx_expr_needs_reactive_wrapper` — props reads count **only inside JSX
  attributes** (`in_jsx_attribute`); context member reads count everywhere.
- A local variable computed before `return` is evaluated **once at render** — its value is
  frozen into the output:

```chemical
// FROZEN — never re-evaluates:
var activeDesc = ""
if(open && props.options) { activeDesc = "chx-select-opt-" + highlight }
<button aria-activedescendant={activeDesc} />

// REACTIVE — stays live:
<button aria-activedescendant={open && props.options ? "chx-select-opt-" + highlight : ""} />
```

- **Conditional UI must be a JSX conditional child, never control flow.**
  `if(!open) { return null }` and `var overlay = open ? ... : null; return overlay` are
  evaluated once at mount. Only `{cond && <jsx/>}` / `{cond ? a : b}` written inline as a child
  becomes a reactive computed that re-evaluates when its deps change. All state reads must
  happen inside that conditional expression. (This bit `ErrorOverlay` — see
  `lang/libs/components/src/ErrorOverlay.ch` for the working pattern.)
- `#css` style helpers take `page : &mut HtmlPage` and are **server-only**; calling one during
  client hydration throws (no `page` argument) and the component silently renders the error
  fallback. Use inline `style={{...}}` objects or `class` strings inside `#universal` bodies.
- Effects re-run only when a dep in the array is a state/computed whose value changes (or on
  mount). Plain values run once. Do not rely on effects running after every render.

### Client JS emission — supported surface

The JS lexer (`src/main.ch`) and `convertJsNode` (`converter/converter_core.ch`) now emit:
all statement forms (`if/else`, `for`, `for-in`, `for-of`, `while`, `do-while`, `switch/case/
default`, `break`, `continue`, `throw`, `try/catch/finally`, `class`, `import`, `export`,
`yield`, `debugger`), operators (`%`, `%=` `**`, `??`, `?.`, `===`, `!==`, `&&`, `||`, compound
assignments, `++/--`), arrow/function/class declarations with default params, spread,
template literals (opaque; `${` always enters Chemical escape mode — no JS interpolation), and
strings with full escaping (`escapeJs`: quotes, backslash, `\n\r\t`, `${` → `\${`, non-ASCII →
`\u{...}`). Regex literals are **not** lexed — use `new RegExp("...")`.

Regression tests for this live in `lang/tests/compiler_plugins/universal/src/statement_emission.ch`.

### SSR evaluation — what can be server-rendered

The converter has three cooperating SSR engines (attributes, children expressions, bool
conditions). Supported at SSR time:

- Literals (strings/numbers/booleans/`null`/`undefined` → `None`), `!x`, ternaries,
  `&&`/`||` (rendered as runtime `if` statements), `==`/`!=`/`===`/`!==`
  (`ssrTextEquals` / `ssrValuesEqual` — loose JS equality, `None` equals
  `"undefined"`/`"null"`), arithmetic `+ - * /` on state-derived numbers, string
  concatenation (`ssrMakeMultipleValue` of parts).
- State initializers: `state todos = [...]` is folded; `todos.filter(p).length` is unrolled
  with the callback param bound to each element (`ssr_bound_param`).
- **`.map()` children**: static sources (state array literals, inline array literals) are
  unrolled at compile time with `item`/`index` params bound; runtime sources (props arrays,
  locals) emit a Chemical for-loop over `MultipleAttributeValues` (`ssrMultipleGet`).
  Unresolvable elements render nothing (never empty wrappers).
- `.length` / `.size` on arrays render the count as text.
- Component-body statements are emitted into the server function: `var`/`let`/`const`/`state`
  decls become SSR locals (`SsrAttributeValue` vars), `x = expr` assignments, `if/else` chains,
  and **conditional `return <jsx/>` chains** — consecutive `if(c){ return <jsx/> }` plus a
  trailing return are chained as if/else-if/else so exactly one branch renders (matching JS
  `return` semantics; `emit_ssr_return_chain`).
- `props.children` appends the pre-rendered children HTML (`SsrText`).
- `${...}` Chemical embeds and user structs (via the `getSsrAttributeValue` protocol) convert
  through `AttrValueConverter.convert_to_attr_value`.
- **Style objects** (`style={{...}}`) are SSR'd statically only: literal values, camelCase →
  kebab (`borderRadius` → `border-radius`), `--vars` kept. Props/state values are skipped in
  the SSR text (the client re-evaluates reactively) — never leak raw JS into the attribute.
- **Skipped** (by design): event attributes (`on*`), `ref`, `dangerouslySetInnerHTML`, arrow
  functions, Chemical lambdas, unresolvable spreads. Skipped values never degrade into
  `style=""` or `checked="null"` — `SsrAttributeValue.None` renders nothing in HTML and
  `undefined` in JS props.

### Attribute rendering (`lang/libs/page/src/ssr.ch`)

- `SsrAttributeValue` variants: `None`, `Boolean`, `Char`, `UInteger`, `Integer`, `Double
  (precision)`, `Text(SsrText)`, `PtrChar`, `Multiple(MultipleAttributeValues)`,
  `Spread(SsrAttributeList)`, `Callable`.
- `renderHtmlAttrs` / `renderJsAttrs` accumulate into a stack `SpecialAttrs`, merge multiple
  `class` values (space-joined) and `style` values (semicolon-joined), dedupe other attrs
  last-wins, skip `false` booleans and `None`.
- **HTML target**: Text/PtrChar are HTML-escaped (`& < > " '`).
- **JS target**: Text/PtrChar are JS-escaped via `appendJsEscaped` (`"`, `\`, `\n\r\t`,
  control chars, and `</` → `\u003C/` so inline `<script>` can't be broken out of).
  `None` → `undefined` in JS props.
- **Bounds**: `SpecialAttrs` holds at most 32 classes, 32 styles, 64 other attrs — beyond
  that entries are silently dropped (bounds-checked, not overflowed).
- `renderHtmlChildValue`: `None` and `Boolean` render nothing (React child semantics).
- Custom serialization for user structs: define
  `func getSsrAttributeValue(&mut self, page : &mut HtmlPage) : SsrAttributeValue` on the
  struct; `convert_node_attr_value` calls it for `Linked`/`Generic` types.

### Prop serialization rules (still sharp edges)

- Never pass C++ structs with `vector<>` fields as props; pre-serialize to JSON and
  post-process with `js_string_escape` (see AGENTS.md) if the data may contain `'` or `\`.
- Unsupported prop types (e.g. arbitrary pointers) fall through to `UInteger` — a silent
  pointer-as-number. There is no diagnostic yet; avoid such props.
- All escaping goes through `appendHtmlEscaped` / `appendJsEscaped` / `escapeJs` / `escapeHtml`
  in the converter — extend those if a new context appears; don't hand-escape at call sites.

## Debug workflow

Trace issues in this order:

1. Component source (`#universal` in the app/library).
2. Generated output: `lang/compiled/<pkg>/output/*.html`, `*.css`, `*.js` — HTML for initial
   state, JS for reactivity (`$_ucs(() => ...)` wrappers) and the `$__uni_dispatch` props.
3. Converter: `lang/libs/universal_cbi/src/converter/*` (+ `react/ast_replace.ch`).
4. Runtime: `page.defaultUniversalSetup()` in `lang/libs/page/src/page.ch` — re-read the
   generated-output pattern before changing runtime behavior.

Fast triage questions:

- Is the initial HTML already wrong (SSR problem) or does it break after hydration (runtime)?
- Is the JS prop/text emission reactive (`$_ucs`) or one-time (frozen local)?
- Did an unresolvable value leak as `null`/`undefined` into attributes?
- Is the component's client function even emitted (dispatch queued but fn missing)?

## Component-library pitfalls

- **Nested universal wrapper elements break icon styling**: hydration boundary spans/wrappers
  add DOM levels; when a component commonly receives another universal child (e.g. `<Icon>`),
  style both the immediate child and one nested level (see `Button.ch`, `Surface.ch`).
- **CSS nested-selector meaning**: a top-level `.a[checked] + .b` can compile into
  `.a [checked] + .b` (descendant space). Prefer nesting inside the base selector
  (`.chx-toggle-input { &[checked] + .chx-checkbox-box { ... } }`) and verify the generated CSS.
- **Event handlers must reach the real `<input>`**, not just the outer `<label>`; don't spread
  all props onto the input if that duplicates `checked`/`name` in SSR output.

## Testing

- Compiler-plugin regressions: `lang/tests/compiler_plugins/universal/src/*` — run with
  `./scripts/test.sh --tcc --plugins`. Covers statement emission (`statement_emission.ch`),
  SSR safety/escaping/spreads (`ssr_safety.ch`), SSR expression evaluation
  (`ssr_expr_eval.ch`), body statements & return chains (`ssr_body_stmts.ch`), conditions
  (`cond_*.ch`), reactivity (`var_reactive_tests.ch`), entity decoding, and the historical
  `failure_regressions.ch` / `remaining_bugs.ch`.
- Parser roundtrips: `lang/tests/compiler_plugins/universal_runtime/` (`universal::parse_universal`).
- Browser E2E (real SSR → hydration → interaction): the `components_e2e` skill +
  `lang/compiled/components-e2e` (Playwright).
- Unit-ish tests of SSR helpers (`renderHtmlAttrs` escaping, None skipping) are plain
  `@test` functions in `ssr_safety.ch` — prefer adding there when the bug is in `page/ssr.ch`.

> ⚠️ **Non-ASCII in JS blobs**: the runtime JS is embedded in Chemical strings in `page.ch`.
> A non-ASCII character (e.g. an em dash in a comment) becomes a UTF-8 byte ≥ 0x80 in the
> emitted JS, which crashes `std::string::find` (Boyer-Moore skip table sign-extends signed
> chars). Keep every byte in `page.ch` ASCII.

## Deep design review — mistakes vs React/Solid (verified 2026-09-11)

> **Superseded by the comprehensive audit below** (2026-09-12). The section above contains
> all findings from this review plus additional issues discovered in the full source audit.
> The original findings are preserved here for traceability:

**Original verified bugs (now covered by audit items #1, #4):**
1. `useLayoutEffect` registers but never runs → audit #1
2. `capture_html_delta_to_js` misses `</script>` escape → audit #4
3. `runtime_contracts.ch` tests → see Testing section

**Original design gaps (now covered by audit items #16, #3, #2, #8, etc.):**
- No keys → audit #16
- No unmount/disposal → audit #3
- Async context loss → audit #8 (subscriber dedup) + architecture problem D
- Microtask-only effects → audit #2
- Mutable flags not stack → architecture problem D
- Three SSR evaluators → audit #5
- Never re-parsed JS → architecture problem C
- Per-page runtime duplication → audit #40 + architecture problem A
- SSR HTML in JS → audit #9
- Flush throws on missing fn → audit #28
- No createElementNS → audit #20
- memmove surgery → audit #17 + architecture problem G
- Silent UInteger fallback → audit #32
- SpecialAttrs bounds → audit #6
- Yield/ArrayDestructuring → audit #30, #31
- Spread-SSR limited → audit #14

## Known open gaps (verified in source, 2026-09)

- `react/template_builder.ch` + `react/render.ch` (`render_universal_jsx`,
  `UniversalTextBinding` et al) are a **dormant second hydration design**: only called by each
  other, reference `$_ut`/`__hydrate` which don't exist in the runtime. Don't fix bugs there;
  the live path is `converter/` + `react/ast_replace.ch`.
- SSR HTML is still transported through the JS bundle (`capture_html_delta_to_js` →
  `$_uc_h(html, name, props)`; backtick/`${`/`\`/newline escaped). The
  `lang/docs/universal-runtime-professionalization-plan.md` describes the marker/manifest
  replacement (Phase 2) — not implemented.
- Hydration of lists is positional; no `key` reconciliation (sorting/reordering lists can
  patch the wrong nodes).
- `$__universal_flush` throws (`$__uni_error`) when a queued component function is still
  missing at flush time; dispatch-time missing functions just queue.
- No compile-time diagnostic for unsupported prop types (silent `UInteger` fallback).
- The professionalization plan's segmented-JS-buffer / two-phase emission (removing
  `move_js_range` surgery) is not implemented.

## Comprehensive design audit (2026-09-12)

**Verified against source code and regression tests. Every finding references the specific
file and line in `lang/libs/page/` or `lang/libs/universal_cbi/src/`.**

### Production readiness verdict

> **Updated 2026-09-12:** several blocking items below have since been fixed —
> layout effects run, unmount/disposal is ownership-driven, effect deps compare
> by resolved value, `</script>` is escaped, and client-side keyed
> reconciliation/memoization exist. The remaining blockers are the untestable
> embedded runtime, per-page duplication, SSR HTML in JS, positional
> hydration, the three SSR evaluators, and no compile-time validation of
> emitted JS. See the professionalization plan §8.

The universal runtime is **not yet production-ready**. It is a functioning prototype with
correct SSR output for static content and a working signal/effect system, but it is still
missing critical infrastructure that every production SSR framework provides. The runtime JS
blob is untestable (embedded string), unminifiable (no tree-shaking), and duplicated per-page.
Hydration is positional (no keyed SSR→client matching), there is no compile-time validation of
the emitted JS, and SSR itself is blocking (no streaming).

Compared to React 19, Solid 2, Preact 10, and Svelte 5:

| Capability | React 19 | Solid 2 | Preact 10 | Svelte 5 | **Chemical** |
|---|---|---|---|---|---|
| Keyed list reconciliation | ✅ O(N) diff | ✅ fine-grained | ✅ O(N) diff | ✅ | ⚠️ client keyed; hydration positional |
| Error boundaries (client) | ✅ | ✅ `try` | ✅ | ✅ | ✅ partial (no SSR) |
| Error boundaries (SSR) | ✅ | ✅ | ❌ | ✅ | ❌ |
| Unmount / effect cleanup | ✅ owner tree | ✅ owner tree | ✅ | ✅ | ✅ ownership-driven (fixed) |
| Automatic batching | ✅ React 18+ | ✅ `batch` | ✅ | ✅ | ⚠️ microtask batch (no sync flush) |
| Layout effects | ✅ | N/A | ✅ | ✅ `bind:` | ✅ run before paint (fixed) |
| Keyed props memoization | ✅ `memo` | ✅ `.memo` | ✅ | ✅ runes | ⚠️ `$__uni_memo` exists |
| Shared runtime asset | ✅ (single React.js) | ✅ (single solid.js) | ✅ | ✅ | ❌ ~32KB per page |
| Suspense / async | ✅ | ✅ | ❌ | ✅ (`{#await}`) | ❌ |
| SSR streaming | ✅ | ✅ | ❌ | ✅ | ❌ blocking only |
| Lazy loading | ✅ `lazy()` | ✅ `lazy()` | ✅ | ✅ `{#await import}` | ❌ |
| Forward ref | ✅ | N/A | ✅ | ✅ `bind:this` | ❌ basic ref only |
| Context (nested providers) | ✅ | ✅ | ✅ | ✅ | ⚠️ flat name-keyed only |
| `useId` (stable SSR ids) | ✅ | N/A | ❌ | ✅ `$id` | ❌ |
| Compile-time validation | ❌ | ✅ (AOT) | ❌ | ✅ (AOT) | ❌ emitted JS never validated |
| Portal / modal system | ✅ | ✅ | ✅ | ✅ | ✅ (inert scan + floating) |

### Critical bugs (verified, blocking production use)

> **Status update (2026-09-12):** items #1, #3, and #4 below are **FIXED**. The
> original analysis is preserved for traceability. See the professionalization
> plan §8 progress log for the implementation.

#### 1. `useLayoutEffect` is registered but never executed — **FIXED**

`$__uni_mount` now drains `inst.layoutEffects` via
`$__uni_run_effects(inst, inst.layoutEffects)` (and the signal setter drains
them synchronously on state change). Contract test:
`runtime_contracts.ch::universal_layout_effects_are_ever_run` passes.

#### 2. No automatic batching — O(N×M) subscriber notification per state update

**File:** `page.ch:528-537` (`$_us` signal setter)

Every `signal.value = next` call synchronously notifies all subscribers in a `for` loop, then
schedules effects on a microtask. In a loop setting 10 signals with 5 subscribers each, that
is 50 synchronous calls. React 18 batches all state updates inside event handlers/promises;
Solid batches via `untrack`/`batch`.

```javascript
// page.ch:$_us setter (simplified):
s.set = function(next) {
  if(next === value) return
  value = next
  for(var i = 0; i < subs.length; i++) { subs[i](next) }  // O(subscribers) per signal
  // ... microtask effect scheduling
}
```

**Test:** `runtime_safety.ch::batched_state_updates` (would need runtime test)

#### 3. No unmount cleanup — subscriptions and event listeners leak — **FIXED**

The runtime now has ownership-driven disposal: `$__uni_dispose(inst)` walks the
instance tree, runs effect cleanups, unsubscribes deps, and disposes
render-scoped resources. Signals/computeds created during a render register with
the owning instance via `$__uni_register_resource` and expose `$_dispose`, so
removed components no longer retain their subscription graph. `$__uni_mount`
also disposes a previous instance tracked on the same host. Contract test:
`runtime_contracts.ch::universal_unmount_cleanup_exists` passes.

The original analysis follows (no `$__uni_unmount` symbol was added; the
existing `$__uni_dispose` is the disposer).

#### 4. `capture_html_delta_to_js` misses `</script>` XSS escape — **FIXED**

`capture_html_delta_to_js` now escapes `</script` as `\u003C/script`
(and `appendJsEscaped` escapes `</`), so SSR HTML cannot break out of an inline
`<script>`. Contract test:
`runtime_contracts.ch::universal_captured_html_is_inline_script_safe`.

#### 5. Three separate SSR evaluators with divergent coverage

**File:** `converter_utils.ch:1796-1905`, `converter_utils.ch:1945-2043`, `converter_utils.ch:2377-2533`

Three functions handle overlapping subsets of JS expressions for SSR:
- `convert_js_expr_to_ssr_bool_value` — conditions
- `convert_ssr_attr_bool_expr` — attribute booleans
- `convert_ssr_attr_value_expr` — attribute values

Each has different coverage. An expression that passes through the wrong evaluator silently
falls back to `None` (unresolvable). This is the root cause of many "SSR renders nothing"
bugs.

**Test:** Covered by `ssr_expr_eval.ch` — but parity gaps remain untested

#### 6. `SpecialAttrs` silently drops attributes beyond hardcoded limits

**File:** `ssr.ch:331-339`

Stack-allocated arrays: `classes[32]`, `styles[32]`, `others[64]`. When a component has >32
class expressions or >64 non-special attributes, entries beyond the limit are **silently
dropped** (guarded by `if(count < 32)` checks). No error, no warning, no dynamic growth.

```javascript
// ssr.ch:361 — silent drop:
if(special.class_count < 32) {
    // append class
} else {
    // silently dropped
}
```

**Test:** `ssr_safety.ch::ssr_special_attrs_32_class_limit`, `ssr_safety.ch::ssr_special_attrs_64_other_limit`

#### 7. `renderHtmlChildValue` boolean renders nothing

**File:** `ssr.ch:627-632`

A boolean child like `<div>{isActive}</div>` renders nothing server-side. React renders
`"true"` or `"false"`. This causes hydration mismatch if the client renders the string
version.

```javascript
// ssr.ch:627:
Boolean(_) => {}  // renders nothing
```

**Test:** `ssr_safety.ch::ssr_boolean_renders_true_false` (fails)

### High-severity design issues

#### 8. Signal subscriber duplication — no dedup guard

**File:** `page.ch:540-546` (`subscribe`)

The `subscribe` function blindly pushes. If the same effect subscribes to the same signal
through two dependency paths, the callback fires twice per update. React's `useEffect`
deduplicates deps; Solid's tracking graph is pointer-based.

#### 9. SSR HTML transported through JavaScript — ~2× bundle bloat

**File:** `page.ch:253-298` (`move_js_range`), `converter_jsx.ch` (`$_uc_h`)

Every component's SSR HTML is captured via `capture_html_delta_to_js` and embedded as a JS
template literal inside `$_uc_h(html, name, props)`. The same markup exists in both the HTML
response and the JS bundle. The professionalization plan (Phase 2) proposes marker-based
replacement — not implemented.

#### 10. JS hoisting via `memmove` buffer surgery — O(N) per component

**File:** `page.ch:253-298` (`move_js_range`)

Component functions are hoisted above dispatch lines using raw `memmove` on the pageJs
string buffer. Each component hoist is O(N) where N is the total JS length. For 50
components, this is O(50×N). The professionalization plan proposes segmented sections
(Runtime/ComponentDefs/Dispatches) serialized in order — not implemented.

#### 11. `is_reactive_var` linear scan on every identifier

**File:** `converter_utils.ch:91-103`

During conversion, every identifier triggers a linear scan through `state_vars` then
`computed_vars`. For a component with 50 identifiers and 20 state vars, that's 1000
comparisons. No hash set is used.

#### 12. `ssrValuesEqual` allocates two strings per comparison

**File:** `ssr.ch:202-208`

Creates `std::string a` and `b` on every call. In a hot loop (e.g., `active == index` for
100 items × 100 comparisons), that's 200 heap allocations. No string interning or fast-path
for identical types.

#### 13. `renderHtmlAttrsInternal` / `renderJsAttrsInternal` — ~60 lines duplicated

**File:** `ssr.ch:342-398` vs `ssr.ch:526-580`

Nearly identical functions with only the `is_first` output param differing. Any bug fix in
one must be manually replicated in the other.

#### 14. Spread-SSR limited to `props` identifier only

**File:** `converter_utils.ch:2719-2738`

Only `{...props}` (the component's own props parameter) is SSR-spreadable. `{...localObj}`
or `{...computedObj}` is silently dropped. React spreads any expression.

#### 15. `expr_references_reactive_var` missing node kinds

**File:** `converter_utils.ch:132-193`

Missing: `TryCatch`, `Switch`, `ForIn`, `ForOf`, `Throw`, `Yield`. A reactive var
referenced inside a `try` body or `for...of` loop won't get a computed wrapper — the
reference becomes a plain read (stale value).

#### 16. No `key` prop support — positional hydration only

**File:** `converter_jsx.ch:426-446`, `page.ch` (hydration)

Keys are never extracted or special-cased. Lists are hydrated by walking DOM children by
index. Sorting, filtering, or reordering a list patches the wrong nodes. React's
`reconcileChildren` does O(N) keyed diffing.

#### 17. No component memoization / `shouldComponentUpdate`

**File:** `emit_js.ch:1-27`

Client JS functions are emitted as plain `function` — no memoization wrapper, no prop
comparison. Every parent re-render calls the child function, re-evaluating the entire body.
React's `React.memo` and Solid's `.memo` signal avoid this.

#### 18. No error boundaries during SSR

**File:** `ast_replace.ch:171`

If a JSX element throws during SSR evaluation (null pointer in a Chemical expression), there
is no try/catch. The entire server render crashes. React's error boundaries catch during
render on both client and server.

#### 19. `ssrTextEquals` lossy float comparison

**File:** `ssr.ch:134-144`

Compares doubles by formatting to string with precision=2 and stripping trailing zeros.
`0.333` → `"0.33"` and `0.334` → `"0.33"` — different values compare equal. This is used
for hydration mismatch detection.

#### 20. No `createElementNS` — SVG/MathML broken

**File:** `page.ch` (`$_ur.createElement`)

SVG and MathML elements are created with `document.createElement` (no namespace) and never
render correctly. React uses per-element namespace detection.

### Medium-severity issues

| # | Issue | Location | Notes |
|---|---|---|---|
| 21 | `capture_html_delta_to_js` missing `\t`/`\0` escaping | `page.ch:328-342` | Minor: tabs harmless, null could truncate |
| 22 | `move_html_to_js_with_lambda_start` same missing escapes | `ssr.ch:645-657` | Same gap as #21 |
| 23 | `var state = ...` treated as reactive keyword | `converter_core.ch:280` | String comparison, not true keyword |
| 24 | `UnaryOp` prefix adds space for `delete` keyword | `converter_core.ch:76-78` | `delete obj.prop` becomes `delete obj.prop` (wrong) |
| 25 | `emit_ssr_body_statements` drops unsupported statements | `converter_utils.ch:2346` | No warning emitted |
| 26 | `writePrimitiveAttrValue` None emits nothing for class | `ssr.ch:297-299` | `class={undefined}` silently skipped |
| 27 | Component name collision risk | `emit_js.ch:14-15` | Mangling may not include unique prefix |
| 28 | `$__universal_flush` throws on missing component fn | `page.ch:1269` | Kills entire flush loop; should continue |
| 29 | Single-class attribute not wrapped in computed | `converter_jsx.ch:395-424` | Single reactive class won't update after mount |
| 30 | `Yield` emission lacks `;` | `converter_core.ch` | Syntax error in emitted JS |
| 31 | `ArrayDestructuring` emits array literal, not binding pattern | `converter_core.ch` | Destructuring declarations mis-emitted |
| 32 | Unsupported prop types silently become `UInteger` | `attr_value.ch` default | Pointer-as-number corruption |
| 33 | No `forwardRef` support | `emit_js.ch` | Basic ref only, no forwarding |
| 34 | No `useId` — SSR/client ID mismatch risk | runtime | IDs generated independently on server/client |
| 35 | No Suspense / async rendering | runtime | Blocking only |
| 36 | `ssrValuesEqual` allocates 2 strings per call | `ssr.ch:202-208` | Hot path perf issue |
| 37 | `renderHtmlChildValue` renders nothing for boolean | `ssr.ch:627-632` | Hydration mismatch with client |
| 38 | `is_reactive_var` linear scan per identifier | `converter_utils.ch:91-103` | O(identifiers × state_vars) |
| 39 | `move_js_range` memmove O(N) per component | `page.ch:253-298` | 50 components = O(50×N) |
| 40 | Per-page runtime duplication (~32KB each) | `defaultUniversalSetup()` | 20-page site = 640KB runtime |

### Architecture problems

#### A. Untestable embedded runtime

The entire client runtime (~800 lines of JS) lives as a string literal inside
`defaultUniversalSetup()` in `page.ch`. This means:
- No JavaScript linter can validate it
- No unit tests can exercise individual functions
- No source maps for browser debugging
- No tree-shaking — unused helpers (portals, context, inert scan = ~40% of blob) ship anyway
- No minification — the raw string is emitted as-is

**Recommendation:** Extract to a standalone `.js` file, add it as a build artifact, and
`<script src>` it. This is Phase 1 of the professionalization plan.

#### B. SSR/Client parity model is implicit

The three SSR evaluators and the client JS emitter have different supported subsets of the
same language. An expression can produce valid client output while producing empty SSR output.
There is no shared type system or AST model that enforces parity.

**Recommendation:** Single `SsrEvaluator` that delegates to the same node visitor as the
client emitter, returning `SsrAttributeValue` when possible and `None` when not. This
eliminates the three-way divergence.

#### C. No compile-time validation of emitted JS

The converter emits JS into `pageJs` without ever re-parsing it. A converter bug ships
invalid JS silently. The `universal_parser` already exists and is used at runtime — adding
a post-emission parse pass is ~50 lines.

**Recommendation:** After each page's JS is finalized, re-parse `getFinalizedPageJs()` with
`universal_parser` and emit a compile diagnostic on failure.

#### D. Global mutable state everywhere

The runtime uses `window.$__uni_current_instance`, `window.$__uni_current_tracker`,
`window.$__uni_child_tracker`, `window.$__uni_hydration_warned` — all mutable globals. This
makes concurrent rendering (React 18's transition API) impossible and causes subtle bugs when
multiple root components hydrate.

**Recommendation:** Thread instance context explicitly (e.g., `$_us(v, inst)` instead of
reading `window.$__uni_current_instance`). The professionalization plan §2.3 describes this.

#### E. Positional hydration is fragile

Hydration walks DOM children by index (`$__uni_hydrate_children`). If the server and client
disagree on child count (e.g., a conditional that evaluated differently), every subsequent
child is mismatched. React's keyed hydration identifies nodes by `data-reactid` attributes.

**Recommendation:** Emit stable IDs into hydration boundary spans and match by ID, not position.

### What to fix first (rewrite priority)

For the rewrite, these are ordered by impact (how many other problems they solve):

1. **Extract runtime to standalone JS file** — enables testing, linting, minification,
   tree-shaking, source maps. Unblocks everything else.

2. **Add keyed reconciliation** — solves list hydration mismatches, enables conditional
   rendering without full re-creation, brings parity with React/Solid.

3. **Add unmount cleanup / owner tree** — solves subscription leaks, event listener leaks,
   and makes the framework safe for dynamic component mounting/unmounting.

4. **Add batching** — solves O(N×M) subscriber notification, enables React 18-style
   automatic batching in event handlers.

5. **Single SSR evaluator** — eliminates parity bugs between server and client rendering.

6. **Post-emission JS validation** — catches converter bugs at compile time instead of
   shipping broken JS to browsers.

7. **Segmented JS sections** (Runtime/ComponentDefs/Dispatches) — eliminates `memmove`
   surgery, makes JS ordering deterministic.

8. **Server-side error boundaries** — prevents full-page crashes when a component throws
   during SSR.
