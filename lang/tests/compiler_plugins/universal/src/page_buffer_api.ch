// =============================================================================
// Feature Regression Tests: HtmlPage Buffer API
//
// These tests pin the behavior of every public function on HtmlPage that is
// testable without a browser. When the universal library is rewritten, each
// test ensures the replacement produces the same output.
// =============================================================================

// --- escape_html / escape_html_view ---

@test
public func feature_escape_html_null_ptr_returns_empty(env : &mut TestEnv) {
    var result = escape_html(null)
    if(result.size() == 0) {
        env.success("escape_html(null) returns empty string")
    } else {
        env.error("escape_html(null) did not return empty")
    }
}

@test
public func feature_escape_html_escapes_all_special_chars(env : &mut TestEnv) {
    var result = escape_html("<b>&\"'hello</b>")
    if(result.contains("&lt;") && result.contains("&gt;") && result.contains("&amp;") && result.contains("&quot;") && result.contains("&#39;")) {
        env.success("escape_html escapes < > & \" ' correctly")
    } else {
        env.error("escape_html did not escape all special chars")
        env.info(result.data())
    }
}

@test
public func feature_escape_html_view_same_as_ptr(env : &mut TestEnv) {
    var view = std::string_view("<div class=\"x\">&test</div>")
    var from_view = escape_html_view(view)
    var from_ptr = escape_html(view.data())
    if(from_view.equals(&from_ptr)) {
        env.success("escape_html_view and escape_html produce identical output")
    } else {
        env.error("escape_html_view and escape_html differ")
        env.info(from_view.data())
    }
}

@test
public func feature_escape_html_plain_text_unchanged(env : &mut TestEnv) {
    var result = escape_html("hello world 123")
    if(result.contains("hello world 123")) {
        env.success("plain text passes through escape_html unchanged")
    } else {
        env.error("escape_html modified plain text")
    }
}

// --- HtmlPage buffer accessors ---

@test
public func feature_html_page_initial_buffers_empty(env : &mut TestEnv) {
    var page = HtmlPage()
    var h = page.getHtml().size() == 0
    var j = page.getJs().size() == 0
    var c = page.getCss().size() == 0
    var hd = page.getHead().size() == 0
    var hj = page.getHeadJs().size() == 0
    if(h && j && c && hd && hj) {
        env.success("all HtmlPage buffers start empty")
    } else {
        env.error("some HtmlPage buffers are non-empty on construction")
    }
}

// --- append_html_* variants ---

@test
public func feature_append_html_char_ptr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("<div>test</div>")
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("<div>test</div>")) {
        env.success("append_html_char_ptr writes to pageHtml")
    } else {
        env.error("append_html_char_ptr did not write correctly")
    }
}

@test
public func feature_append_html_char(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char('A')
    page.append_html_char('B')
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("AB")) {
        env.success("append_html_char writes single chars to pageHtml")
    } else {
        env.error("append_html_char did not write correctly")
    }
}

@test
public func feature_append_html_integer(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_integer(-42)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("-42")) {
        env.success("append_html_integer writes negative integer to pageHtml")
    } else {
        env.error("append_html_integer did not write correctly")
        env.info(html.data())
    }
}

@test
public func feature_append_html_uinteger(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_uinteger(255)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("255")) {
        env.success("append_html_uinteger writes unsigned integer to pageHtml")
    } else {
        env.error("append_html_uinteger did not write correctly")
    }
}

@test
public func feature_append_html_double(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_double(3.14)
    var html = std::string()
    html.append_view(page.getHtml())
    if(html.contains("3.14")) {
        env.success("append_html_double writes double to pageHtml")
    } else {
        env.error("append_html_double did not write correctly")
        env.info(html.data())
    }
}

// --- append_head_* variants ---

@test
public func feature_append_head_char_ptr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_head_char_ptr("<meta charset=\"utf-8\">")
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("<meta charset=\"utf-8\">")) {
        env.success("append_head_char_ptr writes to pageHead")
    } else {
        env.error("append_head_char_ptr did not write correctly")
    }
}

@test
public func feature_append_head_integer(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_head_integer(42)
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("42")) {
        env.success("append_head_integer writes to pageHead")
    } else {
        env.error("append_head_integer did not write correctly")
    }
}

// --- append_css_* variants ---

