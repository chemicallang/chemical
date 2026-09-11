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
