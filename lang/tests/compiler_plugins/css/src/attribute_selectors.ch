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
    // A `#css` block in statement position is a global stylesheet: its own
    // declarations go to :root and `&` anchors to the document root.
    compl_css_equals(env, &got, ":root{color:red;}:root[data-variant=\"destructive\"] { background-color:red; }")
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
    // Global block: the block's declarations land on :root and every `&`
    // anchored rule resolves to the document root.
    compl_css_equals(env, &got, ":root{color:red;}:root[data-size^=\"sm\"] { width:2rem; }:root[data-type$=\"e\"] { width:3rem; }:root[data-x*=\"mid\"] { width:4rem; }:root[data-y~=\"word\"] { width:5rem; }:root[data-z|=\"pre\"] { width:6rem; }")
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
    compl_css_equals(env, &got, ":root{color:red;}:root[disabled] { opacity:0.5; }")
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

@test
public func attribute_selector_with_pseudo_element_stays_compound(env : &mut TestEnv) {
    // Regression: `&[data-variant="error"]::-webkit-progress-value` was parsed
    // as TWO compounds with a descendant combinator (a space appeared between
    // `]` and `::`), breaking the selector. The attribute selector must keep
    // the following pseudo-element in the same compound.
    var page = HtmlPage()
    #css {
        color : red;
        &[data-variant="error"]::-webkit-progress-value {
            background: blue;
        }
        &[data-variant="error"]:hover {
            color: green;
        }
    }
    var got = page.toStringCssOnly();
    // The attribute selector must stay in the same compound as the pseudo
    // element, anchored to the document root in a global block.
    compl_css_equals(env, &got, ":root{color:red;}:root[data-variant=\"error\"]::-webkit-progress-value { background: blue; }:root[data-variant=\"error\"]:hover { color:green; }")
}
