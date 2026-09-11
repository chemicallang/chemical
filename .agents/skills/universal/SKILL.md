---
name: universal
description: Diagnose, fix, and implement features in the Chemical universal component pipeline. Use when working on `#universal` components, `universal_cbi`, generated SSR/hydration output, `HtmlPage` runtime behavior, or bugs where compiled HTML/CSS/JS disagree with source components.
---

# Universal

There are **two** universal-related libraries:

## 1. Compiler Plugin: `universal_cbi` (`lang/libs/universal_cbi/`)

The `#universal` macro compiler plugin. Processes universal component definitions at compile time, generates:
- Server-side rendering (SSR) function (C++ → binary)
- Client-side hydration JavaScript (emitted to `pageJs` bundle)
- CSS output

Universal components work inside `#html` blocks (processed by `html_cbi`) and are the primary way
to build interactive UI. They do SSR + hydration for fast initial paint and full interactivity.

> **Note:** React, Preact, and Solid framework bridges (`react_cbi`, `preact_cbi`, `solid_cbi`) have been
> removed. The universal component system is the only supported component model.

## 2. Runtime Package: `universal` (`lang/libs/universal/`)

A **standalone runtime library** that allows parsing universal component source (JS + JSX) at runtime, **without the compiler**:

```chemical
var out = universal::parse_universal("<div>hello</div>")
```

### Architecture

Reuses the shared `universal_parser` package (which powers the `#universal` compiler macro) with runtime implementations of the compiler `Parser` and `BatchAllocator` static interfaces.

### Files

| File | Purpose |
|------|---------|
| `src/main.ch` | Public API: `tokenize_universal`, `parse_universal`, `convert_universal_node_to_string` |
| `src/tokenizer.ch` | `UniversalTokenizer` — JSX-aware tokenizer replicating the compiler's JSX state machine (`jsx_depth`, `in_jsx_tag`, `jsx_brace_count`, `tag_mode_stack`, `jsx_brace_stack`) |
| `src/converter.ch` | `UniversalRuntimeConverter` — Walks parsed AST (`JsBlock` of JS/JSX nodes) and re-emits source text to `std::string` |

### Key Components

#### `UniversalTokenizer` (`tokenizer.ch:13`)
- **State machine** for JSX: tracks `jsx_depth` (nested JSX elements), `in_jsx_tag` (inside opening tag), `jsx_brace_count` (braces inside JSX expressions), `tag_mode_stack` (stack of `in_jsx_tag` values for nested braces), `jsx_brace_stack` (stack of brace counts per depth)
- **Tokenizes** into `compiler::Token` values (from `lang/libs/compiler`)
- **Handles**: JSX text children, JSX tags (`<`, `>`, `/>`, `</`), JS expressions in `{...}`, JS operators, keywords, identifiers, numbers, strings, templates, comments

#### `UniversalRuntimeConverter` (`converter.ch:11`)
- **Converts** parsed AST back to source text
- **Visitor pattern** over `JsNodeKind` enum (from `universal_parser`)
- **Supports**: All JS/JSX constructs — var/const/let, functions, classes, control flow, JSX elements/fragments/attributes/expression-containers/spread, imports/exports, etc.
- **Ignores**: `ChemicalValue` nodes (embedded Chemical values not re-emitted at runtime)

### Public API (`main.ch`)

```chemical
// Tokenize JSX/JS source string → vector<Token>
public func tokenize_universal(view : std::string_view) : std::vector<Token>

// Parse + convert JSX/JS source string → string (round-trip)
public func parse_universal(view : std::string_view) : std::string

// Convert single parsed node → string
public func convert_universal_node_to_string(node : *mut JsNode) : std::string
```

### Internal Flow (`parse_universal`)

1. `tokenize_universal` → `UniversalTokenizer.tokenize()`
2. Create `RuntimeParser` (implements compiler `Parser` interface)
3. Create `ASTAllocator` + `ASTBuilder` (implements compiler `BatchAllocator`)
4. `parseUniversalRoot(&mut parser, &mut builder)` → `*mut JsBlock` (from `universal_parser`)
5. `convert_universal_root(root, &mut converter)` → writes to output string
6. `allocator.deinit()` → cleanup

### Dependencies (`chemical.mod`)

```
module universal
source "src"
import cstd
import std
import compiler
import html_comp
import universal_parser
import compiler_runtime
```

### Use Cases

- **Runtime parsing** of universal component source strings
- **Tooling** that needs to analyze/transform JSX at runtime
- **Testing** universal component parsing without full compiler
- **Hot-reload / live-preview** systems

### How universal performs ssr + hydration.

