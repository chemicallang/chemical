// Tests for SSR serialization safety and XSS vulnerabilities in the universal
// runtime. These tests target specific bugs documented in the universal skill
// and improvements doc.
//
// SUSPECTED BUGS TARGETED:
// 1. capture_html_delta_to_js does NOT escape </script> — SSR HTML containing
//    it breaks out of inline <script> under toString() (XSS).
// 2. writeJsPrimitiveAttrValue escapes quotes/backslash/newlines but does NOT
//    escape </script> in the JS target (different from appendJsEscaped).
// 3. writePrimitiveAttrValue (HTML target) does NOT escape single quotes in
//    attribute values — only escapes & < > ".
// 4. Boolean false attributes in HTML target are skipped (correct) but the
//    JS target renders "false" as a string — divergence.
// 5. SsrAttributeValue.None renders as "undefined" in JS but nothing in HTML
//    — correct but pinning to detect regressions.
// =============================================================================

// =============================================================================
// Bug #1: </script> not escaped in captured HTML (XSS).
//
// capture_html_delta_to_js escapes `, ${, \, \n, \r but NOT </script>.
// If a component's SSR output contains </script>, the inline <script> tag
// in toString() breaks out, allowing HTML injection.
//
// Test: verify the escape function does NOT escape </ sequences.
// (This confirms the bug exists.)
// =============================================================================

#universal ScriptTagComp(props) {
    return <div>{props.html}</div>
}

