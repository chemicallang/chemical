---
name: Macro Code gen
description: How compiler macro plugins (html_cbi, css_cbi, js_cbi, universal_cbi, json_cbi, md_cbi) turn Chemical source into generated code — page-buffer emission, SSR server functions, hydration dispatches, styled components, and the ASTBuilder API they use.
---

Macros are CBI plugins compiled by TinyCC at build time. They parse their block
(`#html { ... }`, `#universal Name(props) { ... }`, `#styled Name("div") { ... }`) into an
AST, then generate real Chemical AST nodes (function calls, statements) that reference
`HtmlPage` methods and SSR helpers from `lang/libs/page`.

For the CBI registration/API mechanics, load `cbi_plugin_api`; for the AST nodes the
plugins build, load `compiler_api`; for the full SSR/hydration behaviour see `universal`;
for the app-level authoring guide see `design_web_app`.

## Plugin / macro map (verified from `lang/libs/*/build.lab`)

| Macro | Plugin | Registration | Parse hook type | Notes |
|---|---|---|---|---|
| `#html` | `html_cbi` | `html_cbi/build.lab:21-92` | `ParseMacroNode`, `ParseMacroValue` | JSX/HTML → `page.append_html*` |
| `#css` | `css_cbi` | `css_cbi/build.lab:21-104` | `ParseMacroNode`, `ParseMacroValue` | CSS → `page.append_css*`, returns class name |
| `#styled` | `css_cbi` (second CBI name) | `css_cbi/build.lab:91-98` | `ParseMacroTopLevelNode` | SSR-only scoped component |
| `#js` | `js_cbi` | `js_cbi/build.lab:22-103` | `ParseMacroNode`, `ParseMacroValue` | JS → `page.append_js*` |
| `#universal` | `universal_cbi` | `universal_cbi/build.lab:23-110` | `ParseMacroTopLevelNode` | SSR fn + client JS + hydration |
| `#json(Type)` | `json_cbi` | `json_cbi/build.lab:18-60` | `ParseMacroTopLevelNode` | Generates `std::Serializer`/`Deserializer` impls |
| `#md` | `md_cbi` | `md_cbi/build.lab:20-85` | `ParseMacroNode`, `ParseMacroValue` | Markdown → `page.append_html*` |
| `#universal_test("name")` | `html_cbi` (second macro name) | `html_cbi/build.lab` | `ParseMacroTopLevelNode`, `ParseMacroNode` | Component test: SSR fixture + raw `<script>` steps; see `universal_testing` |

`MountStrategy` (`lang/libs/html_comp/ast.ch:6-13`) still declares `Preact`, `React`, `Solid`
enum values, but **no plugin registers `#preact`, `#react`, or `#solid`** — only `Universal`
and `Styled` are live. React/Preact/Solid bridges were removed (confirmed by grepping
`lang/libs/*/build.lab` for those CBI names — no matches). `MountStrategy.Preact/React/Solid`
are dead enum slots.

## How a macro plugin is wired (build.lab → CBI hooks)

Every plugin `build.lab` exposes
`build(ctx : *mut BuildContext, user_job : *mut LabJob) : *mut Module` and does the same steps
(`html_cbi/build.lab:13-95`, `universal_cbi/build.lab:15-113`):

1. Cache check + idempotency: `ctx.get_cached`/`ctx.set_cached`, then
   `ctx.contains_cbi`/`ctx.set_contains_cbi` keyed by the CBI name
   (`html_cbi/build.lab:15-25`; `universal_cbi/build.lab:17-27`). Note `css_cbi` registers
   **two** CBI names ("css" and "styled") from one build script (`css_cbi/build.lab:21, 92`).
2. Create the job and order it before user code:
   `var cbi = ctx.build_cbi(&name); ctx.put_job_before(cbi, user_job)`
   (`html_cbi/build.lab:27-31`).
3. Build dependencies (cstd, std, compiler bindings, shared parsers) and add the module:
   `ctx.add_module(cbi, module)` (`universal_cbi/build.lab:92-96`).
4. Index each entrypoint with its `CBIFunctionType` via `ctx.index_def_cbi_fn(...)` or
   `ctx.index_cbi_fn(cbi, <name>, <entrypoint>, <type>)` (`html_cbi/build.lab:80-86`).

Real registrations:

- `html_cbi`: `html_initializeLexer`, `html_parseMacroValue`, `html_parseMacroNode`,
  `html_symResNode`, `html_symResValue`, `html_replacementNode`, `html_replacementValue`
  (`html_cbi/build.lab:80-86`).
