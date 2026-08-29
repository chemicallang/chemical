---
name: Macro Code gen
description:
    How libraries (html_cbi, universal_cbi) generate code
---

When you write in chemical

```chemical
func my_html(page : &mut HtmlPage) {
    #html {
        <span>Hello World</span>
    }
}
```

Whats actually happening is, behind the scenes this code gets generated

```chemical
func my_html(page : &mut HtmlPage) {
    page.append_html_view("<span>Hello World</span>");
}
```

Its not the exact code, but its pretty close, The difference happens when you use a chemical value, for example
This code

```chemical
func my_html(page : &mut HtmlPage, name : &std::string_view) {
    #html {
        <span>Hello {name}</span>
    }
}
```

generates

```chemical
func my_html(page : &mut HtmlPage, name : &std::string_view) {
    page.append_html_view("<span>Hello ");
    // this method basically calls append_html
    // it exists in lang/libs/page/src/PageWriter.ch
    name.writeToPageBody(page)
    page.append_html_view("</span>");
}
```

This is how we handle chemical values, But you may notice that we support top level macros

`#universal` is the component macro. It handles SSR + hydration for server-rendered interactive UI.

For example:

```chemical
#universal Greeting(props) {
    return <div>Hello {props.name}</div>
}
```

It doesn't have a encapsulating function, so it generates one, it generates code like

```chemical
func Greeting(page : &mut HtmlPage) {
    if(!page.requires_component(123)) {
        return;
    }
    page.set_has_component(123)
    page.append_head_js_view("function Greeting_component(props) { ... }")
}
```

The function names maybe a little different, but they accomplish similar logic, however `universal` is different

universal lib generate a function that takes two more parameters, an SsrAttributeList, content for the children
universal lib also generates in both bundles, HTML that is server side rendered (from the component) and then a js function
that would hydrate the emitted html.
universal components are fast and they render everywhere. They work in `#html` blocks too.

`#styled` is the styling macro (provided by `css_cbi`). It declares a reusable, styled component whose CSS is scoped to a compiler-generated hash class and injected into the page automatically — no manual `#css` + `class={...}` wiring needed.

For example:

```chemical
#styled Card("div") {
    background: #ffffff;
    border: 1px solid #cccccc;
    padding: 8px;
}
```

generates a component `Card` that, when used in `#html`:

```chemical
#html {
    <Card class="xl">Hello <Title>World</Title></Card>
}
```

renders `<div class="hAz5DrX xl">Hello ...</div>` and emits `.hAz5DrX{background:#ffffff;border:1px solid #cccccc;padding:8px;}` into the page's CSS.

Syntax variants:
- `#styled Name("div") { ... }` — tag given as a string literal.
- `#styled Name(.div) { ... }` — shorthand using the dot-tag form.
- `#styled Wrap(Inner) { ... }` — wrap mode: `Wrap` forwards to the inner component `Inner` and merges its own generated class with `Inner`'s onto the rendered element, so both components' CSS apply.

Key properties:
- The generated class is a content hash of the CSS, so it is stable/deterministic across renders.
- User-provided `class` attributes are merged alongside the generated hash class.
- Works in `#html` blocks and is usable cross-module (import the module that declares it).
- Unlike `#universal`, `#styled` components do no hydration — they are pure SSR + injected CSS.

For more information on universal components, load the `universal` skill. For developing new compiler plugins or understanding the plugin API, load the `cbi_plugin_api` skill. For understanding the compiler intrinsics and reflection APIs that macros can use at compile time, load the `intrinsics_compiler_reflection` skill.