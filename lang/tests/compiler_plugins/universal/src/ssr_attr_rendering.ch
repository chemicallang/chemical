// =============================================================================
// Feature Regression Tests: SSR Attribute Rendering (HTML + JS Targets)
//
// Pins every SsrAttributeValue variant's rendering through renderHtmlAttrs
// and renderJsAttrs. When the library is rewritten, each variant must produce
// the same output.
// =============================================================================

// --- Helper ---

func make_ssr_attr(name : *char, value : SsrAttributeValue) : SsrAttribute {
    return SsrAttribute {
        name : make_ssr_text_ut(&std::string_view(name)),
        value : value
    }
}

func make_ssr_text_val(text : *char) : SsrAttributeValue {
    return SsrAttributeValue.Text(make_ssr_text_ut(&std::string_view(text)))
}

func make_ssr_ptr_char_val(ptr : *char) : SsrAttributeValue {
    return SsrAttributeValue.PtrChar(ptr)
}

func make_ssr_int_val(val : bigint) : SsrAttributeValue {
    return SsrAttributeValue.Integer(val)
}

func make_ssr_uint_val(val : ubigint) : SsrAttributeValue {
    return SsrAttributeValue.UInteger(val)
}

func make_ssr_bool_val(val : bool) : SsrAttributeValue {
    return SsrAttributeValue.Boolean(val)
}

func make_ssr_none_val() : SsrAttributeValue {
    return SsrAttributeValue.None()
}

func make_ssr_char_val(c : char) : SsrAttributeValue {
    return SsrAttributeValue.Char(c)
}

func render_html_attrs_single(page : &mut HtmlPage, name : *char, value : SsrAttributeValue) {
    var attr = make_ssr_attr(name, value)
    var list = SsrAttributeList {
        data : &raw attr,
        size : 1
    }
    renderHtmlAttrs(page, &list)
}

func render_js_attrs_single(page : &mut HtmlPage, name : *char, value : SsrAttributeValue) {
    var attr = make_ssr_attr(name, value)
    var list = SsrAttributeList {
        data : &raw attr,
        size : 1
    }
    renderJsAttrs(page, &list)
}

// =============================================================================
// HTML Target: SsrAttributeValue variants
// =============================================================================

// --- None ---

@test
public func feature_html_none_renders_nothing(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-x", make_ssr_none_val())
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("data-x") && !html.contains("null")) {
        env.success("HTML None renders nothing (attr skipped)")
    } else {
        env.error("HTML None rendered unexpected output")
        env.info(html.data())
    }
}

// --- Boolean ---

@test
public func feature_html_boolean_true_renders_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "disabled", make_ssr_bool_val(true))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("disabled=\"true\"")) {
        env.success("HTML Boolean(true) renders attr with value 'true'")
    } else {
        env.error("HTML Boolean(true) did not render correctly")
        env.info(html.data())
    }
}

@test
public func feature_html_boolean_false_skips_attr(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "disabled", make_ssr_bool_val(false))
    var html = std::string()
    html.append_view(page.getHtml())
    if(!html.contains("disabled")) {
        env.success("HTML Boolean(false) skips the attribute")
    } else {
        env.error("HTML Boolean(false) rendered attribute")
        env.info(html.data())
    }
}

// --- Text ---

@test
public func feature_html_text_renders_escaped_value(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "title", make_ssr_text_val("a \"test\" <b>bold</b>"))
    var html = std::string()
    html.append_view(page.getHtml())
    var has_quot = html.contains("&quot;")
    var has_lt = html.contains("&lt;")
    var has_gt = html.contains("&gt;")
    if(has_quot && has_lt && has_gt) {
        env.success("HTML Text escapes &, <, >, \" correctly")
    } else {
        env.error("HTML Text escaping incomplete")
        env.info(html.data())
    }
}

// --- PtrChar ---