- `css_cbi` (`css` name): same seven (`css_cbi/build.lab:83-89`); `styled` name:
  `styled_parseMacroNode` → `ParseMacroTopLevelNode`, `styled_symResDeclareNode` →
  `SymResDeclareTopLevelNode`, `styled_symResSigNode` → `SymResLinkSignatureNode`,
  `styled_symResNode`, `styled_replacementNodeDeclare`, `styled_replacementNode`
  (`css_cbi/build.lab:92-98`).
- `js_cbi`: seven `js_*` entries (`js_cbi/build.lab:91-97`).
- `universal_cbi`: `universal_parseMacroNode` → `ParseMacroTopLevelNode`,
  `universal_symResDeclareNode`, `universal_symResSigNode`, `universal_symResNode`,
  `universal_replacementNodeDeclare`, `universal_replacementNode`
  (`universal_cbi/build.lab:98-104`).
- `json_cbi`: same six top-level hooks (`json_cbi/build.lab:53-58`).
- `md_cbi`: seven `md_*` entries (`md_cbi/build.lab:73-79`).

`css_cbi`, `js_cbi`, `universal_cbi`, and `json_cbi` also create the marker annotation
`"component"` / `"serializable"` on the annotation controller
(`css_cbi/build.lab:29-30`, `js_cbi/build.lab:29-30`, `universal_cbi/build.lab:30-31`,
`json_cbi/build.lab:24-25`) so `#html` can recognise declarations as components.

### The hook lifecycle for a block macro (`#html`)

- **Lexer** — `html_initializeLexer` allocates an `HtmlLexer` and installs it via
  `lexer.setUserLexer(ptr, getNextToken ...)` (`html_cbi/src/main.ch:162-181`). `getNextToken`
  (`:105-160`) switches between HTML text/tags and Chemical `${...}`/`{...}` regions
  (`other_mode`, `chemical_mode`, `in_paren_expr`, `expecting_html_block`).
- **Parse** — `html_parseMacroNode` consumes `{`, calls `parseHtmlRoot(parser, builder)`,
  wraps the result in an `EmbeddedNode` with `builder.make_embedded_node(...)`, then consumes
  `}` (`html_cbi/src/main.ch:87-103`). The value form uses
  `builder.make_embedded_value(...)` (`:69-85`).
- **SymRes** — `html_symResNode` calls `visitor.visitEmbeddedNode(node)` then
  `sym_res_root(root, visitor, loc)` (`html_cbi/src/main.ch:1-8`); `sym_res_root`
  (`html_cbi/src/sym_res/sym_res_root.ch:1-342`) resolves every `HtmlPage` method and SSR
  helper and emits compile errors when one is missing (e.g. `append_html_char`,
  `truncate_html`, `get_html_size`).
- **Replacement** — `html_replacementNode` builds a `Scope`, runs `ASTConverter.convertHtmlRoot`
  into it, and returns it (`html_cbi/src/main.ch:10-27`). The value form returns a block value
  whose calculated value is the converter's string (`:47-67`).

`#universal` / `#styled` / `#json` use the *top-level* hook set instead: declare
(`..._symResDeclareNode`), link signature (`..._symResSigNode`), link body
(`..._symResNode`), replacement declare (`..._replacementNodeDeclare`), and replacement
(`..._replacementNode`). The replacement is split in two so the generated `FunctionDeclaration`
is visible to cross-module symbol resolution before its body is built.

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

> The conceptual `append_html_view(name)` above is shorthand. The real `HtmlPage`
> methods are `append_html(value : *char, len : size_t)` and
> `append_head(value, len)` (`lang/libs/page/src/page.ch:92-94, 128-130`). There is **no**
> `page.append_view`; the closest helpers are `append_head_view` and `append_css_view`
> (`page.ch:132-134, 164-166`). The 2c converter buffers static markup and flushes it through
> `emit_append_html_call` → `page.append_html(stringValue, size)`
> (`html_cbi/src/converter/language/main.ch:214-231, 233-241`).

Chemical values inside `{...}` become interleaved append calls:

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

The value path in source: inside `#html`, `{name}` is a `ChemicalAttributeValue`/child; the
converter calls `put_chemical_value_in` → `put_by_type` → either a primitive
`append_html_*` call or `value.writeToPageHtml(page, pageHtml)` for structs/aliases
(`html_cbi/src/converter/language/embedded_value.ch:109-172`; `put_by_node` at `:41-68`).

Key facts:

- `#html` does **NOT** auto-escape interpolated values — callers escape untrusted data with
  `escape_html` / `escape_html_view` (`lang/libs/page/src/page.ch:7-38`). Literal HTML text
  *is* escaped by the converter via `converter.escapeHtml` → `html_escape_append`
  (`html_cbi/src/converter/language/main.ch:208-210`; attribute literals at
  `attribute.ch:12-17`).