Universal uses the compiler api from the compiler library (`lang/libs/compiler`), it generates a function (a server function) that exists in the binary
that function takes three parameters, first the page reference, second the attribute list (struct for which is present in `lang/libs/page`)
third the text for the children, yes we pass children as `SsrText` (a struct in `lang/libs/page`, its like a `string_view`)

The server function does two things, it appends a js function that would perform hydration into the js bundle, It also appends the server side rendered html
to the html bundle.
The universal component system captures this html and puts it into the js bundle for hydration.

When I say js bundle or html bundle, A struct HtmlPage present in `lang/libs/page` is used for each page, it contains strings
in which we append, `pageHtml`, `pageHeadJs`, `pageJs`, these fields are used to write the final output.

`universal_cbi` appends to the `pageJs`, so its components are present in the js loaded at the end of body.
This means we have to use a queue (`$__uni_hydration_queue`) to hydrate universal components once they have been rendered.

One very important thing to note:

The html (server rendered), does NOT contain attributes that use js expressions or for example js lambdas, because we cannot ssr them.
These skipped attributes are passed to the hydration function we generated (only the skipped attributes). The js function we generated
takes the element, skipped attributes as arguments.

#### Std Library Usage

We heavily use the `lang/libs/std` which provides us heavily used things like `std::string`, `std::string_view`, `std::vector`

---

## Generated JS Architecture

### How JS is generated for universal components

The JS bundle for a page containing universal components is generated by `universal_cbi` and contains three types of code:

1. **Component factory functions** — generated by `emit_js.ch` (`universal_cbi/src/react/emit_js.ch`). Each universal component becomes a JS function:
   ```javascript
   function MyComponent(props) { ... }
   ```

2. **Hydration dispatch calls** — generated by `emit_universal_queue` (`html_cbi/src/converter/language/main.ch:187`). Each universal component usage generates:
   ```javascript
   window.$__uni_dispatch('MyComponent', document.getElementById('u0'), { props... });
   ```

3. **SSR-to-JS bridge** — generated by `convertJSXComponent` (`universal_cbi/src/converter/converter_jsx.ch:186-260`). For JSX-based universal components, an IIFE captures SSR HTML into a template string:
   ```javascript
   (() => { const html = `<div>SSR content</div>`; return $_uc_h(html, "MyComponent", {props}); })()
   ```

### Key runtime helpers (`lang/libs/page/src/page.ch`)

| Helper | Line | Purpose |
|--------|------|---------|
| `$__uni_dispatch(fn, target, props, mode)` | 477 | Mounts a universal component onto a DOM target |
| `$_uc_h(html, name, props)` | 843 | Creates a vnode from SSR HTML string (BAD — see below) |
| `$__uni_html(html)` | 845 | Wraps HTML string as `{__uni_html}` for hydration (BAD — see below) |
| `$_uc(factory, props)` | 1251 | Mounts a universal component via vnode tree |
| `$_urn(v)` | 977 | Renders a vnode tree to DOM nodes |
| `$__uni_hydrate_node(parent, dom, v)` | 1057 | Hydrates a single vnode onto existing DOM |
| `$__uni_hydrate_children(parent, values)` | 1049 | Hydrates multiple vnodes |
| `$__uni_set_prop(el, key, value)` | 874 | Sets a DOM property/attribute, handles events, styles |
| `$__uni_apply_prop(el, key, value)` | 971 | Sets prop + subscribes to reactive changes |
| `$_us(initial)` | 519 | Creates a reactive state signal |
| `$_ucs(fn)` | 549 | Creates a computed signal |
| `$_um(parts...)` | 494 | Merges prop objects, unwraps reactive values |
| `$_ur` | 515 | Runtime vnode helpers (`Fragment`, `createElement`) |
| `$__uni_value(v)` | 844 | Unwraps a state signal to its value |
| `$__uni_is_state(v)` | 833 | Checks if value is a state signal |
| `$__universal_flush()` | 1257 | Processes the hydration queue |
| `$__uni_warn_hydration(msg, expected, got)` | 834 | Logs hydration mismatch (once per page) |
| `$__uni_createContext(name, default)` / `$__uni_useContext(name)` | 666/689 | Context registry (name-keyed) |
| `$__uni_floating(trigger, menu, opts)` | 730 | Positions portaled menus with flip logic |
| `$__uni_inert_scan()` | 784 | Manages `inert` attribute for modal portals |
| `$__uni_mount(host, comp, props, mode)` | 1220 | Mounts component with error boundary + effects |
| `$__uni_run_effects(inst, effects)` | 806 | Runs effects with dependency tracking |

---

## BAD PATTERNS (Do Not Use / Must Fix)

