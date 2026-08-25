# universal_cbi — Improvement Plan

**Status:** Analysis · **Scope:** `lang/libs/universal_cbi`, `lang/libs/page`, `lang/libs/components`, `lang/libs/html_cbi`, `lang/libs/universal_parser` / `js_parser`

This document is a deep review of the Universal component pipeline (SSR + hydration) with the goal of
building **large-scale web apps and websites** on top of it. It covers, in order:

1. How the pipeline works today (file map).
2. **Implementation & design improvements** — performance, flexibility, safety. This is the main body.
3. Feature-level gaps discovered during analysis (condensed from the deep-dive).
4. Prioritized roadmap.
5. Appendix: evidence (generated output, empirical experiments, verified browser behavior).

Every claim below was checked against the actual source and, where possible, against compiled output
and a real browser run.

---

## 1. How the pipeline works today

`#universal Component(props) { ... }` is a CBI compiler plugin:

| Stage | Files |
|---|---|
| Lexing (JS + JSX hybrid, Chemical `${}` escapes) | `universal_cbi/src/main.ch`, `lexer/JsLexer.ch` |
| Parsing (statements, expressions, JSX) | `universal_cbi/src/parser/*`, `ast/*` |
| Sym-res of components (`component not found`, required params) | `universal_cbi/src/sym_res/*` |
| JS→native SSR emission (`components_Button(page, attrs, children)`) | `universal_cbi/src/react/*`, `converter/*`, `react/ast_replace.ch` |
| Client JS emission (`function … { return $_ur.createElement(...) }` + `$__uni_dispatch`) | `converter/converter_jsx.ch`, `react/emit_js.ch` |
| SSR attribute value model | `page/src/ssr.ch` (`SsrAttributeValue` variant) |
| Hydration runtime (inline JS string) | `page/src/page.ch` → `defaultUniversalSetup()` |
| Component library | `lang/libs/components/src/*` |
| Usage samples | `lang/compiled/components` (multi-page demo site), `lang/compiled/learn-chemical` (docs site) |

**How a render works:**

1. The compiler generates a **native server function** per component, e.g.
   `void components_ButtonPrimary(struct page_HtmlPage* page, const struct page_SsrAttributeList* attrs, struct page_SsrText* children)`
   (verified in `lang/compiled/components/build/Translated.c:22624`). It appends HTML to `page.pageHtml`
   and appends the hydration JS to `page.pageJs`.
2. Component JS is emitted **once per page** (guarded by `require_component`/`set_component_hash`), then a
   `window.$__uni_dispatch('module_Component', element, props)` line mounts each instance.
3. The runtime (`defaultUniversalSetup`) provides fine-grained reactive primitives: `$_us` (state signal),
   `$_ucs` (computed signal), `$_ur` (vnode factory), a marker-comment-based hydration patcher
   (`$__uni_hydrate_node`), event binding (`$__uni_set_prop`), and `useEffect`/`useLayoutEffect`.
4. Attributes that cannot be SSR'd (functions, state) are skipped in HTML and passed through props to
   hydration instead — this "skipped attributes" bridge is `SsrAttributeValue`.

**What works (verified in a live browser):** the client-side reactivity model is solid. The shipped
TaskFlow todo demo hydrates correctly — counter ("3 left"), populated list, tabs, checkbox strikethrough,
no console errors. Static SSR output is correct, prop spread + class merging work, nested components and
fragments work.

**What is fragile:** the SSR side (empty stateful content), the JS language surface (statements silently
dropped), prop serialization (unescaped strings, silent type fallback), and several internal designs that
are duplicated, dead, or unbounded. These are the subjects of this document.

---

## 2. Implementation & design improvements

### 2.1 Codegen architecture: from string-building to structured emission

**Problem.** The entire JS output is built by string concatenation
(`converter.str.append_view("$_ur.createElement(\"", …)`) interleaved with AST-node pushes
(`put_chain_in` → one `append_js` call per chunk). Consequences:

- The generated JS is **never validated at compile time**. A converter bug (missing paren, wrong escape)
  ships invalid JS that only the browser discovers.
- There is no structural notion of "this is a component function", "this is a dispatch", "this is a
  string literal" — everything is text, so the converter cannot reason about its own output.
- Three different code paths emit "the same" element (`render_universal_jsx` in `render.ch`,
  `convertJSXNativeElement` / `convertJSXComponent` in `converter_jsx.ch`) with subtly different quoting
  (`'` vs `"`).

**Recommended change.**