- An element opened in one `#html` block must be closed in the same block (no splitting).
- Inside JSX/HTML mode, `@{ ... }` escapes back into Chemical for statements (loops, ifs);
  use nested `#html { }` blocks inside the escape to emit elements. Increment loop indices
  before the nested block; each `@{}` must be self-contained. In `#html`, `{expr}` is a
  Chemical expression; in `#universal`, `{expr}` is JS and `${expr}` is Chemical.
- Conditionals/statements inside `@{}` are lowered to real AST if/else nodes by
  `convertHtmlChild` (`html_cbi/src/converter/language/main.ch:651-708`), which builds
  `builder.make_if_stmt(...)` and recurses into each branch's `converter.vec`.
- Universal components used inside `#html` are handled specially (see below), including
  `@{}` loop rendering of component lists.
- Static sibling text/attributes are HTML-entity-decoded before being re-emitted as client
  vnodes (`decode_html_entities`, `main.ch:75-95`) so server text and client vnodes agree.

## `#css` blocks (css_cbi)

`#css { ... }` parses CSS at compile time, injects the rules into `pageCss`, and evaluates to
the generated class name string (usable as a `class` value):

```chemical
func style_button(page : &mut HtmlPage) : *char {
    return #css {
        color : blue;
        padding : 8px;
        .my-global-button { color : red; }
    }
}
```

- Parse: `css_parseMacroValue` / `css_parseMacroNode` wrap `parseCSSOM` in an
  `EmbeddedValue`/`EmbeddedNode` (`css_cbi/src/main.ch:65-99`).
- Replacement value returns `root.className` as the block value
  (`css_cbi/src/main.ch:45-63`); the node form runs `ASTConverter.convertCSSOM`
  (`css_cbi/src/main.ch:10-25`).
- Emission: loop-free static CSS is hashed with `fnv1a_hash_32(str)`, guarded by
  `page.require_css_hash(hash)`, marked with `page.set_css_hash(hash)`, and emitted once via
  `page.append_css(...)` (`css_cbi/src/converter/string/main.ch:459-476`).
- Non-hashable CSS (dynamic `${}` values, media queries, nested rules, keyframes) uses the
  `require_random_css_hash`/`set_random_css_hash` path with a deterministic seed derived from
  the CSS content or the source location (`main.ch:378-441`, `cssom_stable_hash` at
  `:213-222`).
- Class-name format: `.` + prefix + 6 base64 chars + `{rules}`; prefix is `h` for the hashed
  path (`put_class_name`, `main.ch:182-186`; `allocate_view_with_classname`, `:188-196`) and
  `r` for the non-hashable path (`main.ch:413-419`). `om.className` is the 7-char name without
  the leading dot (`main.ch:418-419, 471`).

## `#styled` components (css_cbi, "styled" CBI)

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

Syntax variants (parsed in `css_cbi/src/styled.ch:52-166`):
- `#styled Name("div") { ... }` — tag as a string literal (`:69-80`).
- `#styled Name(.div) { ... }` — shorthand dot-tag form via the CSS lexer's `ClassName` token
  (`:94-97`).
- `#styled Wrap(Inner) { ... }` — wrap mode: forwards to the inner component `Inner`; css_cbi
  merges its generated class with `Inner`'s onto the rendered element (`:81-85`, `:274-307`,
  `:381-383`, `:390-396`). When `Inner` is a **universal** component,
  `signature.hydrateFunctionNode` points at it and `mountStrategy` becomes `Universal`, so
  `html_cbi`'s hydration dispatch targets the inner component (the styled wrapper is SSR-only).

Usage:

```chemical
#html {
    <Card class="xl">Hello <Title>World</Title></Card>
}
```

renders `<div class="hAz5DrX xl">Hello ...</div>` (base hash class first, user classes merged
after — `renderHtmlAttrsWithBase` in `lang/libs/page/src/ssr.ch:568-608`) and emits
`.hAz5DrX{...}` into the page CSS once (`require_css_hash`/`set_css_hash`).

Key properties:
- The generated class is a content hash of the CSS — stable/deterministic across renders.
- Works in `#html` blocks and is usable cross-module (import the declaring module). The
  `"component"` marker annotation makes it resolvable as `<Name>` in both `#html` and
  `#universal` (`styled.ch:157-163`).
- On the SSR side it is just a call to the generated server function with the attribute list
  and pre-rendered children (same `(page, attrs, children)` shape as universal servers).