@test
public func feature_append_css_char_ptr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_css_char_ptr(".btn { color: red; }")
    var css = std::string()
    css.append_view(page.getCss())
    if(css.contains(".btn { color: red; }")) {
        env.success("append_css_char_ptr writes to pageCss")
    } else {
        env.error("append_css_char_ptr did not write correctly")
    }
}

@test
public func feature_append_css_integer(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_css_integer(100)
    var css = std::string()
    css.append_view(page.getCss())
    if(css.contains("100")) {
        env.success("append_css_integer writes to pageCss")
    } else {
        env.error("append_css_integer did not write correctly")
    }
}

// --- append_js_* variants ---

@test
public func feature_append_js_char_ptr(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_char_ptr("var x = 1;")
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("var x = 1;")) {
        env.success("append_js_char_ptr writes to pageJs")
    } else {
        env.error("append_js_char_ptr did not write correctly")
    }
}

@test
public func feature_append_js_integer(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_integer(99)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("99")) {
        env.success("append_js_integer writes to pageJs")
    } else {
        env.error("append_js_integer did not write correctly")
    }
}

@test
public func feature_append_js_uinteger(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_uinteger(4096)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("4096")) {
        env.success("append_js_uinteger writes to pageJs")
    } else {
        env.error("append_js_uinteger did not write correctly")
    }
}

// --- append_js_escaped_char_ptr ---

@test
public func feature_append_js_escaped_escapes_quotes(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_escaped_char_ptr("say \"hello\"")
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\\"hello\\\"")) {
        env.success("append_js_escaped escapes double quotes")
    } else {
        env.error("append_js_escaped did not escape quotes")
        env.info(js.data())
    }
}

@test
public func feature_append_js_escaped_escapes_backslash(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_escaped_char_ptr("path\\to")
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("path\\\\to")) {
        env.success("append_js_escaped escapes backslashes")
    } else {
        env.error("append_js_escaped did not escape backslash")
        env.info(js.data())
    }
}

@test
public func feature_append_js_escaped_escapes_newline(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_escaped_char_ptr("line1\nline2")
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\n")) {
        env.success("append_js_escaped escapes newlines as \\n")
    } else {
        env.error("append_js_escaped did not escape newline")
        env.info(js.data())
    }
}

@test
public func feature_append_js_escaped_escapes_script_tag(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_escaped_char_ptr("</script>")
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\u003C/")) {
        env.success("append_js_escaped escapes </script> as \\u003C/")
    } else {
        env.error("append_js_escaped did not escape </script>")
        env.info(js.data())
    }
}

// --- get_html_size / truncate_html ---

@test
public func feature_get_html_size_reflects_appends(env : &mut TestEnv) {
    var page = HtmlPage()
    var before = page.get_html_size()
    page.append_html_char_ptr("hello")
    var after = page.get_html_size()
    if(before == 0 && after == 5) {
        env.success("get_html_size reflects prior appends")
    } else {
        env.error("get_html_size incorrect: before != 0 or after != 5")
    }
}

@test
public func feature_truncate_html_reduces_size(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("hello world")
    page.truncate_html(5)
    var html = std::string()
    html.append_view(page.getHtml())
    if(page.get_html_size() == 5 && html.contains("hello")) {
        env.success("truncate_html truncates to correct size")
    } else {
        env.error("truncate_html did not work correctly")
        env.info(html.data())
    }
}

// --- get_js_pos ---

@test
public func feature_get_js_pos_reflects_appends(env : &mut TestEnv) {
    var page = HtmlPage()
    var before = page.get_js_pos()
    page.append_js_char_ptr("var x = 1;")
    var after = page.get_js_pos()
    if(before == 0 && after == 10) {
        env.success("get_js_pos reflects prior JS appends")
    } else {
        env.error("get_js_pos incorrect: before != 0 or after != 11")
    }
}

// --- dedup maps ---

@test
public func feature_require_css_hash_first_call_true(env : &mut TestEnv) {
    var page = HtmlPage()
    var first = page.require_css_hash(42)
    page.set_css_hash(42)
    var second = page.require_css_hash(42)
    if(first && !second) {
        env.success("require_css_hash returns true first time, false after set")
    } else {
        env.error("require_css_hash dedup broken")
    }
}

@test
public func feature_require_component_hash_first_call_true(env : &mut TestEnv) {
    var page = HtmlPage()
    var first = page.require_component(99)
    page.set_component_hash(99)
    var second = page.require_component(99)
    if(first && !second) {
        env.success("require_component returns true first time, false after set")
    } else {
        env.error("require_component dedup broken")
    }
}

