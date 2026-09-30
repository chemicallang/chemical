using namespace std;

// ---------------------------------------------------------------------------
// PageWriter: the JS/CSS literal writers must escape, not just quote.
//
// `writeToPageJs` is the sink every interpolated value in a universal
// component's props payload goes through: `html_cbi`'s `emit_universal_queue`
// asks `is_string_type` whether to add its own quotes, and a `std::string` /
// `std::string_view` is a *Linked* type, so the answer is false and the value is
// delegated to `writeToPageJs` instead.
//
// It used to do `buffer.append_with_len(self.data(), self.size())` between two
// quotes - raw. Because that output is emitted inside an inline <script>, a
// value containing `</script>` closed the element and the rest of the value ran
// as script: a reflected XSS on any page that echoes a URL parameter into a
// component prop. A `'` in the value also broke out of the literal.
// ---------------------------------------------------------------------------

// `find` needs a view, and a method call on a literal is not parseable, so the
// needle is bound first.
func pw_has(hay : &string, needle : &string_view) : bool {
    return hay.find(needle) != NPOS
}
func pw_fail(env : &mut TestEnv, what : string_view, out : &string) {
    var msg = string()
    msg.append_view(&what)
    msg.append_view(" -> ")
    msg.append_view(out.to_view())
    env.error(msg.data())
}

// Runs `writeToPageJs` on a `std::string` and returns what it produced.
func pw_js(value : string) : string {
    var s = value.copy();
    var buffer = string();
    var page = HtmlPage();
    s.writeToPageJs(&mut page, &mut buffer);
    return buffer;
}

// The same through the `std::string_view` impl.
func pw_js_view(value : string) : string {
    var v = value.to_view();
    var buffer = string();
    var page = HtmlPage();
    v.writeToPageJs(&mut page, &mut buffer);
    return buffer;
}

func pw_css(value : string) : string {
    var s = value.copy();
    var buffer = string();
    var page = HtmlPage();
    s.writeToPageCss(&mut page, &mut buffer);
    return buffer;
}

// The regression: a closing script tag must never survive.
@test
func test_write_to_page_js_escapes_a_closing_script_tag(env : &mut TestEnv) {
    var payload = string("</script><script>window.__xss=1</script>")
    var out = pw_js(payload.copy())
    if(pw_has(&out, "</script>")) {
        pw_fail(env, string_view("writeToPageJs let a closing script tag through"), &out)
        return
    }
    if(!pw_has(&out, "\\u003C/script")) {
        pw_fail(env, string_view("writeToPageJs did not escape `</` as \\u003C/"), &out)
        return
    }
    // Still a single-quoted literal.
    if(out.size() < 2u) { pw_fail(env, string_view("output is too short to be a quoted literal"), &out); return }
    if(out.get(0) != '\'') { pw_fail(env, string_view("output must open with a single quote"), &out); return }
    if(out.get(out.size() - 1u) != '\'') { pw_fail(env, string_view("output must close with a single quote"), &out); return }
}

@test
func test_write_to_page_js_escapes_a_breaking_quote(env : &mut TestEnv) {
    var payload = string("it's a; alert(1); //")
    var out = pw_js(payload.copy())
    // No UNESCAPED quote may sit between the delimiters: a `'` preceded by a
    // backslash is the escape, not a break-out.
    var i = 1u;
    while(i + 1u < out.size()) {
        if(out.get(i) == '\'') {
            if(i == 0u || out.get(i - 1u) != '\\') {
                pw_fail(env, string_view("writeToPageJs left an unescaped single quote in the literal"), &out)
                return
            }
        }
        i++
    }
    // And the escaped form is actually present.
    if(!pw_has(&out, "\\'")) {
        pw_fail(env, string_view("writeToPageJs must escape a single quote as \\'"), &out)
        return
    }
}

@test
func test_write_to_page_js_escapes_backslash_and_controls(env : &mut TestEnv) {
    var payload = string("a\\b\nc\td")
    var out = pw_js(payload.copy())
    if(pw_has(&out, "\\n") == NPOS || pw_has(&out, "\\t") == NPOS) {
        pw_fail(env, string_view("writeToPageJs must escape newline and tab"), &out)
        return
    }
    // A raw newline would terminate the JS statement.
    for(var i = 0u; i < out.size(); i++) {
        if(out.get(i) == '\n' || out.get(i) == '\r') {
            pw_fail(env, string_view("writeToPageJs left a raw newline in the literal"), &out)
            return
        }
    }
}

// The same guarantee through the `std::string_view` impl.
@test
func test_write_to_page_js_view_escapes_too(env : &mut TestEnv) {
    var payload = string("</script><img src=x onerror=alert(1)>")
    var out = pw_js_view(payload.copy())
    if(pw_has(&out, "</script>")) {
        pw_fail(env, string_view("the string_view impl let a closing script tag through"), &out)
        return
    }
}

// A value with nothing dangerous must come out unchanged - no over-escaping that
// would corrupt the prop.
@test
func test_write_to_page_js_leaves_ordinary_values_alone(env : &mut TestEnv) {
    var plain = string("Search tasks")
    var out = pw_js(plain.copy())
    if(!pw_has(&out, "Search tasks")) {
        pw_fail(env, string_view("an ordinary value must pass through unchanged"), &out)
        return
    }
    if(pw_has(&out, "\\u003C")) {
        pw_fail(env, string_view("an ordinary value must not gain escapes"), &out)
        return
    }
    // A lone `<` is inert inside a script and must not be mangled.
    var lt_in = string("a < b")
    var lt = pw_js(lt_in.copy())
    if(!pw_has(&lt, "a < b")) {
        pw_fail(env, string_view("a lone `<` must not be escaped"), &lt)
        return
    }
}

@test
func test_write_to_page_css_escapes_a_closing_style_tag(env : &mut TestEnv) {
    var payload = string("</style><script>window.__xss=1</script>")
    var out = pw_css(payload.copy())
    if(pw_has(&out, "</style>")) {
        pw_fail(env, string_view("writeToPageCss let a closing style tag through"), &out)
        return
    }
}