> These patterns are known issues in the current universal runtime. They generate incorrect, insecure, or bloated JS.

### 1. `$__uni_html` — HTML strings as children (CRITICAL)

**Location**: `html_cbi/src/converter/language/main.ch:264`

**Generated code**:
```javascript
"children":window.$__uni_html("<div>hello</div>")
```

**Runtime wrapper** (`page.ch:845`):
```javascript
window.$__uni_html = ((html) => ({ __uni_html: html || "" }))
```

**Hydrator** (`page.ch:999-1001`):
```javascript
if(v && v.__uni_html !== undefined) {
    const tpl = document.createElement("template");
    tpl.innerHTML = v.__uni_html;
    return tpl.content.cloneNode(true);
}
```

**Why it's bad**:
- **XSS vulnerability** (CRITICAL): The HTML escaping in `emit_universal_queue` (lines 271-281) only escapes `"`, `\`, `\n`, `\r`, `\t`. It does NOT escape `<`, `>`, `&`. Any HTML/JS in child content is injected directly via `innerHTML`.
- **HTML duplication**: Same HTML exists in `pageHtml` AND in the JS bundle. Doubles payload size.
- **No reactivity**: Children are static strings. Dynamic content frozen at SSR time.
- **Hydration inconsistency**: Hydrator returns `dom` unchanged at page.ch:1150 (`if(v && v.__uni_html !== undefined) return dom;`), assuming parent handles it. If parent doesn't, hydration breaks.

### 2. `$_uc_h` — SSR HTML embedded as template strings (CRITICAL)

**Location**: `universal_cbi/src/converter/converter_jsx.ch:217-259`

**Generated code**:
```javascript
(() => { const html = `<div class="btn">hello</div>`; return $_uc_h(html, "MyComponent", {class:"btn"}); })()
```

**Runtime** (`page.ch:843`):
```javascript
window.$_uc_h = ((html, name, props) => ({ t: "__uni_uc", p: { html, name, props } }))
```

**Why it's bad**:
- **Template string injection** (CRITICAL): `capture_html_delta_to_js` (page.ch:328-342) escapes `` ` ``, `${`, `\`, `\n`, `\r` but does NOT escape `</`. If the HTML contains `</script>`, it breaks out of the inline `<script>` block.
- **HTML in JS bundle**: SSR HTML is duplicated in JS. Same payload issue as `$__uni_html`.
- **No checksum**: No verification that SSR HTML matches client rendering.
- **`innerHTML` hydration**: Creates `<template>` element + `innerHTML` = HTML reparse on every hydration.
- **No reconciliation**: Bypasses virtual DOM entirely. Direct DOM injection.

### 3. `capture_html_delta_to_js` — Incomplete escaping (HIGH)

**Location**: `lang/libs/page/src/page.ch:328-342`

**Problem**: Escapes `` ` ``, `${`, `\`, `\n`, `\r` but does NOT escape `</`. The `append_js_escaped` function (page.ch:264-293) DOES escape `</` but `capture_html_delta_to_js` doesn't use it. If the HTML contains `</script>`, it breaks out of the `<script>` block.

### 4. `move_html_to_js_with_lambda_start` — Same escaping issue (HIGH)

**Location**: `lang/libs/page/src/ssr.ch:645-660`

Same issue as `capture_html_delta_to_js` but specifically for head JS. Moves HTML from `pageHtml` to `pageHeadJs` with a lambda wrapper. Same incomplete escaping.

### 5. `$_u` reference — Undefined (MEDIUM)

**Location**: `universal_cbi/src/react/template_builder.ch:103`

```javascript
init.append_view("{const c=(window.$_u&&window.$_u['");
```

`window.$_u` is never defined in the runtime (`page.ch`). This appears to be dead/undocumented code for nested component hydration. If `window.$_u` is undefined, the condition short-circuits and the nested hydration is silently skipped.

---

## Comparison: Chemical vs React

### Architecture Comparison

| Aspect | React | Chemical |
|--------|-------|----------|
| **SSR output** | HTML string + JSON component tree | HTML string + HTML strings in JS |
| **Hydration** | Virtual DOM matching + event attachment | DOM vnode walking + `innerHTML` for pre-rendered |
| **Children passing** | JSON props/children | HTML strings (`$__uni_html`, `$_uc_h`) |
| **Updates** | Virtual DOM diffing | Direct DOM manipulation via `$__uni_set_prop` |
| **SSR verification** | Checksum (`react-checksum`) | Text node warning only |
| **Reconciliation** | Full tree diffing | No diffing — direct DOM ops |
| **Bundle size** | Small (JSON tree) | Large (HTML strings duplicated) |
| **Security** | Auto-escaped | Partially escaped (XSS risk) |
| **Streaming SSR** | Supported (React 18+) | Not supported |

### Key Differences Analysis

**1. Children Passing (Biggest Difference)**

React passes children as a JSON-encoded tree. Chemical passes children as HTML strings. Chemical's approach is simpler (no client-side tree construction) but less flexible (no reactivity in children, XSS risk, larger bundle).

**2. Hydration Strategy**

React reconciles virtual DOM with actual DOM, attaches event listeners, preserves DOM nodes. Chemical walks a vnode tree, matches it against DOM nodes, uses `innerHTML` for pre-rendered content blocks. React's approach is more robust (handles any mismatch gracefully). Chemical's approach is faster for initial hydration (no tree construction) but fragile (mismatches can corrupt DOM).

**3. Bundle Size**

React's component tree is JSON-encoded (small). Chemical embeds HTML strings in JS (large — every component's children are duplicated from the HTML into the JS bundle).

---

## Plan: Remove HTML from JS Bundle

### Current HTML-in-JS Paths

| Path | File | Line | Mechanism |
|------|------|------|-----------|
| `$__uni_html` | `html_cbi/src/converter/language/main.ch` | 264 | Children as HTML string in dispatch call |
| `$_uc_h` | `universal_cbi/src/converter/converter_jsx.ch` | 217-259 | SSR HTML as template string in IIFE |
| `move_html_to_js_with_lambda_start` | `lang/libs/page/src/ssr.ch` | 645-660 | HTML moved to head JS |
| `capture_html_delta_to_js` | `lang/libs/page/src/page.ch` | 328-342 | HTML delta captured to JS |

### Migration Strategy

**Phase 1: Add vnode-based children passing**

Replace `$_uc_h(html, name, props)` with `$_uc(name, props, children)` where `children` is a vnode tree (like React's approach):

```javascript
// Instead of:
window.$_uc_h("<div>hello</div>", "MyComponent", {class:"btn"})