@test
public func feature_require_random_css_hash_first_call_true(env : &mut TestEnv) {
    var page = HtmlPage()
    var first = page.require_random_css_hash(77)
    page.set_random_css_hash(77)
    var second = page.require_random_css_hash(77)
    if(first && !second) {
        env.success("require_random_css_hash returns true first time, false after set")
    } else {
        env.error("require_random_css_hash dedup broken")
    }
}

@test
public func feature_dedup_hashes_independent(env : &mut TestEnv) {
    var page = HtmlPage()
    page.set_css_hash(10)
    // Only set css_hash(10). Component and random should still accept 10.
    var comp = page.require_component(10)
    var rand = page.require_random_css_hash(10)
    // css_hash(10) should be false (already set)
    var css = page.require_css_hash(10)
    if(!css && comp && rand) {
        env.success("css/component/random dedup maps are independent")
    } else {
        env.error("dedup maps are not independent")
    }
}

// --- capture_html_delta_to_js ---

@test
public func feature_capture_html_delta_to_js_escapes_backtick(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("a`b")
    page.capture_html_delta_to_js(0)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("a\\`b")) {
        env.success("capture_html_delta_to_js escapes backtick")
    } else {
        env.error("capture_html_delta_to_js did not escape backtick")
        env.info(js.data())
    }
}

@test
public func feature_capture_html_delta_to_js_escapes_dollar_brace(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("val=${x}")
    page.capture_html_delta_to_js(0)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\${")) {
        env.success("capture_html_delta_to_js escapes ${")
    } else {
        env.error("capture_html_delta_to_js did not escape ${")
        env.info(js.data())
    }
}

@test
public func feature_capture_html_delta_to_js_escapes_backslash(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("path\\file")
    page.capture_html_delta_to_js(0)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("path\\\\file")) {
        env.success("capture_html_delta_to_js escapes backslash")
    } else {
        env.error("capture_html_delta_to_js did not escape backslash")
        env.info(js.data())
    }
}

@test
public func feature_capture_html_delta_to_js_escapes_newline(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("a\nb")
    page.capture_html_delta_to_js(0)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("\\n")) {
        env.success("capture_html_delta_to_js escapes newline")
    } else {
        env.error("capture_html_delta_to_js did not escape newline")
        env.info(js.data())
    }
}

@test
public func feature_capture_html_delta_to_js_truncates_html(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("content")
    var size_before = page.get_html_size()
    page.capture_html_delta_to_js(0)
    var size_after = page.get_html_size()
    if(size_before > 0 && size_after == 0) {
        env.success("capture_html_delta_to_js truncates pageHtml to 0")
    } else {
        env.error("capture_html_delta_to_js did not truncate")
    }
}

// --- move_js_range ---

@test
public func feature_move_js_range_basic(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_char_ptr("AAABBBCCC")
    // Move "BBB" (pos 3-6) to index 0 (before "AAA")
    page.move_js_range(3, 6, 0)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("BBBAAACCC")) {
        env.success("move_js_range moves range before existing content")
    } else {
        env.error("move_js_range produced wrong result")
        env.info(js.data())
    }
}

@test
public func feature_move_js_range_move_to_end(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_char_ptr("AAABBBCCC")
    // Move "AAA" (pos 0-3) to index 9 (after everything)
    page.move_js_range(0, 3, 9)
    var js = std::string()
    js.append_view(page.getJs())
    if(js.contains("BBBCCCAAA")) {
        env.success("move_js_range moves range to end")
    } else {
        env.error("move_js_range move-to-end produced wrong result")
        env.info(js.data())
    }
}

// --- defaultPrepare ---

@test
public func feature_default_prepare_adds_meta_tags(env : &mut TestEnv) {
    var page = HtmlPage()
    page.defaultPrepare()
    var head = std::string()
    head.append_view(page.getHead())
    var has_charset = head.contains("charset")
    var has_viewport = head.contains("viewport")
    if(has_charset && has_viewport) {
        env.success("defaultPrepare adds charset and viewport meta tags")
    } else {
        env.error("defaultPrepare missing meta tags")
        env.info(head.data())
    }
}

// --- appendTitle ---