- Generated body (`css_cbi/src/styled.ch:359-415`): emit CSS via `convertCSSOM`, then
  `page.append_html("<")`, `page.append_html(tag)`, `renderHtmlAttrsWithBase(page, attrs,
  baseClass)`, `page.append_html(">")`, `page.append_html(children.data, children.size)`,
  `page.append_html("</tag>")`. Wrap mode instead emits a single call to the inner component
  with `(page, attrs, children)`.

### Style generation / scoping summary

- Class name = deterministic FNV-1a hash of the serialized CSS, rendered through
  `base64_encode_32bit` (`css_cbi/src/converter/string/main.ch:176-196`).
- Scoping is by emitting selectors prefixed with the generated class: the root selector is
  `.` + className, nested rules are prefixed via `generate_css_recurse`
  (`main.ch:251-311`), and `&` is substituted with the parent selector.
- Dedup uses `doneClasses` (`require_css_hash`/`set_css_hash`) and, for dynamic CSS,
  `doneRandomClasses` (`require_random_css_hash`/`set_random_css_hash`) — both `unordered_map`
  fields on `HtmlPage` (`page.ch:63-70, 168-190`).
- Dynamic values inside CSS are emitted through `CssEmitter.emit_chemical_value` →
  `put_chemical_value_in`, which writes an `append_css_char_ptr` call
  (`css_cbi/src/converter/string/main.ch:25-29, 144-162`).

## `#js` blocks (js_cbi)

`#js { ... }` parses JavaScript (with JSX disabled) at compile time and inlines it into
`pageJs`. `${...}` inside the block inserts a Chemical value into the JS stream.

```chemical
#js {
    const btn = document.getElementById("clickable-btn")
    btn.onclick = () => { console.log("clicked") }
}
```

- Lexer: `js_initializeLexer` installs `getNextToken` which calls
  `nextJsToken(js, lexer, false)` (jsx disabled) (`js_cbi/src/main.ch:105-114`).
- Parse: `js_parseMacroValue`/`js_parseMacroNode` call `parseJsRoot` and wrap it
  (`js_cbi/src/main.ch:68-103`).
- Conversion: `JsConverter` implements `JsNodeEmitter` (`js_cbi/src/converter/converter.ch:244-268`);
  `emit_text` appends to its string buffer, `flush`/`put_chain_in` emits a
  `page.append_js(value, len)` call (`:139-165`), and `emit_chemical_value` →
  `put_chemical_value_in` → `put_by_type` emits `page.append_js_char_ptr` /
  `append_js_integer` / `append_js_uinteger` / `append_js_float` / `append_js_double` /
  `append_js_char` based on type (`:86-234`).
- Backtick template strings are emitted back with escaping; `escapeJs` escapes the backtick
  and non-ASCII (as `\u{...}`) (`js_cbi/src/converter/converter.ch:29-75`).
- `js_cbi` also uses the shared statement/expression converter
  `lang/libs/js_parser/src/converter/convert.ch` — the canonical JS printer.

## `#universal` components (universal_cbi)

```chemical
#universal Greeting(props) {
    return <div>Hello {props.name}</div>
}
```

The macro (`universal_cbi/src/react/macro.ch:16-124`) parses an optional typed/optional param
list (`#universal Card(props : title, onClick, isWide?)`) into a `JsComponentDecl` embedded node
and marks it with the `"component"` annotation (`:107-118`). `{ ... }` here is JS/JSX
(`jsParser.parseBlock`, `:99-101`); `${...}` is Chemical.

During symres (`universal_cbi/src/sym_res/sym_res.ch:7-73`) universal_cbi generates a **server
function**:

```chemical
func <module>_<Name>(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void
```

`universal_symResNode` resolves `HtmlPage` (`:15-18`), builds the void function and its three
parameters (`:24-51`), opens a scope, declares `page`, visits the body, and resolves child
components + required props (`:59-71`; prop/component validation in
`universal_cbi/src/sym_res/sym_res_root.ch:1-101`).

The server function body (`universal_cbi/src/react/ast_replace.ch:29-197`) does, in order:

