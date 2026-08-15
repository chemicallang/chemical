// Tests for attribute selectors ([data-variant="..."], [attr^=...], ...),
// length/percentage background-size values, and var() inside color functions.
// These cover fixes in css_parser's selector parser (quoted attribute value
// truncation), value parser (background-size %), and color parser (var()).

@test
public func attribute_selector_full_value_preserved(env : &mut TestEnv) {
    // Regression: quoted attribute values were truncated by one char
    // (subview used an exclusive end as a length), so "destructive" became
    // "destructiv". The full value must round-trip.
    var page = HtmlPage()
    #css {
        color : red;
        &[data-variant="destructive"] {
            background-color: red;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{color:red;}")
    expected.append_view(&classView)
    expected.append_view("[data-variant=\"destructive\"] { background-color:red; }")
    compl_css_equals(env, &got, expected.to_view())
}

@test
public func attribute_selector_operators_work(env : &mut TestEnv) {
    // Prefix ^=, suffix $=, substring *=, word ~=, dash |= operators with
    // quoted values must all parse and serialize intact.
    var page = HtmlPage()
    #css {
        color : red;
        &[data-size^="sm"] { width: 2rem; }
        &[data-type$="e"] { width: 3rem; }
        &[data-x*="mid"] { width: 4rem; }
        &[data-y~="word"] { width: 5rem; }
        &[data-z|="pre"] { width: 6rem; }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{color:red;}")
    expected.append_view(&classView)
    expected.append_view("[data-size^=\"sm\"] { width:2rem; }")
    expected.append_view(&classView)
    expected.append_view("[data-type$=\"e\"] { width:3rem; }")
    expected.append_view(&classView)
    expected.append_view("[data-x*=\"mid\"] { width:4rem; }")
    expected.append_view(&classView)
    expected.append_view("[data-y~=\"word\"] { width:5rem; }")
    expected.append_view(&classView)
    expected.append_view("[data-z|=\"pre\"] { width:6rem; }")
    compl_css_equals(env, &got, expected.to_view())
}

@test
public func attribute_selector_without_value_works(env : &mut TestEnv) {
    // Bare presence selectors like [disabled] must parse and serialize.
    var page = HtmlPage()
    #css {
        color : red;
        &[disabled] {
            opacity: 0.5;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{color:red;}")
    expected.append_view(&classView)
    expected.append_view("[disabled] { opacity:0.5; }")
    compl_css_equals(env, &got, expected.to_view())
}

@test
public func background_size_percent_and_length_second_value_works(env : &mut TestEnv) {
    // background-size accepts keyword | length | percentage for each of its
    // one/two values; a % as the second value must not be rejected.
    var page = HtmlPage()
    #css {
        background-size: 50% 100%;
        background-size: 12px 34%;
        background-size: contain;
    }
    css_equals(env, page.toStringCssOnly(), "background-size:50% 100%;background-size:12px 34%;background-size:contain;")
}

@test
public func css_variable_inside_color_function_works(env : &mut TestEnv) {
    // hsl(var(--primary) / 0.9) style values: the color parser must accept
    // var(...) inside a color function and preserve it verbatim.
    var page = HtmlPage()
    #css {
        background-color: hsl(var(--primary) / 0.9);
        color: hsl(var(--foreground));
    }
    css_equals(env, page.toStringCssOnly(), "background-color:hsl(var(--primary) / 0.9);color:hsl(var(--foreground));")
}