@test
public func feature_append_title(env : &mut TestEnv) {
    var page = HtmlPage()
    page.appendTitle(&std::string_view("My Page"))
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("<title>My Page</title>")) {
        env.success("append_title wraps content in title tags")
    } else {
        env.error("append_title did not produce correct output")
        env.info(head.data())
    }
}

// --- appendViewportMeta ---

@test
public func feature_append_viewport_meta(env : &mut TestEnv) {
    var page = HtmlPage()
    page.appendViewportMeta()
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("viewport") && head.contains("width=device-width")) {
        env.success("appendViewportMeta emits viewport meta tag")
    } else {
        env.error("appendViewportMeta did not produce correct output")
        env.info(head.data())
    }
}

// --- appendCharsetUTF8Meta ---

@test
public func feature_append_charset_utf8_meta(env : &mut TestEnv) {
    var page = HtmlPage()
    page.appendCharsetUTF8Meta()
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("charset") && head.contains("utf-8")) {
        env.success("appendCharsetUTF8Meta emits charset meta tag")
    } else {
        env.error("appendCharsetUTF8Meta did not produce correct output")
        env.info(head.data())
    }
}

// --- appendFavicon ---

@test
public func feature_append_favicon(env : &mut TestEnv) {
    var page = HtmlPage()
    page.appendFavicon(&std::string_view("image/png"), &std::string_view("/favicon.png"))
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("icon") && head.contains("image/png") && head.contains("/favicon.png")) {
        env.success("appendFavicon generates correct link tag")
    } else {
        env.error("appendFavicon did not produce correct output")
        env.info(head.data())
    }
}

@test
public func feature_append_png_favicon(env : &mut TestEnv) {
    var page = HtmlPage()
    page.appendPngFavicon(&std::string_view("/icon.png"))
    var head = std::string()
    head.append_view(page.getHead())
    if(head.contains("icon") && head.contains("image/png")) {
        env.success("appendPngFavicon generates correct link tag")
    } else {
        env.error("appendPngFavicon did not produce correct output")
        env.info(head.data())
    }
}

// --- toString variants ---

@test
public func feature_to_string_has_doctype(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("<div>hello</div>")
    var result = page.toString()
    if(result.contains("<!DOCTYPE html>")) {
        env.success("toString includes DOCTYPE")
    } else {
        env.error("toString missing DOCTYPE")
        env.info(result.data())
    }
}

@test
public func feature_to_string_inlines_css_in_style_tag(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_css_char_ptr(".x{color:red}")
    var result = page.toString()
    if(result.contains("<style>") && result.contains(".x{color:red}") && result.contains("</style>")) {
        env.success("toString wraps CSS in <style> tags")
    } else {
        env.error("toString did not inline CSS correctly")
        env.info(result.data())
    }
}

@test
public func feature_to_string_empty_css_no_style_tag(env : &mut TestEnv) {
    var page = HtmlPage()
    var result = page.toString()
    if(!result.contains("<style>")) {
        env.success("toString omits <style> tag when CSS is empty")
    } else {
        env.error("toString included empty <style> tag")
        env.info(result.data())
    }
}

@test
public func feature_to_string_body_contains_html_content(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("<p>content</p>")
    var result = page.toString()
    if(result.contains("<body>") && result.contains("<p>content</p>") && result.contains("</body>")) {
        env.success("toString places HTML content inside <body>")
    } else {
        env.error("toString body content incorrect")
        env.info(result.data())
    }
}

@test
public func feature_to_string_with_lang_and_classes(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_html_char_ptr("<p>hi</p>")
    var result = page.toString(std::string_view("en"), std::string_view("root"), std::string_view("app-body"))
    if(result.contains("<html lang=\"en\" class=\"root\">") && result.contains("<body class=\"app-body\">")) {
        env.success("toString applies lang and class attributes to html/body")
    } else {
        env.error("toString did not apply lang/class correctly")
        env.info(result.data())
    }
}

// --- getFinalizedPageJs ---

@test
public func feature_get_finalized_page_js_concatenates_pages(env : &mut TestEnv) {
    var page = HtmlPage()
    page.append_js_char_ptr("var a = 1;")
    // Simulate what defaultUniversalSetup does for pageJsEnd
    var finalized = page.getFinalizedPageJs()
    if(finalized.contains("var a = 1;")) {
        env.success("getFinalizedPageJs contains pageJs content")
    } else {
        env.error("getFinalizedPageJs missing pageJs content")
    }
}
