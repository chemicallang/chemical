// Regression tests for SSR serialization safety:
// 1. Unresolvable attribute values (SsrAttributeValue.None) must be skipped, not
//    rendered as literal "null" (previously produced style="null").
// 2. Dynamic attribute values must be HTML-escaped in the HTML target and
//    JS-escaped in the JS target.
using namespace std;

func make_ssr_text_ut(str : &std::string_view) : SsrText {
    return SsrText {
        data : str.data(),
        size : str.size()
    }
}

@test
public func universal_render_html_attrs_skips_none(env : &mut TestEnv) {
    var pg = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("class")),
        value : SsrAttributeValue.None()
    })
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("style")),
        value : SsrAttributeValue.None()
    })
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("title")),
        value : SsrAttributeValue.None()
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut pg, &list)
    var html = pg.getHtml()
    if(!html.contains("null")) {
        env.success("None attribute values are skipped in HTML")
    } else {
        env.error("None attribute values rendered as literal null")
        env.info(html.data())
    }
}

@test
public func universal_render_js_attrs_skips_none(env : &mut TestEnv) {
    var pg = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("style")),
        value : SsrAttributeValue.None()
    })
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("class")),
        value : SsrAttributeValue.None()
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut pg, &list)
    var js = pg.getJs()
    if(!js.contains("null")) {
        env.success("None attribute values are skipped in JS")
    } else {
        env.error("None attribute values rendered as literal null in JS")
        env.info(js.data())
    }
}

@test
public func universal_render_html_attrs_escapes_values(env : &mut TestEnv) {
    var pg = HtmlPage()
    var hostile = std::string_view("\"><img src=x onerror=alert(1)>")
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("title")),
        value : SsrAttributeValue.Text(make_ssr_text_ut(&hostile))
    })
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("data-ptr")),
        value : SsrAttributeValue.PtrChar("\"><script>alert(1)</script>")
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut pg, &list)
    var html = pg.getHtml()
    if(html.contains("&lt;") && html.contains("&quot;") && html.contains("&gt;") && !html.contains("<img")) {
        env.success("HTML attribute values are escaped")
    } else {
        env.error("HTML attribute values are not escaped")
        env.info(html.data())
    }
}

#universal SpreadPropsComp(props) {
    return <div {...props}>x</div>
}

@test
public func universal_spread_props_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadPropsComp a="1" b="two" /> }
    var html = page.getHtml()
    if(html.contains("a=\"1\"") && html.contains("b=\"two\"")) {
        env.success("spreading real props works in SSR")
    } else {
        env.error("spreading props failed in SSR")
        env.info(html.data())
    }
}

#universal SpreadObjectLiteralComp(props) {
    return <div {...{a: 1, b: "x"}}>y</div>
}

@test
public func universal_spread_object_literal_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadObjectLiteralComp /> }
    var html = page.getHtml()
    if(html.contains("a=\"1\"") && html.contains("b=\"x\"")) {
        env.success("spreading an object literal enumerates its members in SSR")
    } else {
        env.error("spreading an object literal failed in SSR")
        env.info(html.data())
    }
}

#universal ChemStringPropComp(props) {
    return <div title={props.title}>x</div>
}

@test
public func universal_chem_string_prop_js_safe(env : &mut TestEnv) {
    var page = HtmlPage()
    var title = "say \"hi\" \\ path"
    #html { <ChemStringPropComp title={title} /> }
    var js = page.getJs()
    // The generated JS must escape the embedded quotes and backslash so the
    // dispatch props object stays a valid JS string literal.
    if(js.contains("say \\\"hi\\\"") && js.contains("\\\\ path")) {
        env.success("Chemical string prop is escaped for the JS bundle")
    } else {
        env.error("Chemical string prop is not escaped for the JS bundle")
        env.info(js.data())
    }
}

// A spread of a dynamic value that cannot be SSR-resolved must be skipped entirely
// (not silently spread the component's own props and not render style="null").
#universal SpreadDynamicComp(props) {
    function get_extra_attrs() {
        return {}
    }
    var extra = get_extra_attrs()
    return <div {...extra}>x</div>
}

@test
public func universal_spread_dynamic_skipped_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SpreadDynamicComp /> }
    var html = page.getHtml()
    if(!html.contains("null")) {
        env.success("dynamic spread is skipped in SSR without degrading")
    } else {
        env.error("dynamic spread degraded into null in SSR")
        env.info(html.data())
    }
}

@test
public func universal_render_js_attrs_escapes_values(env : &mut TestEnv) {
    var pg = HtmlPage()
    var hostile = std::string_view("a\"b\\c\nd")
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("title")),
        value : SsrAttributeValue.Text(make_ssr_text_ut(&hostile))
    })
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("data-x")),
        value : SsrAttributeValue.Text(make_ssr_text_ut(&std::string_view("</script>")))
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut pg, &list)
    var js = pg.getJs()
    if(js.contains("\\\"") && js.contains("\\\\") && js.contains("\\n") && js.contains("\\u003C/")) {
        env.success("JS attribute values are escaped")
    } else {
        env.error("JS attribute values are not escaped")
        env.info(js.data())
    }
}