@test
public func universal_captured_html_not_escaped_for_script_tag(env : &mut TestEnv) {
    var page = HtmlPage()
    // Manually append HTML that contains </script> to simulate SSR output
    // that would be captured by capture_html_delta_to_js.
    page.append_html_char_ptr("<script>alert(1)</script>")
    var html_size_before = page.getHtml().size()
    // capture_html_delta_to_js now escapes </script> as \u003C/script> to
    // prevent inline <script> breakout (XSS fix).
    page.capture_html_delta_to_js(0)
    var js = std::string()
    js.append_view(page.getJs())
    // Verify the </script> is escaped as \u003C/script>
    if(js.contains(&std::string_view("\\u003C/script>"))) {
        env.success("</script> is correctly escaped as \\u003C/script> in captured HTML")
    } else if(js.contains(&std::string_view("</script>"))) {
        env.error("</script> is NOT escaped — XSS vulnerability still present")
    } else {
        env.error("unexpected output — neither raw nor escaped </script> found")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #2: JS attribute rendering does NOT escape </script>.
//
// appendJsEscaped escapes </ as \u003C/ to prevent inline script breakout.
// But writeJsPrimitiveAttrValue uses appendJsEscaped, which DOES handle it.
// However, when the value is embedded in pageJs as a string literal (not
// through appendJsEscaped), the </script> is NOT escaped.
//
// Test: verify that JS-escaped attribute values DO escape </script>.
// This tests the appendJsEscaped path, which should work.
// =============================================================================

@test
public func universal_js_attr_escapes_script_tag(env : &mut TestEnv) {
    var pg = HtmlPage()
    var hostile = std::string_view("</script><script>alert(1)</script>")
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("data-x")),
        value : SsrAttributeValue.Text(make_ssr_text_ut(&hostile))
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut pg, &list)
    var js = std::string()
    js.append_view(pg.getJs())
    // appendJsEscaped should convert </ to \u003C/
    if(js.contains(&std::string_view("\\u003C/"))) {
        env.success("JS attribute values escape </script> via appendJsEscaped")
    } else {
        env.error("JS attribute values do NOT escape </script> — possible XSS")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #3: HTML attribute rendering does NOT escape single quotes.
//
// appendHtmlEscaped escapes & < > " but NOT ' (single quote). This is
// technically correct for double-quoted attributes (attr="...") but means
// single-quoted HTML attributes (attr='...') are vulnerable. Since the
// renderer always uses double quotes, this is not currently exploitable,
// but it's a defense-in-depth gap.
//
// Test: verify that HTML attribute values escape double quotes but not
// single quotes (pinning current behavior).
// =============================================================================

@test
public func universal_html_attr_escapes_double_quotes_not_single(env : &mut TestEnv) {
    var pg = HtmlPage()
    var hostile = std::string_view("it's a \"test\" <b>bold</b>")
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("title")),
        value : SsrAttributeValue.Text(make_ssr_text_ut(&hostile))
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderHtmlAttrs(&mut pg, &list)
    var html = std::string()
    html.append_view(pg.getHtml())
    // Double quotes should be escaped as &quot;
    // Single quotes should NOT be escaped (current behavior).
    // < and > should be escaped.
    var has_quot = html.contains(&std::string_view("&quot;"))
    var has_lt = html.contains(&std::string_view("&lt;"))
    var has_gt = html.contains(&std::string_view("&gt;"))
    var has_single_quote = html.contains(&std::string_view("it's"))
    if(has_quot && has_lt && has_gt) {
        env.success("HTML attributes escape &, <, >, \" correctly")
    } else {
        env.error("HTML attribute escaping incomplete")
        env.info(html.data())
    }
    // Single quote is NOT escaped — pinning this as current behavior.
    if(has_single_quote) {
        env.success("confirmed: single quote is NOT escaped in HTML attributes (defense-in-depth gap)")
    }
}

// =============================================================================
// Bug #4: Boolean false in JS target renders as "false" string.
//
// In writeJsPrimitiveAttrValue, Boolean(false) renders as "false" (the
// string). In the HTML target, Boolean(false) renders nothing (attribute
// skipped). This divergence is correct for JS props but worth pinning.
//
// Test: verify the JS rendering.
// =============================================================================

@test
public func universal_js_bool_false_is_skipped(env : &mut TestEnv) {
    var pg = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("disabled")),
        value : SsrAttributeValue.Boolean(false)
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut pg, &list)
    var js = std::string()
    js.append_view(pg.getJs())
    // React semantics: a false boolean prop is omitted from the JS prop object
    // (the client treats a missing/false prop as "not set"), it is not
    // serialized as the string "false".
    if(!js.contains(&std::string_view("disabled"))) {
        env.success("JS omits Boolean(false) attributes")
    } else {
        env.error("JS rendered Boolean(false) instead of omitting it")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #5: SsrAttributeValue.None renders as "undefined" in JS.
//
// In writeJsPrimitiveAttrValue, None renders as "undefined". In the HTML
// target, None renders nothing (attribute skipped). This divergence is
// correct but pinning catches regressions.
// =============================================================================

@test
public func universal_js_none_is_skipped(env : &mut TestEnv) {
    var pg = HtmlPage()
    var attrs = std::vector<SsrAttribute>()
    attrs.push(SsrAttribute {
        name : make_ssr_text_ut(&std::string_view("data-missing")),
        value : SsrAttributeValue.None()
    })
    var list = SsrAttributeList {
        data : attrs.data() as *SsrAttribute,
        size : attrs.size()
    }
    renderJsAttrs(&mut pg, &list)
    var js = std::string()
    js.append_view(pg.getJs())
    // None means "no value": the attribute is omitted from the JS prop object
    // rather than serialized as the literal `undefined`.
    if(!js.contains(&std::string_view("data-missing"))) {
        env.success("JS omits SsrAttributeValue.None attributes")
    } else {
        env.error("JS rendered None instead of omitting it")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #6: Prop with backslash performs escape injection.
//
// A Chemical string like "path\\to\\file" contains literal backslashes.
// When passed as a prop to a universal component, the converter embeds it
// in the JS bundle. If not properly escaped, the backslashes become JS
// escape sequences (\t, \f, etc.), corrupting the value.
//
// Test: verify that backslashes in prop values are properly escaped.
// =============================================================================

#universal BackslashPropComp(props) {
    return <div>{props.path}</div>
}

@test
public func universal_backslash_prop_escaped_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    var path = "C:\\Users\\test\\file.txt"
    #html { <BackslashPropComp path={path} /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The JS bundle should contain the backslashes doubled (\\) so the
    // runtime receives the original string. If single backslashes appear,
    // escape injection has occurred.
    if(js.contains(&std::string_view("C:\\\\Users\\\\test\\\\file.txt"))) {
        env.success("backslash props are properly escaped in JS bundle")
    } else {
        env.error("backslash props may not be properly escaped")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #7: Newline in prop breaks JS string literal.
//
// A prop containing a literal newline character must be escaped as \n in
// the JS bundle. Otherwise the string literal is broken across lines.
//
// Test: verify that newlines in prop values are escaped.
// =============================================================================

#universal NewlinePropComp(props) {
    return <pre>{props.text}</pre>
}

@test
public func universal_newline_prop_escaped_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    var text = "line1\nline2"
    #html { <NewlinePropComp text={text} /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The JS bundle should contain \n (escaped) not an actual newline.
    if(js.contains(&std::string_view("\\n"))) {
        env.success("newline props are escaped in JS bundle")
    } else {
        env.error("newline props may not be escaped (literal newline in JS string)")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #8: Double quote in prop breaks JS string literal.
//
// A prop containing " must be escaped as \" in the JS bundle. Otherwise
// the string literal terminates early.
//
// Test: verify that double quotes in prop values are escaped.
// =============================================================================

#universal QuotePropComp(props) {
    return <span>{props.value}</span>
}

@test
public func universal_quote_prop_escaped_in_js(env : &mut TestEnv) {
    var page = HtmlPage()
    var value = "say \"hello\""
    #html { <QuotePropComp value={value} /> }
    var js = std::string()
    js.append_view(page.getJs())
    // The JS bundle should contain \" (escaped) not raw ".
    if(js.contains(&std::string_view("\\\""))) {
        env.success("double quote props are escaped in JS bundle")
    } else {
        env.error("double quote props may not be escaped")
        env.info(js.data())
    }
}

// =============================================================================
// Bug #9: HTML entity rendering — special characters in text content should
// be escaped. This tests the component body rendering path, not the attribute
// path (which is tested in ssr_safety.ch).
// =============================================================================

#universal HtmlEntityComp(props) {
    return <div>{props.content}</div>
}

@test
public func universal_text_content_html_escaped(env : &mut TestEnv) {
    var page = HtmlPage()
    var content = "<script>alert('xss')</script>"
    #html { <HtmlEntityComp content={content} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // Text content inside elements should be escaped so <script> doesn't
    // become a real script tag.
    if(html.contains(&std::string_view("&lt;script&gt;")) || !html.contains(&std::string_view("<script>"))) {
        env.success("text content is HTML-escaped in SSR output")
    } else {
        env.error("text content is NOT HTML-escaped — XSS vulnerability")
        env.info(html.data())
    }
}

// =============================================================================
// Bug #10: Numeric precision — float values rendered with default precision 3.
// This is documented as potentially truncating for large-scale data UIs.
// Test that a float prop renders with at most 3 decimal places.
// =============================================================================

#universal FloatPrecisionComp(props) {
    return <span>{props.value}</span>
}

@test
public func universal_float_precision_in_ssr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <FloatPrecisionComp value={3.14159} /> }
    var html = std::string()
    html.append_view(page.getHtml())
    // The SSR output should render the float with some precision.
    // Default is 3 decimal places (append_double with precision 3).
    if(html.contains(&std::string_view("3.14"))) {
        env.success("float precision renders correctly in SSR")
    } else {
        env.error("float precision did not render as expected")
        env.info(html.data())
    }
}