1. `if(page.require_component(hash)) { page.set_component_hash(hash); ... }` — appends the
   **client component JS** `function <module>_<Name>(props) { ... }` to `pageJs` once per page
   (hash = the component function's encoded location; `selfHash` at `:74`). The JS block is
   hoisted above earlier dispatch lines with `page.get_js_pos()` / `page.move_js_range(...)` /
   `page.js_hoist_pos` (`:80-161`) so component functions are defined before use. JS emission
   itself is `append_universal_component_js` (`universal_cbi/src/react/emit_js.ch:1-27`).
2. Inside `if(page.render_js_only) { ... } else { ... }` (`:172-188`): the `else` branch emits
   SSR HTML to `pageHtml` by converting the component body with `target = BufferType.HTML`
   (`:177-186`). `render_js_only` prevents a nested component's subtree from being
   server-rendered twice (`page.ch:55-61`).
3. Non-SSRable attributes (event handlers, refs) are skipped in HTML and passed through the
   hydration props instead.

**SSR attribute/child model**: the server function receives `attrs : *SsrAttributeList` and
`children : SsrText`. Attributes are built as `SsrAttribute`/`SsrText` structs and rendered by
`renderHtmlAttrs`/`renderHtmlAttrsWithBase`, which merge `class`, join `style`, dedupe other
attrs last-wins, skip `false` booleans and `None`, and HTML-escape `Text`/`PtrChar`
(`lang/libs/page/src/ssr.ch:478-608`). Children are captured at the call site (see below).

When a `#universal` component is used inside `#html`, `html_cbi` renders:

```html
<span id="u{loc}" data-chx-i>...SSR HTML...</span>
```

plus a hydration trigger in `pageJs`:

```js
window.$__uni_dispatch('module_ComponentName', document.getElementById('u{loc}'), {props})
```

`data-chx-i` spans are `display:contents` (added by `page.defaultUniversalSetup()`,
`page.ch:457-461`) so the boundary never affects layout. In table structure elements
(`table`/`thead`/`tbody`/`tr`/…), a comment boundary `<!--u{loc}-->` is used instead and the
dispatch calls `window.$__uni_boundary(...)` with mode `"root"` because a `<span>` would be
foster-parented (`html_cbi/src/converter/language/component.ch:290-332`;
`main.ch:101-109, 424-547`). `$__uni_dispatch` mounts immediately if the component function
exists, otherwise queues it (`window.$__uni_hydration_queue`) until
`window.$__universal_flush()` at the end of the bundle (`page.ch:466-520`).

**Nested components** (inside a `#universal` body) emit `$_uc_c(ComponentFn, props)` and hydrate
the server-rendered DOM in place — the client component vnode references the function directly
(`universal_cbi/src/converter/converter_jsx.ch:235-302`). The legacy
`capture_html_delta_to_js` / `$_uc_h(html, {name, props})` path was removed (see the `universal`
skill); do not emit it from new code.

## `#json(Type)` (json_cbi)

`#json(Type)` is a top-level macro that auto-generates `std::Serializer` and
`std::Deserializer` impls for `Type` by inspecting its fields.

```chemical
#json(User)

public struct User {
    var name : std::string
    var age : int
}
```

- Parse: `json_parseMacroTopLevelNode` reads `#json` `(`, a namespaced type path
  (`a::b::Type`), and `)`; stores it in a `SerializableInfo` on the AST arena and wraps it in a
  top-level `EmbeddedNode` (`json_cbi/src/main.ch:3-80`).
- SymRes: `resolve_types` resolves `std::Serializer`, `std::Deserializer`, `std::Result`,
  `std::Result.Ok/Err`, `std::Unit`, `std::SerializationError`
  (`json_cbi/src/main.ch:96-120`).
- Replacement: `build_serialize_fn` generates
  `func serialize(&self, encoder : &JsonEncoder) : Result<Unit, SerializationError>` and
  `build_deserialize_fn` generates the counterpart
  (`json_cbi/src/main.ch:335-501, 503-…`). Field types with no serializer/deserializer are
  rejected at the `#json` site with a diagnostic; variant-typed fields are explicitly
  unsupported (`main.ch:241-289`).
- The generated impls plug into the `json` runtime (`JsonEncoder`/`JsonDecoder`,
  `TypeDecoder<T>`); see the `json_serialization` skill for the full pipeline.

## `#md` (md_cbi)

`#md { ... }` (terminated by `#endmd`, no closing brace) parses Markdown at compile time and
emits HTML into `pageHtml`.

- Parse: `md_parseMacroNode`/`md_parseMacroValue` call `parseMdRoot`; the value form consumes
  the `#endmd` token (`md_cbi/src/main.ch:15-37`).
- Conversion: `MdConverter` implements `MdEmitter`; `flush` emits
  `page.append_html(value, len)`, and `emit_interpolation` maps the value's type to
  `append_html_integer`/`uinteger`/`float`/`double`/`char_ptr`
  (`md_cbi/src/converter/converter.ch:11-45, 50-72`).

## Generated-code model in detail

### `page.*` runtime API emitted by the converters

- **HTML / head**: `append_html(value : *char, len)` and `append_head(value, len)` — real
  methods, there is **no** `page.append_view` or `append_html_view`
  (`page.ch:92-94, 128-130`). `#universal`/`#styled` render SSR markup with the `append_html_*`
  family (flushed by `emit_append_html_call`, `main.ch:214-241`).
- **CSS**: `append_css`, `append_css_view`, `append_css_char_ptr`, `append_css_*`
  (`page.ch:160-214`); guarded by `require_css_hash`/`set_css_hash` and
  `require_random_css_hash`/`set_random_css_hash` (`page.ch:168-190`).
- **JS**: `append_js`, `append_js_char_ptr`, `append_js_escaped`, `append_js_*`
  (`page.ch:216-255`).
- **Buffer surgery**: `get_html_size()` / `truncate_html(n)` (`page.ch:120-126`) and
  `get_js_pos()` / `move_js_range(a,b,i)` (memmove hoist, `page.ch:257-…`).
- **Component dedup**: `require_component(hash)` / `set_component_hash(hash)`
  (`page.ch:176-182`) and the `render_js_only` field (`page.ch:55-61`).

### `capture_html_delta_to_js`, `truncate_html`, `get_html_size`

The **current** child-capture protocol is `get_html_size` + `truncate_html`:

1. Record `startIdx = page.get_html_size()`.
2. Render the children (they append to `pageHtml`).
3. Copy the new slice out with
   `page.pageHtml.data + startIdx` / `page.pageHtml.size - startIdx` into a `std::string` via
   `append_with_len`, then `page.truncate_html(startIdx)` to remove it from the page buffer.
4. Wrap the slice as an `SsrText { data, size }` and pass it as the 3rd argument to the child
   server function.

Source: `html_cbi/src/converter/language/component.ch:133-235` (`build_ssr_children`) and the
mirrored universal path in `universal_cbi/src/converter/converter_jsx.ch:180-232`. This is used
for `#styled`/`#universal` children, which are pre-rendered before the parent's HTML is emitted.

> **Legacy note (preserved for traceability).** Earlier builds used a helper named
> `capture_html_delta_to_js` (and `move_html_to_js_with_lambda_start`) that moved the SSR HTML
> delta into the JS bundle (paired with `$_uc_h(html, name, props)`). That mechanism was
> **removed**; the live path is `get_html_size`/`truncate_html` for SSR children and `$_uc_c`
> for nested client vnodes. `truncate_html` still exists and is tested in
> `lang/tests/compiler_plugins/universal/src/page_buffer_api.ch:297-322`.

### SSR function generation & hydration dispatch

- **SSR function**: one per `#universal`/`#styled` component, shaped
  `(page : &mut HtmlPage, attrs : *SsrAttributeList, children : SsrText) : void`
  (`sym_res.ch:24-51`; `styled.ch:195-225`).
- **Hydration boundary**: `#html` wraps the SSR call for a universal component in
  `<span id="u{loc}" data-chx-i>…</span>` (`component.ch:320-332`) or a comment boundary in
  table context (`:311-318`). The `loc` comes from `element.loc`.
- **Dispatch**: `window.$__uni_dispatch('<scopedName>', <target>, {props} [, "root"])` is built
  in `emit_universal_queue` (`html_cbi/src/converter/language/main.ch:424-547`). `target` is
  `document.getElementById('u{loc}')` or `window.$__uni_boundary('u{loc}')`; `"root"` mode is
  used for the comment boundary.
- **Scoped component names**: `get_module_scoped_name(functionNode, name, out)` prefixes
  `<scope>_<module>_` when the declaration has a module scope
  (`lang/libs/html_comp/ast.ch:33-47`). So `<Greeting>` in module `app` dispatches
  `'app_Greeting'` (with an extra `scope_` prefix for nested scopes). The same helper is used
  for client vnodes and the `function <name>(props)` definition, so the two always agree
  (`html_cbi/src/converter/language/main.ch:376, 439`;
  `universal_cbi/src/react/emit_js.ch:9-14`).
- **Mount modes**: default `"children"` hydrates into the target's children; `"root"` treats
  the target element itself as the SSR root (`page.ch:475-499`).
- **Client vnodes**: static children are emitted as `$_ur.createElement(tag, props, ...children)`
  or `$_uc_c(ComponentFn, props)` when the child is itself a component
  (`html_cbi/src/converter/language/main.ch:354-422`); dynamic children fall back to a
  `window.$__uni_html("<html>", count)` blob (`:507-537`). The runtime vnode factory is
  `window.$_ur.createElement` built by `defaultUniversalSetup` (`page.ch:543-546`).

### Buffer → output files

`HtmlPage` fields map to output assets as documented in the `universal` skill. The finalization
entry points are `getFinalizedPageJs` = `pageJs + pageJsEnd` (`page.ch:2196-2202`);
`htmlPageToString`, which references assets via `<link>`/`<script src>` (`page.ch:2205-2232`);
and `writeToDirectory`, which writes `.html`, `.css`, `_head.js`, `.js`
(`page.ch:2240-2281`).

## ASTBuilder API calls the plugins use

Plugins obtain a builder from the hook (`parser`/`builder` args, or
`visitor.getSymbolResolver().getJobBuilder()`), then construct nodes. Verified call sites:

**Values / identifiers**
- `builder.make_identifier(name, linkedNode, isMut, loc)` — `component.ch:9-11`;
  `ast_replace.ch:9`; `js_embedded_value.ch:7`.
- `builder.make_access_chain(std::span<*mut Value>([base, member]), loc)` — `component.ch:11`;
  `ast_replace.ch:11`.
- `builder.make_function_call_value(chain, loc)` /
  `builder.make_function_call_node(chain, parent, loc)` — `component.ch:256`;
  `js_embedded_value.ch:10`.
- `builder.make_string_value(&view, loc)`, `make_ubigint_value`, `make_char_value`,
  `make_bool_value`, `make_null_value`, `make_int_value` — `main.ch:85-87`;
  `component.ch:44, 78`; `js_embedded_value.ch:98`.
- `builder.make_expression_value(lhs, rhs, Operation.Addition, type, loc)` — `ast_replace.ch:128, 137`.
- `builder.make_struct_value(linkedNode, loc)` + `.add_value(name, val)` — `component.ch:37, 43`.
- `builder.make_array_value(elemType, loc)` + `.get_values()` — `component.ch:49-50`.
- `builder.make_addr_of_value(value, isMut, loc)` — `component.ch:258`.
- `builder.make_dereference_value(value, type, loc)` — `styled.ch:399`.

**Statements / declarations**
- `builder.make_varinit_stmt(isConst, isUnsafe, &name, type, initializer, spec, parent, loc)` —
  `ast_replace.ch:93`.
- `builder.make_assignment_stmt(lhs, rhs, Operation.Assignment, parent, loc)` —
  `ast_replace.ch:161`.
- `builder.make_if_stmt(cond, parent, loc)` + `.get_body()` / `.add_else_body()` /
  `.add_else_if(cond)` — `ast_replace.ch:102`; `styled.ch:407`.
- `builder.make_return_stmt(value, parent, loc)`, `make_scope(parent, loc)` + `.getNodes()`,
  `make_function(name, retType, isVariadic, parent, loc)` + `.add_body()` + `.get_params()` —
  `main.ch:14-25`; `sym_res.ch:25-53`.
- `builder.make_function_param(name, type, index, defaultVal, isSelf, func, loc)` —
  `sym_res.ch:34, 45, 49`.
- `builder.make_value_wrapper(value, parent)`, `make_block_value(parent, loc)` +
  `setCalculatedValue(v)` — `main.ch:51-66`.
- `builder.make_top_level_embedded_node(spec, key, dataPtr, knownTypeFn, childResFn,
  crossModProxyFn, nodes, values, parent, loc)` — `universal_cbi/src/react/macro.ch:105`;
  `css_cbi/src/styled.ch:144`; `json_cbi/src/main.ch:66`.
- `builder.make_embedded_node(...)` / `make_embedded_value(...)` — `html_cbi/src/main.ch:76, 94`.

**Types / allocations**
- `builder.make_linked_type(name, linkedNode, loc)`, `make_ptr_type(child, isMut, loc)`,
  `make_reference_type(child, isMut, loc)`, `make_void_type(loc)`,
  `builder.get_int_type()`, `builder.get_u64_type()`, `builder.get_bool_type()` —
  `sym_res.ch:24-49`; `styled.ch:196-220`; `ast_replace.ch:93`.
- `builder.allocate<T>()` + placement `new(ptr) T{...}` — `macro.ch:74-92`;
  `styled.ch:114-141`.
- `builder.allocate_view(&string)` / `builder.allocate_view(view)`,
  `builder.allocate_str_size(n)`, `builder.allocate_str(data, len)` —
  `macro.ch:24`; `css_cbi/src/converter/string/main.ch:188-196`.

The full API is documented in `compiler_api`; the hook typedefs and registration are in
`cbi_plugin_api`.

## Prop serialization and escaping rules

There are three distinct escaping contexts; each has one canonical helper — extend those, do
not hand-escape at call sites:

1. **HTML text / attributes** — HTML-escaped `& < > " '` by `appendHtmlEscaped`
   and `escape_html`/`escape_html_view` (`lang/libs/page/src/page.ch:7-38`;
   `ssr.ch:361-363`). `renderHtmlAttrs` HTML-escapes `Text`/`PtrChar` values.
2. **JS string literals** — `appendJsEscaped` quotes `"`, escapes `\`, `\n\r\t`, control chars,
   and rewrites `</` to `\u003C/` so inline `<script>` cannot be broken out of
   (`page.ch:224-235`; `ssr.ch:362-364`). The html converter's `append_js_string_literal`
   implements the same rules (`html_cbi/src/converter/language/main.ch:337-352`), and
   `append_escaped_single_quoted` escapes `\ ' \n \r \t` for single-quoted contexts
   (`main.ch:119-131`).
3. **Single-quoted JS props** — when a value is embedded between single quotes, escape `\` and
   `'` (and control chars). Generated dispatches today embed string props in **double** quotes
   (`emit_universal_queue`, `main.ch:464-478`, and the `#html` script path at
   `component.ch:369-382`), so they go through `put_js_value_in` →
   `append_js_escaped_char_ptr` (`js_embedded_value.ch:139-149`). The documented
   `js_string_escape()` workaround (double the backslash, `'` → `\u0027`) still applies to any
   consumer that wraps props in single quotes (e.g. hand-written serializers in app code; it is
   not a repo function).

**User structs as props:** define `writeToPageHtml` for HTML and `writeToPageJs` for the JS
bundle (`html_cbi/src/converter/language/embedded_value.ch:41-68`;
`js_embedded_value.ch:86-114`). Without them the converter now reports a **compile diagnostic**
at the macro location (`unsupported_value`/`js_unsupported`, `embedded_value.ch:27-39`;
`js_embedded_value.ch:65-79`) instead of silently writing an error string. On the TCC (2c)
backend the diagnostic is printed but non-fatal; the LLVM backend fails.

## Gotchas

- **Cannot split an element across `#html` blocks.** An element opened in one block must be
  closed in that block; use `@{}` + nested `#html {}` for dynamic structure
  (`design_web_app` §"Cannot split #html").
- **`@{}` ordering.** Inside `@{}`, increment the loop index *before* the nested `#html {}` so
  the captured value matches the iteration; each `@{}` must be self-contained.
- **Vector `.get_ptr(i)` not `.get(i)`.** For `vector<T>` where `T` has a destructor, `.get(i)`
  copies internals and double-frees; always use `.get_ptr(i)` (AGENTS.md §"Vector Access").
- **Prop types.** Never pass a C++ struct with `vector<>` fields as a component prop; pre-
  serialize to JSON and post-process with the `js_string_escape` pattern. `bool` props that are
  `false` and `None` are skipped by `accumulateAttrs` (`ssr.ch:488-508`).
- **Escaping is explicit for interpolated HTML.** `#html` does not auto-escape `{value}`
  (`page.ch:2-6`); call `escape_html`/`escape_html_view`.
- **Table contexts.** A `<span data-chx-i>` inside `<tr>`/`<table>` is foster-parented; the
  converter switches to a comment boundary + `"root"` hydration mode
  (`component.ch:290-332`; `main.ch:101-109`).
- **Plugin recompilation.** After editing a plugin's `.ch`, rebuild with plugins enabled;
  `--cached-plugins` (or `--cache`) may otherwise reuse the stale JIT module. Use
  `--frecompile-plugins` when iterating on a macro implementation.
- **Enum sync.** Adding a value to a CBI-exposed enum without mirroring it in the
  `lang/libs/compiler/src/*.ch` binding shifts every later value and crashes all plugins —
  see `cbi_plugin_api` §"Enum Sync Rule".
- **`render_js_only`.** When emitting child client JS, the converter sets
  `page.render_js_only = true` around the child server call and restores it afterwards; getting
  this wrong double-renders SSR subtrees (`converter_jsx.ch:255-283`; `page.ch:55-61`).

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
> re-parsed before shipping; unsupported prop types were silently `UInteger` (now a diagnostic);
> layout effects were registered but never executed (now fixed); per-page runtime duplication
> (~32 KB inline per page); legacy `capture_html_delta_to_js` did not escape `</script>`
> (removed / fixed). Contract tests pin these in
> `lang/tests/compiler_plugins/universal/src/runtime_contracts.ch`.

For developing new compiler plugins or the plugin API itself, load the `cbi_plugin_api`
skill. For compile-time intrinsics/reflection used by macros, load
`intrinsics_compiler_reflection`. For the full universal SSR/hydration model, load `universal`.
For the AST node hierarchy used by `ASTBuilder`, load `compiler_api`. For authoring apps with
these macros, load `design_web_app`.