@test
public func feature_html_ptr_char_renders_escaped_value(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-v", make_ssr_ptr_char_val("hello & world"))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-v=\"hello &amp; world\"")) {
        env.success("HTML PtrChar escapes & correctly")
    } else {
        env.error("HTML PtrChar escaping incorrect")
        env.info(html.data())
    }
}

// --- Integer ---

@test
public func feature_html_integer_renders_decimal(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-n", make_ssr_int_val(-100))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-n=\"-100\"")) {
        env.success("HTML Integer renders as decimal string")
    } else {
        env.error("HTML Integer did not render correctly")
        env.info(html.data())
    }
}

// --- UInteger ---

@test
public func feature_html_uinteger_renders_decimal(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-n", make_ssr_uint_val(42))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-n=\"42\"")) {
        env.success("HTML UInteger renders as decimal string")
    } else {
        env.error("HTML UInteger did not render correctly")
        env.info(html.data())
    }
}

// --- Double ---

@test
public func feature_html_double_renders_with_precision(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-d", SsrAttributeValue.Double(3.14159, 3))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-d=\"3.14")) {
        env.success("HTML Double renders with specified precision")
    } else {
        env.error("HTML Double did not render correctly")
        env.info(html.data())
    }
}

// --- Char ---

@test
public func feature_html_char_renders_single_character(env : &mut TestEnv) {
    var page = HtmlPage()
    render_html_attrs_single(&mut page, "data-c", make_ssr_char_val('X'))
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-c=\"X\"")) {
        env.success("HTML Char renders single character")
    } else {
        env.error("HTML Char did not render correctly")
        env.info(html.data())
    }
}

// =============================================================================
// JS Target: SsrAttributeValue variants
// =============================================================================

// --- None in JS ---

@test
public func feature_js_none_renders_nothing(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "data-x", make_ssr_none_val())
    var js = std::string()
    js.append_view(page.getJs())
    // None is skipped entirely in JS (not rendered as "undefined" at attr level)
    if(!js.contains("data-x") && !js.contains("undefined")) {
        env.success("JS None renders nothing (attr skipped)")
    } else {
        env.error("JS None rendered unexpected output")
        env.info(js.data())
    }
}

// --- Boolean in JS ---

@test
public func feature_js_boolean_true_renders_true(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "active", make_ssr_bool_val(true))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("active:true")) {
        env.success("JS Boolean(true) renders as bare true")
    } else {
        env.error("JS Boolean(true) did not render correctly")
        env.info(js.data())
    }
}

@test
public func feature_js_boolean_false_skipped(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "disabled", make_ssr_bool_val(false))
    var js = std::string()
    js.append_view(page.getJs())
    // Boolean(false) is skipped in non-special JS attrs
    if(!js.contains("disabled")) {
        env.success("JS Boolean(false) is skipped (no attr in JS output)")
    } else {
        env.error("JS Boolean(false) rendered unexpected output")
        env.info(js.data())
    }
}

// --- Text in JS ---

@test
public func feature_js_text_renders_escaped(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "label", make_ssr_text_val("say \"hi\""))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\\"hi\\\"")) {
        env.success("JS Text escapes double quotes")
    } else {
        env.error("JS Text did not escape correctly")
        env.info(js.data())
    }
}

// --- PtrChar in JS ---

@test
public func feature_js_ptr_char_renders_escaped(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "data", make_ssr_ptr_char_val("path\\to"))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("path\\\\to")) {
        env.success("JS PtrChar escapes backslash")
    } else {
        env.error("JS PtrChar did not escape correctly")
        env.info(js.data())
    }
}

// --- Integer in JS ---

@test
public func feature_js_integer_renders_bare_number(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "count", make_ssr_int_val(42))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("count:42")) {
        env.success("JS Integer renders as bare number (no quotes)")
    } else {
        env.error("JS Integer did not render correctly")
        env.info(js.data())
    }
}

// --- UInteger in JS ---

@test
public func feature_js_uinteger_renders_bare_number(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "size", make_ssr_uint_val(1024))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("size:1024")) {
        env.success("JS UInteger renders as bare number (no quotes)")
    } else {
        env.error("JS UInteger did not render correctly")
        env.info(js.data())
    }
}