// Generate:
window.$_uc("MyComponent", {class:"btn"}, [window.$_urn("hello")])
```

**Phase 2: Update the converter**

- In `converter_jsx.ch:217-259`, replace the IIFE pattern that embeds HTML template strings with vnode-based output
- In `html_cbi/src/converter/language/main.ch:264`, replace `"children":window.$__uni_html("...")` with a JSON-encoded children array

**Phase 3: Update the hydrator**

Update `$__uni_hydrate_node` in `page.ch` to handle vnode children instead of `__uni_html`. The `__uni_uc` case needs to call the component function with props and hydrate the returned vnodes into the DOM, NOT use `innerHTML`.

**Phase 4: Remove dead code**

- Remove `$_uc_h` (page.ch:843)
- Remove `$__uni_html` (page.ch:845)
- Remove `capture_html_delta_to_js` (page.ch:328-342)
- Remove `move_html_to_js_with_lambda_start` (ssr.ch:645-660)
- Remove `__uni_html` handling in hydrator (page.ch:999-1001, 1150)

### Expected Benefits

| Metric | Before | After |
|--------|--------|-------|
| JS bundle size | Large (HTML strings) | Small (JSON vnodes) |
| XSS surface | High (innerHTML) | Low (DOM API) |
| Children reactivity | Frozen at SSR | Dynamic (vnode tree) |
| Hydration speed | Fast (innerHTML) | Slightly slower (DOM creation) |
| Bundle complexity | High (template strings, escaping) | Low (JSON serialization) |

---

## How universal performs ssr + hydration.

Universal uses the compiler api from the compiler library (`lang/libs/compiler`), it generates a function (a server function) that exists in the binary
that function takes three parameters, first the page reference, second the attribute list (struct for which is present in `lang/libs/page`)
third the text for the children, yes we pass children as `SsrText` (a struct in `lang/libs/page`, its like a `string_view`)

The server function does two things, it appends a js function that would perform hydration into the js bundle, It also appends the server side rendered html
to the html bundle.
The universal component system captures this html and puts it into the js bundle for hydration.

When I say js bundle or html bundle, A struct HtmlPage present in `lang/libs/page` is used for each page, it contains strings
in which we append, `pageHtml`, `pageHeadJs`, `pageJs`, these fields are used to write the final output.

`universal_cbi` appends to the `pageJs`, so its components are present in the js loaded at the end of body.
This means we have to use a queue (`$__uni_hydration_queue`) to hydrate universal components once they have been rendered.

One very important thing to note:

The html (server rendered), does NOT contain attributes that use js expressions or for example js lambdas, because we cannot ssr them.
These skipped attributes are passed to the hydration function we generated (only the skipped attributes). The js function we generated
takes the element, skipped attributes as arguments.

#### Std Library Usage

We heavily use the `lang/libs/std` which provides us heavily used things like `std::string`, `std::string_view`, `std::vector`

## Issues

Trace universal issues across four layers, in this order:

1. Read the source component or page using `#universal`.
2. Read the generated output in the compiled package, especially `output/*.html`, `output/*.css`, and `output/*.js`.
3. Read the universal compiler pieces in `lang/libs/universal_cbi`.
4. Read `page.defaultUniversalSetup()` in `lang/libs/page/src/page.ch` before changing runtime behavior.

