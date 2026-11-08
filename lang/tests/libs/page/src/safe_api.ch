// P2.5 — safe page surface: escaping, raw append, typed parameters.
//
// These tests exercise the additive `page::safe` façade only (never a raw
// pointer). The XSS cases mirror the strings a browser would treat as markup /
// script if escaping were skipped.

using namespace std;

func ps_contains(hay : string_view, needle : string_view) : bool {
    return hay.find(&needle) != NPOS
}

func ps_eq(value : string_view, expected : string_view) : bool {
    return value.equals(&expected)
}

@test
func page_safe_escape_html_escapes_all_five_entities(env : &mut TestEnv) {
    var out = safe_page_escape_html("<a href='x'>&\"</a>")
    if(!ps_contains(out.to_view(), string_view("&lt;a"))) { env.error("did not escape '<'"); return }
    if(!ps_contains(out.to_view(), string_view("&gt;"))) { env.error("did not escape '>'"); return }
    if(!ps_contains(out.to_view(), string_view("&amp;"))) { env.error("did not escape '&'"); return }
    if(!ps_contains(out.to_view(), string_view("&quot;"))) { env.error("did not escape '\"'"); return }
    if(!ps_contains(out.to_view(), string_view("&#39;"))) { env.error("did not escape '\\''"); return }
    if(ps_contains(out.to_view(), string_view("<a"))) { env.error("raw tag survived escaping"); return }
}

@test
func page_safe_escape_js_breaks_scripts_and_quotes(env : &mut TestEnv) {
    var out = safe_page_escape_js("it's a\\b</script><script>alert(1)</script>")
    if(ps_contains(out.to_view(), string_view("</script>"))) { env.error("raw </script> survived"); return }
    if(!ps_contains(out.to_view(), string_view("\\u003C/script"))) { env.error("</ was not escaped as \\u003C/"); return }
    if(!ps_contains(out.to_view(), string_view("\\'"))) { env.error("single quote was not escaped"); return }
    if(!ps_contains(out.to_view(), string_view("\\\\"))) { env.error("backslash was not escaped"); return }
}

@test
func page_safe_escape_json_breaks_scripts_and_quotes(env : &mut TestEnv) {
    var out = safe_page_escape_json("a\"b\\c</script>")
    if(ps_contains(out.to_view(), string_view("</script>"))) { env.error("raw </script> survived JSON escaping"); return }
    if(!ps_contains(out.to_view(), string_view("\\u003C/script"))) { env.error("</ was not escaped in JSON"); return }
    if(!ps_contains(out.to_view(), string_view("\\\""))) { env.error("double quote was not escaped"); return }
    if(!ps_contains(out.to_view(), string_view("\\\\"))) { env.error("backslash was not escaped in JSON"); return }
}

@test
func page_safe_append_html_and_js_keep_length(env : &mut TestEnv) {
    var html_page = HtmlPage()
    safe_page_append_html(&mut html_page, "<b>hi</b>")
    safe_page_append_js(&mut html_page, "var x = 1;")
    var html = html_page.getHtml()
    var js = html_page.getJs()
    if(!ps_eq(html, string_view("<b>hi</b>"))) { env.error("append_html wrote the wrong bytes"); return }
    if(!ps_eq(js, string_view("var x = 1;"))) { env.error("append_js wrote the wrong bytes"); return }
}

@test
func page_safe_append_escaped_blocks_injection(env : &mut TestEnv) {
    var html_page = HtmlPage()
    var payload = string("<img src=x onerror=alert(1)>")
    var payload_str = std::str.make_view(payload.to_view())
    safe_page_append_html_escaped(&mut html_page, &payload_str)
    var html = html_page.getHtml()
    if(ps_contains(html, string_view("<img"))) { env.error("append_html_escaped let markup through"); return }
    if(!ps_contains(html, string_view("&lt;img"))) { env.error("append_html_escaped did not escape"); return }

    safe_page_append_js_escaped(&mut html_page, &payload_str)
    var all_js = html_page.getJs()
    if(ps_contains(all_js, string_view("</script>"))) { env.error("append_js_escaped let a closing tag through"); return }
}

@test
func page_safe_typed_parameter_round_trip(env : &mut TestEnv) {
    var html_page = HtmlPage()

    var count : int = 42
    var enabled : bool = true
    var name = string("Ada")
    safe_page_set_parameter(&mut html_page, "count", &count)
    safe_page_set_parameter(&mut html_page, "enabled", &enabled)
    safe_page_set_parameter(&mut html_page, "name", &name)

    var count_opt = safe_page_get_parameter_safe(&html_page, "count")
    if(count_opt is Option.None) { env.error("count parameter missing"); return }
    var Some(count_text) = count_opt else unsafe { unreachable }
    if(!ps_eq(count_text.to_view(), string_view("42"))) { env.error("int parameter was not formatted as text"); return }

    var enabled_opt = safe_page_get_parameter_safe(&html_page, "enabled")
    if(enabled_opt is Option.None) { env.error("enabled parameter missing"); return }
    var Some(enabled_text) = enabled_opt else unsafe { unreachable }
    if(!ps_eq(enabled_text.to_view(), string_view("true"))) { env.error("bool parameter was not formatted as text"); return }

    var name_opt = safe_page_get_parameter_safe(&html_page, "name")
    if(name_opt is Option.None) { env.error("name parameter missing"); return }
    var Some(name_text) = name_opt else unsafe { unreachable }
    if(!ps_eq(name_text.to_view(), string_view("Ada"))) { env.error("string parameter did not round-trip"); return }

    var missing = safe_page_get_parameter_safe(&html_page, "nope")
    if(missing is Option.Some) { env.error("missing parameter should be None"); return }

    // The key must be copied into page-owned storage: mutating the caller's
    // key after setting the parameter must not affect lookup.
    var dynamic_key = string("dynamic_key")
    safe_page_set_parameter(&mut html_page, &dynamic_key, &count)
    dynamic_key.clear()
    var dynamic_opt = safe_page_get_parameter_safe(&html_page, "dynamic_key")
    if(dynamic_opt is Option.None) { env.error("dynamic parameter key was not copied"); return }
}