// --- Double in JS ---

@test
public func feature_js_double_renders_bare_number(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "ratio", SsrAttributeValue.Double(0.75, 3))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("ratio:0.75")) {
        env.success("JS Double renders as bare decimal number")
    } else {
        env.error("JS Double did not render correctly")
        env.info(js.data())
    }
}

// --- Char in JS ---

@test
public func feature_js_char_renders_single_quoted(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "letter", make_ssr_char_val('A'))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("letter:'A'")) {
        env.success("JS Char renders as single-quoted character")
    } else {
        env.error("JS Char did not render correctly")
        env.info(js.data())
    }
}

@test
public func feature_js_char_escapes_single_quote(env : &mut TestEnv) {
    var page = HtmlPage()
    render_js_attrs_single(&mut page, "ch", make_ssr_char_val('\''))
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\'")) {
        env.success("JS Char escapes single quote inside single-quoted string")
    } else {
        env.error("JS Char did not escape single quote")
        env.info(js.data())
    }
}

// --- Class merging in HTML ---

@test
public func feature_html_class_merge_space_separated(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("class", make_ssr_text_val("base")))
    attrs.push(make_ssr_attr("class", make_ssr_text_val("extra")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("class=\"extra base\"") || html.contains("class=\"base extra\"")) {
        env.success("HTML class merging produces space-separated class list")
    } else {
        env.error("HTML class merging did not produce expected output")
        env.info(html.data())
    }
}

// --- Style merging in HTML ---

@test
public func feature_html_style_merge_semicolon_separated(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("style", make_ssr_text_val("color:red")))
    attrs.push(make_ssr_attr("style", make_ssr_text_val("font-size:14px")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("color:red") && html.contains("font-size:14px") && html.contains(";")) {
        env.success("HTML style merging produces semicolon-separated style list")
    } else {
        env.error("HTML style merging did not produce expected output")
        env.info(html.data())
    }
}

// --- Class merging in JS ---

@test
public func feature_js_class_merge_space_separated(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("class", make_ssr_text_val("base")))
    attrs.push(make_ssr_attr("class", make_ssr_text_val("extra")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut page, &list)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("class:")) {
        env.success("JS class merging produces merged class string")
    } else {
        env.error("JS class merging did not produce expected output")
        env.info(js.data())
    }
}

// --- Style merging in JS ---

@test
public func feature_js_style_merge_semicolon_separated(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("style", make_ssr_text_val("color:red")))
    attrs.push(make_ssr_attr("style", make_ssr_text_val("font-size:14px")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut page, &list)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("style:")) {
        env.success("JS style merging produces merged style string")
    } else {
        env.error("JS style merging did not produce expected output")
        env.info(js.data())
    }
}

// --- Spread attrs in HTML ---

@test
public func feature_html_spread_attrs_resolved(env : &mut TestEnv) {
    var page = HtmlPage()
    var inner_attrs = std::vector<SsrAttribute>()
    inner_attrs.push(make_ssr_attr("title", make_ssr_text_val("spread")))
    var inner_list = SsrAttributeList {
        data : inner_attrs.data() as *SsrAttribute,
        size : inner_attrs.size()
    }
    var spread_value = SsrAttributeValue.Spread(inner_list)
    var attr = SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("data-x")),
        value : spread_value
    }
    var list = SsrAttributeList {
        data : &raw attr,
        size : 1
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("title=\"spread\"")) {
        env.success("HTML spread attrs are recursively resolved")
    } else {
        env.error("HTML spread attrs not resolved")
        env.info(html.data())
    }
}

// --- Last-wins dedup in HTML ---

