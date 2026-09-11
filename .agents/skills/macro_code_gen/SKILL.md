---
name: Macro Code gen
description: How compiler macro plugins (html_cbi, universal_cbi, css_cbi) turn Chemical source into generated code — page-buffer emission, SSR server functions, hydration dispatches, and styled components.
---

Macros are CBI plugins compiled by TinyCC at build time. They parse their block
(`#html { ... }`, `#universal Name(props) { ... }`, `#styled Name("div") { ... }`) into an
AST, then generate real Chemical AST nodes (function calls, statements) that reference
`HtmlPage` methods and SSR helpers from `lang/libs/page`.

## `#html` blocks (html_cbi)

```chemical
func my_html(page : &mut HtmlPage) {
    #html {
        <span>Hello World</span>
    }
}
```

generates approximately:

```chemical
func my_html(page : &mut HtmlPage) {
    page.append_html_view("<span>Hello World</span>")
}
```

Chemical values inside `{}` become interleaved append calls:

```chemical
func my_html(page : &mut HtmlPage, name : &std::string_view) {
    #html { <span>Hello {name}</span> }
}
// generates:
func my_html(page : &mut HtmlPage, name : &std::string_view) {
    page.append_html_view("<span>Hello ")
    name.writeToPageBody(page)   // HtmlPageWriter impl, lang/libs/page/src/PageWriter.ch
    page.append_html_view("</span>")
}
```

Key facts:

- `#html` does **NOT** auto-escape interpolated values — callers escape untrusted data with
  `escape_html` / `escape_html_view` (`lang/libs/page/src/page.ch`).
- An element opened in one `#html` block must be closed in the same block (no splitting).
- Inside JSX/HTML mode, `@{ ... }` escapes back into Chemical for statements (loops, ifs);
  use nested `#html { }` blocks inside the escape to emit elements. Increment loop indices
  before the nested block; each `@{}` must be self-contained.
- Universal components used inside `#html` are handled specially (see below), including
  `@{}` loop rendering of component lists.

## `#universal` components (universal_cbi)

```chemical
#universal Greeting(props) {
    return <div>Hello {props.name}</div>
}
```

The macro (`universal_cbi/src/react/macro.ch`) parses an optional typed/optional param list
(`#universal Card(props : title, onClick, isWide?)`) into a `JsComponentDecl` embedded node.
During symres, universal_cbi generates a **server function**:

```chemical
func <module>_<Name>(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void
```

The server function body (`universal_cbi/src/react/ast_replace.ch`) does three things:

1. `if(page.require_component(hash)) { page.set_component_hash(hash); ... }` — appends the
   **client component JS** `function <module>_<Name>(props) { ... }` to `pageJs` once per page
   (hash = the component function's encoded location). The JS block is hoisted above earlier
   dispatch lines with `page.move_js_range(...)` so component functions are defined before use.
2. Appends SSR HTML to `pageHtml` by converting the component body (the full SSR evaluation
   rules are in the `universal` skill).
3. Non-SSRable attributes (event handlers, refs) are skipped in HTML and passed through the
   hydration props instead.

When a `#universal` component is used inside `#html`, `html_cbi` renders:

```html
<span id="u{loc}" data-chx-i>...SSR HTML...</span>
```

plus a hydration trigger in `pageJs`:

```js
window.$__uni_dispatch('module_ComponentName', document.getElementById('u{loc}'), {props})
```

`data-chx-i` spans are `display:contents` (added by `page.defaultUniversalSetup()`) so the
boundary never affects layout. `$__uni_dispatch` mounts immediately if the component function
exists, otherwise queues it (`window.$__uni_hydration_queue`) until `$__universal_flush()` at
the end of the bundle. For the SSR/hydration/reactivity model, load the `universal` skill.

## `#styled` components (css_cbi)

```chemical
#styled Card("div") {
    background: #ffffff;
    border: 1px solid #cccccc;
    padding: 8px;
}
```

`#styled` declares a reusable, SSR-only styled component whose CSS is scoped to a
compiler-generated hash class and injected into the page automatically — no manual
`#css` + `class={...}` wiring, and **no hydration**.

Syntax variants:
- `#styled Name("div") { ... }` — tag as a string literal.
- `#styled Name(.div) { ... }` — shorthand dot-tag form.
- `#styled Wrap(Inner) { ... }` — wrap mode: forwards to the inner component `Inner`; css_cbi
  merges its generated class with `Inner`'s onto the rendered element. When `Inner` is a
  **universal** component, `signature.hydrateFunctionNode` points at it, so `html_cbi`'s
  hydration dispatch targets the inner component (the styled wrapper is SSR-only).

Usage:

```chemical
#html {
    <Card class="xl">Hello <Title>World</Title></Card>
}
```

renders `<div class="hAz5DrX xl">Hello ...</div>` (base hash class first, user classes merged
after — `renderHtmlAttrsWithBase` in `lang/libs/page/src/ssr.ch`) and emits
`.hAz5DrX{...}` into the page CSS once (`require_css_hash`/`set_css_hash`).

Key properties:
- The generated class is a content hash of the CSS — stable/deterministic across renders.
- Works in `#html` blocks and is usable cross-module (import the declaring module).
- On the SSR side it is just a call to the generated server function with the attribute list
  and pre-rendered children (same `(page, attrs, children)` shape as universal servers).

## Shared plumbing you will touch when adding a macro

- **Buffer emission**: everything ends up in `HtmlPage` (`lang/libs/page/src/page.ch`):
  `pageHead`, `pageHtml`, `pageCss`, `pageHeadJs`, `pageJs` (+ `pageJsEnd`). Buffer-targeted
  helper families exist: `append_html*`, `append_css*`, `append_head_js*`, `append_js*`
  (the last one targets `pageJs`), including escaped variants
  (`append_js_escaped_char_ptr` / `append_js_escaped`) that quote/backslash/`</`-escape for
  embedding inside JS string literals.
- **SSR value model** (`lang/libs/page/src/ssr.ch`): build `SsrAttributeList` /
  `SsrAttributeValue` values and let `renderHtmlAttrs` / `renderJsAttrs` render them — they
  handle class/style merging, dedup, escaping, and skipping unresolvable values. User structs
  can plug in with `getSsrAttributeValue(&mut self, page) : SsrAttributeValue`.
- **SymResSupport**: plugins resolve every `HtmlPage` method / SSR helper they emit calls to
  (see `universal_cbi/src/sym_res/support_fix.ch` for the pattern) and emit compile errors
  when a required helper is missing — do this instead of emitting unresolved names.
- **Generated SSR function bodies bypass symres type determination**: call plain helper
  functions (`ssrMakeTextValue`, `ssrMultipleGet`, ...) instead of raw index/variant
  operations, or LLVM codegen crashes on unresolved types.

> Known generated-code pitfalls (deep review in the `universal` skill): emitted JS is never
> re-parsed before shipping; unsupported prop types silently become `UInteger`; layout effects
> are registered but never executed; per-page runtime duplication (~32 KB inline per page);
> `</script>` not escaped in captured SSR HTML. Contract tests pin these in
> `lang/tests/compiler_plugins/universal/src/runtime_contracts.ch`.

For developing new compiler plugins or the plugin API itself, load the `cbi_plugin_api`
skill. For compile-time intrinsics/reflection used by macros, load
`intrinsics_compiler_reflection`. For the full universal SSR/hydration model, load `universal`.