Do not assume the bug is in the component source. Many failures come from compiler output or hydration runtime behavior.

## Primary files

- `lang/libs/universal_cbi/src/converter/*`
- `lang/libs/universal_cbi/src/react/*`
- `lang/libs/page/src/page.ch`
- `lang/libs/page/src/ssr.ch` — `SsrAttributeValue`, `renderJsAttrValue`, serialization to JS
- `lang/libs/page/src/PageWriter.ch` — `HtmlPageWriter` interface, string serialization
- `lang/libs/components/src/*`
- `lang/compiled/*/output/*.html`
- `lang/compiled/*/output/*.css`
- `lang/compiled/*/output/*.js`

## Debug workflow

For interactive regressions, compare all three generated artifacts:

- In HTML, check the initial SSR state. Look for suspicious attrs such as `style=""`, `checked="null"`, duplicated attrs from prop spreading, or missing initial text/content.
- In JS, check whether state-derived props are emitted as reactive wrappers like `$_ucs(() => ...)` instead of one-time values.
- In CSS, check whether nested selectors compiled correctly. Universal CSS generation can accidentally introduce descendant spaces that change selector meaning.

If the user mentions dialog, tabs, toggles, or other interactivity, inspect generated output before editing source.

## Common failure modes

### Reactive props compiled as one-time values

Symptoms:

- Dialog closes or opens only after an extra click
- Tab content appends or leaves stale panels visible
- Button visibility changes but paired content does not
- Radio state text changes but visual state does not

What to check:

- `components.js` should emit state-dependent attrs and text as `$_ucs(() => ...)` when they depend on `state`.
- `convert_jsx_runtime_expr` and related JSX conversion paths must wrap state-derived expressions, not flatten them once.

Relevant files:

- `lang/libs/universal_cbi/src/converter/converter_core.ch`
- `lang/libs/universal_cbi/src/converter/converter_jsx.ch`
- `lang/libs/universal_cbi/src/react/jsx_props.ch`
- `lang/libs/universal_cbi/src/converter/converter_utils.ch`

### Attribute values from plain locals are frozen (reactivity gotcha)

A local variable computed before `return` is evaluated **once at render** and its value
is inlined as a static attribute — it does NOT update when state changes. Only
attribute **expressions** that directly read state/props (or call `$_ucs`) subscribe.

```chemical
// FROZEN — never re-evaluates:
var activeDesc = ""
if(open && props.options && props.options[highlight] != null) {
    activeDesc = "chx-select-opt-" + highlight
}
<button aria-activedescendant={activeDesc} />

// REACTIVE — converter wraps this in $_ucs(() => ...) so it updates live:
<button aria-activedescendant={open && props.options && props.options[highlight] != null ? "chx-select-opt-" + highlight : ""} />
```

Same rule applies to child props: `var checked = props.checked || false` freezes the
value. Pass `props.checked` directly in the attribute when it must stay reactive
(see ToggleGroupItem/RadioGroupItem).

### Effects re-run on reactive dep changes

`useEffect(fn, [dep])` re-runs when any dep in the array is a state/computed whose
signal value changes — including computeds derived from **parent-controlled props**
(e.g. `isOpen = props.open != null ? props.open : open`). This is what lets a Dialog
focus trap react when a parent toggles `open`. Deps that are plain values only run
once. Do not rely on effects running after every render.

### Hydration mismatch detection

`$__uni_hydrate_node` warns once via `$__uni_warn_hydration` when hydrated text/tag
content disagrees with the SSR DOM, then self-corrects (never crashes). Look for
`Hydration mismatch` in the console while debugging; the runtime also records
`$__uni_hydration_warned` so pages can surface it.

### Portals (`createPortal`) — escaping overflow/transform clipping

Overlays that must escape `overflow: hidden` / `transform` ancestors (Select menu,
Dialog, Sheet) render through `$_r.createPortal(children)`:

```chemical
return createPortal(
    <div class={${overlay_styles(page)}} style={isOpen ? "" : "display:none;"}>
        {props.children}
    </div>
)
```

Runtime contract (`lang/libs/page/src/page.ch`):