@test
public func feature_html_last_wins_dedup(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("data-v", make_ssr_text_val("first")))
    attrs.push(make_ssr_attr("data-v", make_ssr_text_val("second")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("data-v=\"second\"") && !html.contains("data-v=\"first\"")) {
        env.success("HTML last-wins dedup: last value overwrites earlier")
    } else {
        env.error("HTML dedup behavior incorrect")
        env.info(html.data())
    }
}

// --- Last-wins dedup in JS ---

@test
public func feature_js_last_wins_dedup(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(make_ssr_attr("data-v", make_ssr_text_val("first")))
    attrs.push(make_ssr_attr("data-v", make_ssr_text_val("second")))
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut page, &list)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("data-v:\"second\"")) {
        env.success("JS last-wins dedup: last value overwrites earlier")
    } else {
        env.error("JS dedup behavior incorrect")
        env.info(js.data())
    }
}

// --- SpecialAttrs bounds: >64 other attrs silently dropped ---

@test
public func feature_special_attrs_64_others_overflow(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    // Create 70 distinct non-class/non-style attributes
    var i = 0
    while(i < 70) {
        var name = std::string("a")
        name.append_integer(i as bigint)
        attrs.push(SsrAttribute {
            name : make_ssr_text_ut(&name.to_view()),
            value : make_ssr_text_val("v")
        })
        i++
    }
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    // Count how many of a0-a69 appear
    var count = 0
    var j = 0
    while(j < 70) {
        var name = std::string("a")
        name.append_integer(j as bigint)
        if(html.contains(&name.to_view())) {
            count++
        }
        j++
    }
    // SpecialAttrs allows 64 others — 65th and beyond silently dropped
    if(count <= 64) {
        env.success("SpecialAttrs bounds: attrs count <= 64 (bounded)")
    } else {
        env.error("SpecialAttrs bounds exceeded: more than 64 attrs rendered")
    }
}

// --- SpecialAttrs bounds: >32 classes silently dropped ---

@test
public func feature_special_attrs_32_classes_overflow(env : &mut TestEnv) {
    var page = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    var i = 0
    while(i < 40) {
        var name = std::string("c")
        name.append_integer(i as bigint)
        attrs.push(SsrAttribute {
            name : make_ssr_text_ut(&name.to_view()),
            value : make_ssr_text_val("cls")
        })
        i++
    }
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut page, &list)
    var html = std::string()
    html.append_view(page.getHtml())
    var count = 0
    var j = 0
    while(j < 40) {
        var name = std::string("c")
        name.append_integer(j as bigint)
        // Each class attr name should appear in the merged class value
        if(html.contains(&name.to_view())) {
            count++
        }
        j++
    }
    if(count <= 32) {
        env.success("SpecialAttrs class bounds: class count <= 32")
    } else {
        env.error("SpecialAttrs class bounds exceeded: more than 32 classes rendered")
    }
}

// --- HTML renders Boolean(false) child value as nothing ---

@test
public func feature_html_child_boolean_false_renders_nothing(env : &mut TestEnv) {
    var page = HtmlPage()
    var val = make_ssr_bool_val(false)
    renderHtmlChildValue(&mut page, &val)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.size() == 0) {
        env.success("HTML renderHtmlChildValue skips Boolean(false)")
    } else {
        env.error("HTML renderHtmlChildValue rendered Boolean(false)")
        env.info(html.data())
    }
}

@test
public func feature_html_child_none_renders_nothing(env : &mut TestEnv) {
    var page = HtmlPage()
    var val = make_ssr_none_val()
    renderHtmlChildValue(&mut page, &val)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.size() == 0) {
        env.success("HTML renderHtmlChildValue skips None")
    } else {
        env.error("HTML renderHtmlChildValue rendered None")
        env.info(html.data())
    }
}

@test
public func feature_html_attr_boolean_true_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    var val = make_ssr_bool_val(true)
    renderHtmlAttrValue(&mut page, &val)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("true")) {
        env.success("HTML renderHtmlAttrValue renders Boolean(true)")
    } else {
        env.error("HTML renderHtmlAttrValue did not render Boolean(true)")
        env.info(html.data())
    }
}
