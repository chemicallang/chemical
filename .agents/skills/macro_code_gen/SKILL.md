---
name: Macro Code gen
description:
    How libraries (html_cbi, preact_cbi, react_cbi, solid_cbi, universal_cbi) generate code
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

`#preact`, `#react`, `#solid`, `#universal` All these component framework libraries are top level macros. The reason being
that these handle components and that's why.

for example

```chemical
#preact Greeting(props) {
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
universal components are fast and they render everywhere, they work in react, preact and solid components. They work in `#html` too.

For more information on universal components, load the `universal` skill. For developing new compiler plugins or understanding the plugin API, load the `cbi_plugin_api` skill. For understanding the compiler intrinsics and reflection APIs that macros can use at compile time, load the `intrinsics_compiler_reflection` skill.