1. **Keep string emission for speed, but add a "final JS verification pass".** The plugin already
   contains a complete JS parser (`universal_parser`). In a debug/assertions build (or always), feed the
   emitted `pageJs` text back through the parser. If it fails to parse, emit a compiler diagnostic with
   the component location instead of shipping broken JS. This single change converts every silent codegen
   bug into a compile error. (Guard it behind `--assertions`/debug builds if it proves expensive.)
2. **Introduce a tiny structured JS emitter** (`JsTextWriter`) with methods like
   `begin_function(name)`, `emit_call(callee, args)`, `emit_string(value)`, `end()`, that own quoting and
   escaping in exactly one place. The three parallel element-emitters then share it, eliminating the
   quoting inconsistencies.
3. **Two-phase emission.** Phase 1: walk the component AST and collect (a) component function bodies,
   (b) dispatches, (c) inline script. Phase 2: assemble in the correct order. This removes the need for
   the `move_js_range` reordering hack (see 2.3) and gives a natural place to run verification.

### 2.2 The JS bundle model: sections, hoisting, and shared runtime

**Problem.** `pageJs` is one flat `std::string`. Component functions must be defined **before** their
dispatches run, but the emit order is "render HTML, then append JS" interleaved with dispatch lines. The
current solution is post-hoc string surgery: `HtmlPage::move_js_range()` does `memmove`-based range moves
with malloc'd temps and a `js_hoist_pos` bookkeeping field (≈70 lines, 4 edge cases). This is O(n) per
hoist, subtle (off-by-one territory), and works only because universal currently appends to `pageJs`
(loaded at end of body).

**Recommended change.**

1. Replace `pageJs` string surgery with a **segmented buffer** on `HtmlPage`:
   `segments : vector<(text, section)>` plus an append position. Emit into sections
   (`Runtime` / `ComponentDefs` / `HydrateCalls`) and serialize in section order at the end. O(1) appends,
   no memmove, trivially correct ordering. Keep the `pageJs` accessors as serialization points so the
   public API doesn't change.
2. **Cross-page shared runtime.** Today each page file embeds the full ≈7 KB hydration runtime
   (`defaultUniversalSetup`). A 20-page site ships 140 KB of identical JS. Add
   `page.writeSharedUniversalRuntime(path)` that writes `uni-runtime.js` once, and have
   `htmlPageToString` reference it via `<script src>` instead of inlining when an opt-in flag is set.
3. **Cross-page component dedup.** `require_component` is per-page. For large multi-page sites, add a
   shared component manifest (`uni-components.js` with the union of used components), letting each page
   reference it. This is the single biggest byte win for large sites after the runtime.

### 2.3 Converter redesign: one SSR evaluator, one renderer, no flag toggling

**Problem (duplication).** The SSR evaluation logic exists in **three parallel, inconsistent engines**:

| Engine | Capability |
|---|---|
| `eval_ssr_js_expr` (attributes path) | literals, state-init text, `!`, `==`/`===`/`!=`/`!==`, boolean `&&`/`||`, ternary with bool condition |
| `convert_jsx_ssr_expression` (children path) | props reads, literals, `!`, `&&`, `||`, ternary — **but its condition helper `convert_js_expr_to_ssr_bool_value` cannot evaluate `active == 2`** |
| `convert_js_expr_to_ssr_bool_value` | props, literals, `!` only |

This asymmetry is a **real bug I reproduced**: tab button `style` attributes SSR'd correctly (because
`active == 0` was evaluable via `eval_ssr_js_expr`), while the same `active == 2 ? "Done" : …` as a
*child* of `ChipAccent` rendered empty. Two code paths, two different answers for the same expression.

**Problem (duplication, second instance).** `renderHtmlAttrsInternal` and `renderJsAttrsInternal` in
`page/src/ssr.ch` are ~80% identical (same spread recursion, same class/style/others accumulation) and
differ only in the output target. Same for `writePrimitiveAttrValue` vs `writeJsPrimitiveAttrValue`.

**Recommended change.**

1. **One `SsrEvaluator`**: a single tree-walking interpreter over the JS AST with a scope
   `{ stateInitializers, props: SsrAttributeList, constants }`. Both the attributes path and the children
   path call it. Add evaluation rules in one place: binary ops on constants, `&&`/`||` short-circuit with
   JS truthiness, ternaries, member access on state-initializer literals (objects/arrays), `.length`,
   simple arithmetic (`+`, `-`, `*`), and `.map`/`.filter` over static arrays (see 2.5).
2. **One `AttrRenderer`** parameterized by output target (`Html | Js | Css`) instead of two parallel
   `*Internal` functions.
