func untrusted_html() : *char {
    return "<script>alert(1)</script>"
}

func untrusted_attr() : *char {
    return "\"><script>x</script>"
}

@test
public func escape_html_char_ptr_escapes_special_chars(env : &mut TestEnv) {
    var out = escape_html(std::string_view("<>& \"'").data())
    string_equals(env, &out, "&lt;&gt;&amp; &quot;&#39;")
}

@test
public func escape_html_view_escapes_special_chars(env : &mut TestEnv) {
    var out = escape_html_view(std::string_view("a<b>c&d\"e'f"))
    string_equals(env, &out, "a&lt;b&gt;c&amp;d&quot;e&#39;f")
}

@test
public func escape_html_ptr_escapes(env : &mut TestEnv) {
    var out = escape_html(untrusted_html())
    string_equals(env, &out, "&lt;script&gt;alert(1)&lt;/script&gt;")
}

@test
public func escape_html_null_is_safe(env : &mut TestEnv) {
    var out = escape_html(null)
    string_equals(env, &out, "")
}

@test
public func escape_html_empty_is_safe(env : &mut TestEnv) {
    var out = escape_html_view(std::string_view(""))
    string_equals(env, &out, "")
}

@test
public func escape_html_used_in_expression_is_safe(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{escape_html(untrusted_html())}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>&lt;script&gt;alert(1)&lt;/script&gt;</div>")
}

@test
public func escape_html_attr_used_in_expression_is_safe(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div class={escape_html(untrusted_attr())}>content</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div class=\"&quot;&gt;&lt;script&gt;x&lt;/script&gt;\">content</div>")
}

@test
public func framework_does_not_auto_escape_char_ptr(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{untrusted_html()}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><script>alert(1)</script></div>")
}