- `createPortal(...)` returns a `{ t: "__uni_portal", c: [...] }` vnode marker.
- **SSR** renders the portal children inline where the portal sits (no body on the
  server) — output identical to a normal subtree.
- **Hydration** (`$__uni_hydrate_node`): the SSR'd nodes sit at the current `dom`
  position; they are hydrated in place, then the `[startDom, cur)` range is MOVED into
  a container appended to `document.body`. State subscriptions reference elements, so
  they keep working after the move.
- **`$_urn`** (fresh client render) creates the body container directly.
- **`$__uni_floating(trigger, menu, opts)`** anchors a portaled menu under its trigger
  with fixed coordinates (`getBoundingClientRect`), re-measured on scroll/resize.
  Returns a cleanup used as the effect's return value.

Component-side rules:

- **Never toggle menu visibility with an inline reactive `style`** when the menu is
  portaled and floating-positioned: the style subscription sets `el.style.cssText`,
  wiping the inline `position/top/left`. Use `data-open={open ? "true" : "false"}` +
  CSS `&[data-open="true"] { display: grid; }`.
- The component root must spread `{...props}` so custom attributes (`data-testid`,
  `aria-*`) reach the DOM.

Converter support (needed once per new portal component):

- `converter_core.ch` hook-name switch: add `createPortal` → `$_r.` prefix.
- `react/utils.ch` `unwrap_returned_jsx_node` / `find_returned_jsx`: unwrap
  `createPortal(<jsx/>)` to the inner JSX element, or the component's JS function is
  never emitted and SSR is skipped.
- `converter_utils.ch` `convert_jsx_ssr_expression` FunctionCall case: `createPortal`
  renders its argument inline.

### Error boundaries (`useErrorBoundary`)

A component whose render throws must not take down the page:

- `$__uni_mount` wraps `comp(props)` in try/catch. On error it logs
  `[universal] component render failed` and renders `$__uni_render_fallback(inst,
  props, err)` — the component's `useErrorBoundary(fallback)` result, or the default
  `.chx-error-boundary` UI (styled by the components theme).
- Effect bodies/cleanups and event handlers are also wrapped: a throwing handler or
  effect is logged and contained.
- **Scope:** boundaries are per-component. Each universal component mounts
  independently, so a parent's `useErrorBoundary` does NOT catch a child's render
  error — declare the boundary in the component that can throw.

```chemical
#universal BadComponent(props) {
    useErrorBoundary(() => <p role="alert">Fallback shown</p>)
    var boom = () => { throw new Error("bad component"); }
    boom()
    return <p>never rendered</p>
}
```

### Context system (`createContext` / `useContext`)

Components have NO module-level JS declarations (a component's JS function is
only emitted when used), so context cannot be a shared top-level object. Context
is a **name-keyed registry in the runtime** (`window.$__uni_ctx`), keyed by a
string both sides derive from a shared prop:

```chemical
// provider (group):
const ctx = createContext("rg-" + (props.name || "default"), "")
ctx.value = value                    // publish the state signal (wired)
ctx.write = (v) => { value = v; if(props.onValueChange) { props.onValueChange(v) } }

// consumer (item):
const ctx = useContext("rg-" + (props.__rgName || props.name || "default"))
// reads are reactive: ctx.value inside a $_ucs() computed subscribes
<input checked={ctx.value == props.value} />
<button onClick={() => { if(ctx.write) { ctx.write(props.value) } }} />
```

Contract:

- **Registry** (`page.ch` runtime): `createContext(name, default)` is idempotent
  (first call creates the entry, keyed by name); the entry holds a `$_us` signal.
  Assigning a **signal** to `ctx.value` WIRES the entry to follow it (provider
  publishes its state/computed); assigning a plain value sets it directly.
  `useContext(name)` ensures the entry exists (default undefined) and returns it.
- **Converter** (`universal_cbi`): `const X = createContext(A, B)` /
  `useContext(A)` registers `X` as a context var (name + default expressions).
  `ctx.*` reads count as reactive: JSX attrs referencing them wrap in `$_ucs`,
  so consumer components re-render when the provider's signal changes.
  `ctx.value = <signal>` emits the RAW signal (no deref) so the runtime wiring
  kicks in; `props.x = expr` assignments emit the raw LHS (the `$__uni_value`
  getter is an invalid assignment target).
- **SSR**: a provider's SSR function runs AFTER its children render (children
  HTML is pre-rendered and passed in as the 3rd arg), so a consumer can never
  observe a published value at SSR. `ctx.value` reads therefore resolve to the
  STATIC createContext default (or None for useContext), and `ctx.value = x` /
  `ctx.write = fn` are no-ops at SSR. Groups render children unpressed/
  unchecked at SSR; hydration applies the selection. This is documented in the
  components (see RadioGroup/ToggleGroup).