3. **Replace boolean flag toggling** (`in_jsx_attribute`, `skip_reactive_deref`, `function_depth`) with a
   small `EmitContext` **stack** pushed/popped per conversion (attribute mode, hook-deps mode, function
   body). Flag-to-flag resets are how `skip_reactive_deref` leaks between args today.
4. **Stop reaching into `current_func.get_params().get(1)`/`get(2)`** (`make_ssr_prop_v_call`,
   `build_ssr_attributes`). `JsConverter` conflates "the server function being emitted" with "the JS
   component AST". Carry an explicit `SsrServerContext { propsParam, childrenParam, pageNode, … }`
   resolved once at emission start.

### 2.4 Two parallel hydration designs — pick one

`react/template_builder.ch` (`compute_universal_template`) defines a **second, dormant hydration design**
based on template paths and offsets (`$_ut(root, path, offset)`, `__hydrate`, `UniversalTextBinding` /
`UniversalPropTextBinding` / `NestedBinding`). It is **never called** (only `append_universal_component_js`
in `react/ast_replace.ch:101` is wired). It references runtime primitives (`$_ut`, `__hydrate`) that do
not exist in `defaultUniversalSetup`.

Keeping two designs in the tree is a maintenance hazard: fixes to one (reactive deps, subscriber
snapshots) silently leave the other stale. **Delete `template_builder.ch` and its binding types, or finish
and wire it with a clear migration plan.** If the template approach is kept for performance (it would
avoid per-node vnodes), it needs the runtime primitives implemented and its own test suite.

### 2.5 SSR engine: render state-derived content, honestly

**Problem (verified in shipped output).** For the todo demo, the initial `demo-todo.html` contains:

- `<h3 style="margin-top:0.3rem;"> left</h3>` — the `{todos.filter(…).length}` number is **missing**
- `<span class="…hDZnqF"></span>` — the toolbar badge is **empty**
- `<div class="todo-list"></div>` — the **entire task list is empty** (`.map()` over state not SSR'd)

Because the children path cannot evaluate state-derived expressions, every interactive section SSRs as an
empty shell. That means: no SEO content for interactive UI, bad LCP, and a visible "empty → pop-in" flash
on hydration. The attribute path already resolves *some* state from initializers
(`find_state_init_text`); the children path doesn't try.

**Recommended change.**

1. **Constant-fold children using state initializers.** `state todos = [ … ]` is a static JS literal —
   it is 100% SSR-able. Fold `todos.filter(t => t.visible && !t.done).length`, `todos.map(...)`, ternaries
   on state, and string concatenation over known values. The emitted SSR HTML then matches the hydrated
   DOM, which also removes a whole class of hydration-patch mismatches.
2. **Never render `"null"`.** The generated JS embeds `<div … style="null">` in `$_uc_h` payloads
   (verified in `main_TodoCard`'s captured HTML) because an unresolvable `style={props.cardStyle}` was
   serialized instead of skipped. `writePrimitiveAttrValue`'s `None()` case appends the literal string
   `"null"`; the converter should skip such attributes entirely (attributes already have a skip path —
   `is_non_ssr_expression` — extend it to "unresolvable at SSR time").
3. **Fix spread-SSR.** `build_ssr_attributes` has a `TODO`: any `{...expr}` spreads **the component's own
   `props`** in SSR, even when the source is an ordinary local object. That makes SSR HTML wrong while
   hydration is right → SSR/hydration mismatch. Only treat the actual props parameter as spreadable;
   for object literals, enumerate their statically-known members; otherwise skip with a diagnostic.

### 2.6 Runtime JS: extract, test, de-globalize, harden

**Problem 1 — the runtime is an un-testable inline string.** ~7 KB of JavaScript lives as a string literal
inside `HtmlPage::defaultUniversalSetup` in `page.ch`. It has no syntax checking, no linter, no unit
tests, and every edit is high-risk (the skill log documents multiple runtime bugs: subscriber mutation
during notification, `checked="null"`, `style=""`).

**Recommended change.** Move the runtime to a real asset — e.g.
`lang/libs/page/src/universal_runtime.js` read/embedded at build time (or a `.ch` file whose only job is
to ship the string) — and add a **Node-based unit test suite** for the runtime primitives (`$_us`,
`$_ucs`, `$_urn`, `$__uni_hydrate_node`, `$__uni_set_prop`) against a minimal DOM shim. This is the only
way to regress-test hydration logic without a full compiler build.

**Problem 2 — global mutable instance state.** `$_us` captures `window.$__uni_current_instance`, which is
a **single global** set around `comp(props)` in `$__uni_mount`. Nested mounts during render (a component
that renders another component synchronously) and async interleavings can corrupt the "current instance",
so effects land on the wrong component.

**Recommended change.** Thread an explicit `inst` context through the render call
(`$_us(v, inst)`, or a per-mount context object) instead of a module-global. This is a robustness fix
that costs little.

**Problem 3 — hooks emitted but not implemented.** The converter rewrites
`useState/useEffect/useMemo/useCallback/useRef/useContext/useReducer/useLayoutEffect/useErrorBoundary` to
`$_r.*`. The runtime `$_r` defines **only `useEffect` and `useLayoutEffect`** (verified in `page.ch`).
Using `useRef`, `useMemo`, `useCallback`, `useReducer`, or `useContext` in a component throws
`$_r.useRef is not a function` at mount. Either implement them (ref is trivial; `useMemo` can wrap
`$_ucs`) or reject them at compile time with a clear diagnostic. Partial-surface hooks are a trap.

**Problem 4 — no error isolation.** `$__uni_dispatch`/`$__universal_flush` have no try/catch; a single
failing component aborts the hydration queue and disables the rest of the page. Wrap each dispatch in
try/catch, log via `console.error`, and continue.

**Problem 5 — list updates are positional.** Hydration walks the DOM by index. Append-only lists work;
sorting, filtering, or reordering lists (dashboards, tables, feeds) patch the wrong nodes — checkbox
state, focus, and input values migrate to the wrong rows. Add a `key` prop:
1. Attach `key` to vnode children from `.map((item, i) => <X key={item.id} />)`.
2. Keyed reconciliation in `$__uni_hydrate_node` (move/insert instead of in-place patch) or, minimally,
   stable marker comments per key so subscribed lists reorder correctly.
This is the most important runtime feature for data-heavy large apps.

### 2.7 Safety: prop/attribute serialization

**Problem 1 — JS strings are emitted unescaped.** In `writeJsPrimitiveAttrValue`
(`page/src/ssr.ch`): `Text`/`PtrChar` values are written as `"` + raw + `"` with **no escaping of `"`,
`\`, newline, or control characters**; `Char` is `'` + raw + `'`. Consequences:

- A prop containing `"` breaks the JS bundle (parse error).
- A prop containing `\` performs escape injection (`\n`, `\u`, etc.).
- With `HtmlPage::toString()` (inline `<script>`), a prop containing `</script>` **breaks out of the
  script tag** → HTML injection. `writeToDirectory` (external `.js`) avoids the tag breakout but the
  broken-string/escape-injection issues remain.

This matches the documented "prop serialization breaks on special characters" failure mode. Fix in one
place: a proper `jsStringEscape` (escape `"`, `\`, control chars, and `</script>` defensively) used by all
`writeJsPrimitiveAttrValue` paths.

**Problem 2 — HTML attributes/children from dynamic props are not HTML-escaped.** In
`writePrimitiveAttrValue` (HTML target), `Text`/`PtrChar` values are appended raw into `attr="…"` and into
child content. A prop like `title={'"><img src=x onerror=alert(1)>'}` would break out of the attribute /
inject markup. Static JSX text IS escaped (`escapeHtml`), and `html_cbi` escapes its own attribute values
(`html_cbi/src/converter/language/attribute.ch`), but the universal SSR bridge does not. **Audit and fix:**
escape `&`, `<`, `>`, `"`, `'` in HTML-target primitive rendering. Add tests with hostile prop values.

**Problem 3 — unsupported prop types silently become integers.** In `convert_to_attr_value`
(`converter/attr_value.ch`), unknown types fall through to `wrapArgAttrValueVariantCall("UInteger", …)` —
a struct prop becomes a pointer-sized number in the JS bundle. Silent data corruption. **Fail loudly**:
emit a compile diagnostic for unsupported prop types and offer an escape hatch (the `getSsrAttributeValue`
protocol already exists for user structs — document and require it).

**Problem 4 — bounded stack arrays, no bounds checks.** `SpecialAttrs` in `ssr.ch` uses fixed
`[32]`/`[32]`/`[64]` arrays for classes/styles/other-attributes with unchecked `count++`. A single element
with >64 distinct attributes (e.g., a wide data-table row, or a heavy spread chain) **overflows the
stack**. Replace with growable vectors or bounds-checked code with a diagnostic.

### 2.8 Performance

Measured / observed hotspots, in rough order of impact for large sites:

1. **Per-page runtime duplication** (≈7 KB × pages) — see 2.2. Biggest win.
2. **`move_js_range` string surgery** — O(n) memmove per hoist; replaced by segmented buffer (2.2).
3. **Children HTML copying per nesting level.** For nested components, the converter renders children into
   `pageHtml`, copies the delta into a fresh `childrenHtml_` string, truncates the page, then passes the
   copy down (`converter_jsx.ch`). Deep trees pay O(depth × content) copies. Consider a page-level "cursor
   + region" API (`page.get_html_region(start) → view`) so children can be passed as a slice without a
   copy when the child will append them verbatim.
4. **Chunked appends already exist** (`put_chain_in` batches text runs) — keep this; just make sure the
   JS path uses the same batching as the HTML path.
5. **SSR memoization.** SSR functions are pure w.r.t. `(attrs, children)` for most components (the
   `${chem_expr}` escape hatch can read page state, so it must be opt-in). For large repeated subtrees
   (cards in loops, headers/footers), cache rendered HTML keyed by a hash of the SSR-able attr values.
   Page-level cache in `HtmlPage` (e.g., `doneComponents`-style LRU) is a natural place.
6. **Hydration cost** is already fine-grained (marker comments + direct node patching, no tree
   re-render) — do not replace it with a whole-tree virtual DOM. Keyed reconciliation (2.6) must not
   regress this: keep it O(children) with a key map.
7. **Bundle text size** — the generated JS uses backtick template literals and `$_uc_h(html, …)` wrappers
   that re-embed SSR HTML on the client (duplicate of what's already in `pageHtml`). For large pages this
   roughly doubles content. Consider emitting only the delta or only a reference to the SSR'd node when
   the SSR HTML is known to be final (no state-driven content inside).

### 2.9 Flexibility & extensibility

1. **Unify the two JS parsers.** `lang/libs/js_parser/src/TokenType.ch` and
   `lang/libs/universal_parser/src/TokenType.ch` define **identical** `JsTokenType` enums; `js_ide` and
   `universal_ide` duplicate semantic-token/folding logic. One shared JS parser library should serve
   `js_cbi`, `universal_cbi`, and the LSP — otherwise grammar fixes must be applied twice and drift.
2. **Typed props.** Components take `props` as untyped `any`; `props.typo` compiles fine and renders
   `undefined`. Design options, in increasing effort:
   - (a) Validate `props.<field>` accesses against the `:param` list declared on the component
     (`#universal Button(props: title, onClick)`), warning on undeclared fields.
   - (b) Let the component declare a Chemical struct type for props and type-check the JSX attribute
     expressions at the call sites.
   At minimum (a) — it converts the most common large-app bug (typos) into a diagnostic.
3. **Formalize the `$__uni_*` contract.** `html_cbi`, `universal_cbi`, and the page runtime all speak a
   shared, implicit JS-global protocol (`$__uni_dispatch(fn, target, props, mode)`, `$_uc_h`, `$_ur`,
   `$__uni_value`, …). Document it as a versioned contract (a doc page + optional
   `window.$__uni_version` handshake) so third-party macros and runtime tweaks don't silently break each
   other.
4. **Shared state / context primitive.** Large apps need cross-component communication beyond prop
   drilling. The signal primitives already support it (subscribe + `.value`); expose a first-class
   `createStore`/context helper in the runtime and teach the converter to treat store references like
   state references (`.value` rewriting, subscription tracking). This is mostly a library + converter
   convenience addition, not a new mechanism.
5. **Extensible hooks surface.** Decide the supported hook list explicitly (2.6, Problem 3), implement it,
   and reject unknown `use*` calls at compile time with a helpful message ("unknown hook; supported: …").

### 2.10 Safety: general robustness

1. **Global reentrancy** — see 2.6 Problem 2 (`$__uni_current_instance`).
2. **SSR/hydration divergence is the root of most rendering bugs.** Every place where SSR and hydration
   can disagree (spread-SSR, unresolvable attrs, state-derived children) should either render identically
   or skip in SSR and let hydration own it — never degrade into `"null"` or empty-with-no-marker. The
   unified evaluator (2.3) + honest SSR (2.5) addresses this systematically.
3. **`dangerouslySetInnerHTML` / `$_uc_h` html payloads** are by-design unescaped. Keep them, but audit
   that the SSR-embedded-HTML-in-JS path (`capture_html_delta_to_js`, `move_html_to_js_with_lambda_start`)
   escapes backticks/`$`/`\` correctly (it does today) and add tests for hostile HTML.
4. **Numeric serialization precision.** `Double` values are rendered with default precision 2
   (`append_double(value, 3)` in page append helpers). For large-scale data UIs this truncates
   (e.g., `1.999 → 2.00`). Make precision explicit per call site rather than a hidden default.
5. **Type-checking the emitted JS** (2.1) doubles as a safety net: nothing ships that the plugin's own
   parser cannot re-parse.

---

## 3. Feature-level gaps (from the analysis)

These are the user-facing capabilities that are missing or broken today. Implementation notes are in
§2; this section is the catalog.

### 3.1 Statements silently dropped from the generated JS (critical)

The parser supports `switch`/`case`/`default`, `while`, `do-while`, `break`, `continue`, `throw`,
`for…of`, `class`, `yield`, `import`, `export`, `debugger` — but `converter_core.ch::convertJsNode` has
**no emit case** for any of them, so they vanish silently. Verified empirically:

```js
// Source: switch(props.kind) { case "a": … case "b": … default: … }
// Emitted: function SwitchTest(props) { var label = "none"; return …{label}; }   // switch GONE

// Source: while(i < props.n) { sum += i; i++ }
// Emitted: var i = 0; var sum = 0; return …{sum};                                 // while GONE

// Source: if(props.bad) { throw new Error("boom") }
// Emitted: if(…props.bad)) { }                                                    // throw GONE
```

Matches `lang/compiled/universal_failures.md` #3/#4. **Either implement emission for these kinds (they
are already parsed — the work is small) or emit a hard diagnostic.** Silent miscompilation is the
single most dangerous gap for large apps (multi-state UIs use `switch`; pagination uses `while`; error
paths use `throw`).

### 3.2 Missing operators and tokens

- **`%` (modulo) has no lexer token** — `main.ch` has no `%` case → `unexpected token` parse errors
  (verified). Needed for pagination math, carousels, grid wrapping.
- **No `??` (nullish coalescing), `?.` (optional chaining), `**` (exponent)**.
- **No JS template-literal interpolation** — backtick strings are opaque; `${` always enters Chemical
  escape mode, even inside a JS template literal.
- **No regex literals** — the regression test `RegExpFromString` works around this with
  `new RegExp("…")`.

### 3.3 SSR renders stateful content empty (verified)

`demo-todo.html` shipped with `" left"` (missing number), an empty toolbar badge, and an empty task
list; the client fixed all of it after hydration. Impact: no SEO content for interactive sections, bad
LCP, visible flash. Root cause: the children-path SSR evaluator only handles props/literals/`!`, unlike
the attributes path. Fix in §2.5.

### 3.4 `style="null"` / attribute degradation (verified)

Unresolvable attributes render as literal `"null"` in the embedded SSR HTML (e.g., `main_TodoCard`'s
`<div … style="null">`). Should be skipped, not degraded. Fix in §2.5.

### 3.5 Hooks surface incomplete

`useRef/useMemo/useCallback/useReducer/useContext/useErrorBoundary` are emitted as `$_r.*` but undefined
at runtime. Fix in §2.6.

### 3.6 No list `key` support

Positional hydration breaks sorted/filtered/reordered lists. Fix in §2.6.

### 3.7 No error isolation in hydration

One failing component aborts the whole page's hydration queue. Fix in §2.6.

### 3.8 Component library gaps

The shipped demo pages self-report these: layout primitives (`Stack`/`Grid`/`Container`), forms
(`Form`/validation/submit), data views (sortable `Table`/`DataGrid`/pagination), navigation
(`Navbar`/`Breadcrumbs`/`Steps`), media (`Image`/`Gallery`/`Carousel`), feedback (`Toast`/skeletons),
rating/timeline, and accessibility primitives (focus trap for `Dialog`, keyboard nav for menus/tabs).
`Dropdown` in `Surface.ch` hand-injects raw JS (`page.pageJs.append_view`) because outside-click
behavior isn't expressible — a signal that event/ref facilities need work (see `ref` support and
document-level event helpers).

### 3.9 Testing can't catch any of the above

Tests are golden-string comparisons of generated JS text (`lang/tests/compiler_plugins/universal/src/`).
Add: (1) structural tests asserting `switch`/`while`/`throw`/`%` appear in output (or that a diagnostic
fires); (2) a headless-DOM harness that executes the generated JS and asserts the hydrated DOM (this is
how the todo demo was validated during this analysis); (3) SSR-content tests (`<h3>5 left</h3>`, not
`<h3> left</h3>`); (4)    move every `universal_failures.md` case into the suite.

### 3.10 Reactive render model: pitfalls that break conditional UI (verified, cdm `ErrorOverlay`)

While building `ErrorOverlay` (a global uncaught-error catcher) in
`lang/libs/components/src/ErrorOverlay.ch`, two framework behaviors repeatedly prevented the
overlay from ever appearing. Both are direct consequences of the runtime's
**"component bodies run once; only derived/conditional JSX nodes re-render"** model (the
`$_ucs` computed-signal patcher in `defaultUniversalSetup`).

**Pitfall A — `#css` style helpers are server-only and crash at hydration.**
`#css { … }` helpers such as `error_overlay_styles(page : &mut HtmlPage) : *char` take the SSR
`page` pointer. On the client the component is invoked as `factory(props)` with **no `page`
argument**, so calling the helper throws during hydration → the component hits the error
boundary (`$__uni_render_fallback`) and renders nothing. Symptom: the component is silently
absent and nothing appears, with no visible error.
- **Workaround (proven):** do not use `#css` helpers inside `#universal` components that must
  render on the client. Use inline `style={{ … }}` objects (string/number values) or plain
  `class="…"` strings, matching the existing app dialogs (e.g. `CdmApp`'s Tools dialog uses
  `class` + inline `style`). For reusable styling, emit the CSS once in a top-level
  `<style>`/theme and reference class names instead of per-component `#css` helpers.

**Pitfall B — visibility toggles must be a JSX conditional child, never control-flow
`if`/`return`.**
`if(!open) { return null }` is evaluated **once at mount** and is not a reactive binding, so
flipping `open` later does nothing. Storing the conditional in a local `var overlay = open ? …
: null` and then `return overlay` is also one-time — the local is not wrapped in a computed
signal. Only a conditional used **directly as a JSX child expression** becomes a `$_ucs`
computed that re-evaluates when its state dependencies change.
- **Workaround (proven):** render `{open && errors.length > 0 ? <div …>…</div> : null}` inline
  as a child (wrap in an inert `<div style={{display:"contents"}}>` if a single root is
  needed). All state reads (`open`, `errors`, `selected`, `copied`) must happen *inside* that
  conditional so they subscribe. Never `return` a precomputed local.

**Net result:** once both pitfalls were avoided (inline styles + inlined conditional child),
`window.__reportError(msg, stack)` and the `window.addEventListener("error" /
"unhandledrejection")` handlers installed in the component's `useEffect` correctly flip
`open` and the modal renders. This is the pattern every show/hide component (dialogs, toasts,
dropdowns, modals) in this framework must follow.

Suggested roadmap additions:
- (N) In `#universal` components, hard-warn (or reject) `#css` helper usage in positions that
  execute during client hydration — or make `#css` a no-op that returns a stable class name on
  the client.
- (N) Document the "conditional child, not `if`/`return`" rule in the component authoring guide
  and ideally lint for early-`return`-on-state patterns.

---

## 4. Prioritized roadmap

| # | Change | Kind | Where | Effort | Impact |
|---|---|---|---|---|---|
| 1 | Emit or reject `switch`/`while`/`do-while`/`throw`/`break`/`continue`/`for-of`/`class` | Correctness | `converter/converter_core.ch` | S | Critical |
| 2 | Re-parse own emitted JS (verification pass) | Safety | new pass after emit | S–M | Critical |
| 3 | Unified `SsrEvaluator` (one engine for attrs + children, state-initializer folding) | Correctness/Perf | `converter/converter_utils.ch` | M | High |
| 4 | `jsStringEscape` + HTML-escape dynamic attr/child values; skip unresolvable attrs (no `"null"`) | Safety | `page/src/ssr.ch`, `converter/attr_value.ch` | S–M | High (XSS) |
| 5 | `%`, `??`, `?.`, `**`, template interpolation, regex literals | Language | `main.ch`, `parser/*` | M | High |
| 6 | Extract runtime JS to a file + Node unit tests | Safety/DX | `page`, new test dir | M | High |
| 7 | Implement `useRef`/`useMemo`/`useCallback`/`useReducer`/`useContext` or reject unknown hooks | Correctness | `page.ch` runtime, converter | S–M | High |
| 8 | Keyed list reconciliation | Correctness | `page.ch` runtime | M–L | High |
| 9 | Error isolation per dispatch | Robustness | `page.ch` runtime | S | Medium |
| 10 | Segmented JS buffer (remove `move_js_range`), two-phase emission | Perf/Maint | `page.ch`, converter | M | Medium |
| 11 | Fix spread-SSR (only spread real props; enumerate object literals) | Correctness | `converter_utils.ch` | S–M | Medium |
| 12 | `SpecialAttrs` bounds-checked / growable | Safety | `page/src/ssr.ch` | S | Medium |
| 13 | Typed/validated props (validate `props.x` against declared params) | DX | `sym_res/`, parser | M | Medium |
| 14 | Cross-page shared runtime + component manifest | Perf | `page.ch`, writer | M | Medium–High |
| 15 | Unify `js_parser` / `universal_parser` (+ IDEs) | Maint | libs | M | Medium |
| 16 | Remove or finish dormant `template_builder.ch` design | Maint | `react/` | S | Medium |
| 17 | SSR memoization for deterministic subtrees | Perf | converter, page | M | Medium |
| 18 | Children-HTML slice passing (avoid per-level copies) | Perf | `converter_jsx.ch` | M | Medium |
| 19 | Store/context primitive + converter support | Flexibility | runtime + converter | M | Medium |
| 20 | Runtime test harness (headless DOM) + move `universal_failures.md` into tests | DX | tests | M | High |

**Suggested first batch (2–3 weeks):** items 1, 2, 3, 4, 7, 9 — they fix silent miscompilation, the XSS
surface, and the SSR/attribute correctness issues with mostly localized changes. Items 5, 8, 14, 20 are
the next tranche for "large-scale" readiness.

---

## 5. Appendix: evidence

### 5.1 Empirical compile experiments (this analysis)

Test module compiled with `TCCCompiler` (`--mode debug_quick --no-cache`):

| Construct | Result |
|---|---|
| `switch`/`case`/`default` in component body | compiles; emitted JS omits the entire switch |
| `while` loop | compiles; emitted JS omits the loop (`sum` stays 0) |
| `throw new Error(...)` | compiles; emitted JS turns the `if` body into `{ }` |
| `a % b` | **parser error** `unexpected token` (no lexer token for `%`) |
| `for(var item of arr)` | parse-error cascade |

### 5.2 Generated output (shipped demo app, `lang/compiled/components`)

- `output/demo-todo.html` (SSR): ` left` counter, empty badge, empty `.todo-list`, `value=""` input.
- `output/demo-todo.js` (client): `$_ucs(() => todos.value.filter(...).length)` etc. — hydration repairs
  everything; `main_TodoCard` embeds `<div … style="null">` in its `$_uc_h` payload.
- Headless Chrome (`--headless=new --dump-dom` after JS execution): counter "3 left", badge "All",
  6 checkbox inputs, 5 visible todo cards, **zero console errors**, no `style="null"` remaining.

### 5.3 Code-level facts

- `converter_core.ch::convertJsNode` — switch has no `While/DoWhile/Switch/Throw/Break/Continue/ForOf/
  ClassDecl/Yield/Import/Export/Debugger` cases (all fall through `default => {}`).
- `main.ch` (lexer) — no `%`, `?`-`?`, `?`-`.` tokens; backtick reads to closing backtick regardless of
  `${`.
- `page.ch::defaultUniversalSetup` — `window.$_r = { useEffect, useLayoutEffect }` only.
- `page.ch::move_js_range` — memmove-based string surgery + `js_hoist_pos`.
- `ssr.ch` — `writePrimitiveAttrValue`/`writeJsPrimitiveAttrValue` write `Text`/`PtrChar` raw (no HTML or
  JS escaping); `None()` writes `"null"`; `SpecialAttrs` uses fixed `[32]/[32]/[64]` arrays.
- `converter_utils.ch` — three SSR evaluators with different capabilities; `build_ssr_attributes` spread
  `TODO`.
- `react/template_builder.ch::compute_universal_template` — never called; references `$_ut`/`__hydrate`
  which don't exist in the runtime.
- `js_parser/src/TokenType.ch` ≡ `universal_parser/src/TokenType.ch` (duplicate `JsTokenType`).
- `lang/compiled/universal_failures.md` — 5 documented failure modes, none yet in the test suite.

### 5.4 Files most likely to change

| Concern | Files |
|---|---|
| Statement emission, hooks, operator tokens | `universal_cbi/src/converter/converter_core.ch`, `universal_cbi/src/main.ch`, `universal_cbi/src/parser/*` |
| SSR evaluation & attribute building | `universal_cbi/src/converter/converter_utils.ch`, `converter/converter_jsx.ch`, `converter/attr_value.ch` |
| Runtime & hydration | `lang/libs/page/src/page.ch` (`defaultUniversalSetup`), new `universal_runtime.js` |
| Serialization & escaping | `lang/libs/page/src/ssr.ch` |
| Component library additions | `lang/libs/components/src/*` |
| Tests | `lang/tests/compiler_plugins/universal/src/*` |