- **Name threading**: groups inject their `name` into item child vnodes
  (`c.p.props.__rgName = props.name`) so items resolve the key without repeating
  `name` on every item. The injection mutates the `$_uc_h` vnode props before
  mount (group body runs before children mount), then the item spreads `{...props}`
  — injected keys appear as DOM attributes (harmless, e.g. `__rgname="tg-main"`).
- **No provider**: `useContext` without a matching `createContext` returns an
  entry with an undefined value; comparisons are false, items stay unpressed/
  unchecked, and `ctx.write` is absent (guarded calls no-op).

Relevant files:

- Runtime registry: `lang/libs/page/src/page.ch` (`window.$__uni_ctx`,
  `$_r.createContext`, `$_r.useContext`)
- Converter: `lang/libs/universal_cbi/src/converter/converter_core.ch` (VarDecl
  context registration, `ctx.value = x` / `props.x = y` assignments),
  `converter_utils.ch` (`is_context_var`, reactive detection, SSR default
  resolution, `emit_ssr_assignment_stmt` no-op)
- Components: `lang/libs/components/src/RadioGroup.ch`, `ToggleGroup.ch`

> ⚠️ **Non-ASCII in JS blobs**: the runtime JS is embedded in Chemical strings
> in `page.ch`. A non-ASCII character (e.g. an em dash in a comment) becomes a
> UTF-8 byte ≥ 0x80 in the emitted JS, which crashes `std::string::find` (the
> Boyer-Moore skip table sign-extends signed chars). Keep every byte in `page.ch`
> ASCII.

### `$__uni_floating` collision flipping

Portaled menus position themselves under the trigger with fixed coordinates. When
`spaceBelow < menuHeight + gap` they FLIP above the trigger (`bottom` anchored,
`maxHeight` clamped to available space). This is required: a fixed menu opening
below the fold is unreachable (fixed elements can't be scrolled into view).

### Modal portals and inert background

When a modal overlay (Dialog, Sheet) opens via `createPortal(children, {modal: true})`,
the portal container gets `data-uni-modal`. The runtime helper `$__uni_inert_scan()` scans
all `<main>` / `<body>` children: non-portal siblings and non-modal portal containers get
`inert` added; modal portals and their children are exempt. On close the scan runs again
to remove inert.

Key rules:

- Pass `{modal: true}` as the second arg to `createPortal` for modals (Dialog/Sheet).
- Non-modal portals (Select menu, DropdownMenu) must NOT pass `{modal: true}`.
- `inert` on a parent makes all descendants inert (not focusable, not clickable for AT).
- The portal container itself is outside the inert subtree (it's appended to `document.body`).
- `el.inert` (DOM property) only reflects the element's own attribute, NOT inherited inert
  state. Use `el.closest("[inert]")` to check if an element is effectively inert.

The `page.ch` runtime helper:

```js
window.$__uni_inert_scan = () => {
    const main = document.querySelector("main") || document.body;
    for (const kid of main.children) {
        kid.inert = !kid.hasAttribute("data-uni-portal") || kid.hasAttribute("data-uni-modal");
    }
};
```

Dialog/Sheet components add a `useEffect([isOpen])` that calls `$__uni_inert_scan()` on
both open and close.

### Subscriber mutation during notification

Symptoms:

- First click updates only part of the UI
- Second click updates the paired control
- Dialog button hides but dialog does not appear until another click
- Tabs partially update on each click

Cause:

- A state or computed-state subscriber unsubscribes/resubscribes while the runtime is iterating the same subscriber array.

Fix pattern:

- In `page.defaultUniversalSetup()`, snapshot subscriber arrays before notifying them.
- Apply this both to plain state (`$_us`) and computed state (`$_ucs`).

Relevant file:

- `lang/libs/page/src/page.ch`

### SSR attrs emitted incorrectly

Symptoms:

- `style=""` on elements that should be hidden or shown
- `checked="null"` in HTML
- Wrong initial tab or dialog state before hydration

Rules:

- Non-SSRable expressions should be skipped instead of degraded into bogus attrs.
- Simple state-derived expressions that can be resolved from known state initializers should be SSR-evaluated so initial DOM matches hydrated DOM.

Important:

- `current_func` on `JsConverter` is the server function, not the JS component AST.
- Do not read `current_func.body` expecting JS statements.
- If SSR evaluation needs state initial values, cache them explicitly during JS conversion.

Relevant files:

- `lang/libs/universal_cbi/src/converter/converter_utils.ch`
- `lang/libs/universal_cbi/src/converter/converter_base.ch`
- `lang/libs/universal_cbi/src/converter/converter_core.ch`

### Prop serialization breaks on special characters

Symptoms:
- `JSON.parse` error in browser console when mounting a universal component
- "Bad control character in string literal" in JSON
- "Unexpected identifier" or "Unterminated string" errors
- Garbage bytes (`�`) in the generated JavaScript props object

Root cause:
- The universal system's `renderJsAttrValue` in `lang/libs/page/src/ssr.ch` wraps `string` values in **single quotes** (`'...'`) without escaping `'` or `\` inside them
- This means any `'` in the data breaks the JS string, and any `\` followed by a JS-significant character (like `n`, `"`, `\`, etc.) creates unintended escape sequences
- C++ structs with `vector<>` fields produce corrupt output when serialized — the serializer doesn't handle complex nested types

Workarounds:
1. **Never pass C++ structs with `vector<>` fields** as universal component props
2. **Pre-serialize complex data to JSON** using `quiz_to_json()`-like functions
3. **Post-process the JSON string** with `js_string_escape()` that doubles backslashes and escapes single quotes as `\u0027`
4. **Keep all string data on the JS side** (defined in `state` inside the component) whenever possible

Debug steps:
1. Fetch the page HTML and find the `$__uni_dispatch(...)` call
2. Examine the third argument (props object) — look for `�` (garbage bytes) or broken quotes
3. Check if any prop value contains `'` or `\` that would be mangled by single-quote wrapping
4. Check if any prop value is a C++ struct/object that the serializer can't handle

Relevant files:
- `lang/libs/page/src/ssr.ch` — `renderJsAttrValue()` serialization logic
- `lang/libs/page/src/PageWriter.ch` — `HtmlPageWriter` interface

### Toggle visuals do not match state

Symptoms:

- Caption text changes but checkbox/switch/radio visuals stay static

What to check:

- Event handlers must reach the real `<input>`, not just the outer `<label>`.
- Do not blindly spread all props to the `<input>` if that duplicates `checked`, `name`, or other attrs in SSR output.
- Verify the compiled CSS selector meaning in `output/*.css`.

Known selector pitfall:

- Writing top-level `.chx-toggle-input[checked] + ...` can compile into `.chx-toggle-input [checked] + ...` with an unwanted descendant space.
- Prefer nesting under the base selector, e.g. inside `.chx-toggle-input { &[checked] + .chx-checkbox-box { ... } }`, then verify the generated CSS.

Relevant file:

- `lang/libs/components/src/Toggle.ch`

### Nested universal wrapper elements break icon styling

Symptoms:

- `IconButton` glyph looks off-center
- `Fab` shows a dark icon pill above the blue fab background
- Styling direct children is not enough

Cause:

- Universal hydration boundaries often wrap children in an extra node such as `<div id=...><span ...></span></div>`.

Fix pattern:

- Style both the immediate child and one nested descendant level when the component commonly receives another universal child such as `<Icon>`.
- Verify against generated HTML, not just source JSX.

Relevant files:

- `lang/libs/components/src/Button.ch`
- `lang/libs/components/src/Surface.ch`

## Practical checks by feature

### Dialog

Verify:

- Initial HTML has the correct visible/hidden state.
- `components.js` uses reactive `style` for both the dialog and the open button.
- Clicking once changes both controls.
- No bogus `style=""` remains unless it is intentionally empty.

### Tabs

Verify:

- Only one panel is initially visible in HTML.
- Clicking a tab updates tab button styling and panel visibility on the same click.
- Panels are not appended repeatedly due to partial hydration updates.

### Toggles

Verify:

- Generated HTML does not contain `checked="null"`.
- Event handlers are attached to the input that owns the `checked` attr.
- Generated CSS selectors still mean "same element has `[checked]`".

### IconButton and Fab

Verify:

- Generated HTML shape for the icon child.
- CSS neutralizes the nested icon background and border where needed.
- Glyph alignment is checked against compiled output, not inferred from source.

## Editing guidance

- Prefer fixing the smallest layer that explains the generated output.
- If output is wrong, do not only tweak the component source. Confirm whether the compiler or runtime is producing the wrong HTML/CSS/JS.
- When changing universal runtime behavior, treat it as cross-cutting. Re-read the generated output pattern first.
- If the user explicitly says not to compile or run tests, do not do so.

## Fast triage questions

Ask yourself:

- Is the initial HTML already wrong before hydration?
- Is the JS prop/text emission reactive or one-time?
- Is the runtime updating all subscribers in one click?
- Did CSS compilation change selector meaning?
- Did a nested hydration wrapper invalidate the styling assumption?
