// "@(expr)" is the explicit form of a chemical value.
//
// It exists because "{expr}" cannot mean "chemical expression" everywhere at
// once. Inside <pre> a bare '{' is a literal character, so a code sample can be
// written without escaping anything -- but a value still has to be writable
// there. "@( ... )" is that form, and it is accepted everywhere in the macro so
// that there is exactly one rule to learn rather than one rule per context:
//
//     {expr}     chemical value, the usual form
//     @{ ... }   chemical statements
//     @(expr)    chemical value, explicit, works inside <pre> too
//
// The sigil is '@' followed by '('. That pair does not occur in any programming
// language: every annotation syntax requires a name after '@' (Java, Kotlin,
// C#, Scala, Dart, Swift, JS/TS, Python, Ruby), and the only near miss is PHP's
// error-suppression operator applied to a parenthesised expression, which
// nobody writes. So an '@(' inside displayed code is a real sigil and never a
// false positive.
//
// The region is a closed span: it ends at its matching ')' and never touches
// the brace depth. That is what lets it appear inside <pre>, where a '}' is
// text, and it is why the counter used here is paren_count and not lb_count.

public struct Pair {
    var a : i32
    var b : i32
}

func add2(x : i32, y : i32) : i32 {
    return x + y
}

func sum_pair(p : Pair) : i32 {
    return p.a + p.b
}

// ------------------------------------------------------- inside <pre>

@test
public func paren_value_works_inside_pre(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 42
    #html {
        <pre>let v = @(v);</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>let v = 42;</pre>");
}

@test
public func paren_value_works_among_literal_braces_in_pre(env : &mut TestEnv) {
    // the case this sigil exists for: real code with real braces, plus one
    // value spliced in
    var page = HtmlPage()
    var v = 7
    #html {
        <pre>if (n > @(v)) { return 1; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (n &gt; 7) { return 1; }</pre>");
}

@test
public func two_paren_values_in_one_pre(env : &mut TestEnv) {
    var page = HtmlPage()
    var a = 2
    var b = 3
    #html {
        <pre>f(@(a)) + g(@(b))</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>f(2) + g(3)</pre>");
}

@test
public func paren_value_with_nested_parens_in_pre(env : &mut TestEnv) {
    // the inner ')' of the call must not close the region; paren_count starts at
    // 1 for the '@(' itself so only the second ')' ends it
    var page = HtmlPage()
    #html {
        <pre>result = @(add2(1, 2));</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>result = 3;</pre>");
}

@test
public func paren_value_with_a_struct_literal_in_pre(env : &mut TestEnv) {
    // a '}' inside the expression belongs to the struct literal, and a ')' is
    // consumed by the call. Neither may be read as the end of the region: the
    // struct's '}' brings lb_count back to chem_start_lb, which is why the
    // RBrace path carries a !in_paren_value guard. sum_pair is used so that the
    // expression yields a scalar (a struct has no HTML serialisation).
    var page = HtmlPage()
    #html {
        <pre>v = @(sum_pair(Pair{a: 1, b: 2}));</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>v = 3;</pre>");
}

@test
public func paren_value_inside_an_if_block_in_pre(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = true
    var v = 5
    #html {
        <pre>@if(condition) { n = @(v); }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>n = 5; </pre>");
}

@test
public func statements_still_work_in_pre_alongside_paren_values(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>x = @{ for(var i = 0; i < 2; i++) { #html { <b>1</b> } } };</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>x = <b>1</b><b>1</b>;</pre>");
}

@test
public func a_bare_brace_value_is_still_literal_text_in_pre(env : &mut TestEnv) {
    // pins the documented trade-off: inside <pre> the implicit form does not
    // interpolate, which is the whole reason "@( )" exists
    var page = HtmlPage()
    var v = 9
    #html {
        <pre>{v}</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>{v}</pre>");
}

@test
public func markup_after_a_paren_value_in_pre_survives(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 3
    #html {
        <pre>n = @(v);</pre>
        <p>after</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>n = 3;</pre><p>after</p>");
}

// ------------------------------------------------------ outside <pre>

@test
public func paren_value_works_in_normal_markup(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 9
    #html {
        <div>@(v)</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>9</div>");
}

@test
public func implicit_and_explicit_values_agree(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 4
    #html {
        <div>{v}|@(v)</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>4|4</div>");
}

@test
public func two_paren_values_in_normal_markup(env : &mut TestEnv) {
    var page = HtmlPage()
    var a = 1
    var b = 2
    #html {
        <p>@(a) and @(b)</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>1 and 2</p>");
}

@test
public func paren_value_with_nested_parens_in_normal_markup(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p>@(add2(3, 4))</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>7</p>");
}

@test
public func paren_value_with_a_call_result_in_normal_markup(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <p>sum=@(add2(add2(1, 2), add2(3, 4)))</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>sum=10</p>");
}

@test
public func statements_and_paren_values_together(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 8
    #html {
        <div>@{ for(var i = 0; i < 2; i++) { #html { <b>1</b> } } }@(v)</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><b>1</b><b>1</b>8</div>");
}

@test
public func a_paren_value_does_not_disturb_a_following_brace_value(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 6
    #html {
        <div>@(v){v}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>66</div>");
}

// ------------------------------------------------------------ attributes
//
// Note the unquoted form. An attribute value is lexed as one opaque
// DoubleQuotedValue / SingleQuotedValue token, so a quoted value is plain text
// and never interpolated -- that is true of "{cls}" as well, and predates
// "@( )". Interpolation in an attribute is written class=@(cls), unquoted.

@test
public func paren_value_in_an_unquoted_attribute(env : &mut TestEnv) {
    var page = HtmlPage()
    var cls = "alpha"
    #html {
        <div class=@(cls)>x</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div class=\"alpha\">x</div>");
}

@test
public func paren_value_in_an_unquoted_attribute_matches_the_implicit_form(env : &mut TestEnv) {
    var page = HtmlPage()
    var cls = "beta"
    #html {
        <div class=@(cls)>x</div>
        <div class={cls}>y</div>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<div class=\"beta\">x</div><div class=\"beta\">y</div>");
}

@test
public func paren_values_in_two_unquoted_attributes(env : &mut TestEnv) {
    // two values in one attribute (id=@(a)-@(b)) is not expressible: inside a
    // tag a bare '-' is not lexable, so an unquoted attribute value only
    // accepts a number, a quoted string, or a chemical value
    var page = HtmlPage()
    var first = "a"
    var second = "b"
    #html {
        <div id=@(first) title=@(second)>x</div>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<div id=\"a\" title=\"b\">x</div>");
}

@test
public func comma_separated_paren_values_in_an_unquoted_attribute(env : &mut TestEnv) {
    var page = HtmlPage()
    var a = "x"
    var b = "y"
    #html {
        <div class=@(a, b)>t</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div class=\"x y\">t</div>");
}

@test
public func a_quoted_attribute_value_is_plain_text(env : &mut TestEnv) {
    // documents the limitation above: quoting suppresses interpolation, for
    // both the implicit and the explicit form
    var page = HtmlPage()
    var cls = "alpha"
    #html {
        <div class="@(cls)">x</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div class=\"@(cls)\">x</div>");
}

// ------------------------------------------------- '@' stays harmless

@test
public func an_at_sign_in_text_is_not_a_sigil(env : &mut TestEnv) {
    // an e-mail address: '@' not followed by '{' or '('
    var page = HtmlPage()
    #html {
        <p>mail me at waqas@example.com</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>mail me at waqas@example.com</p>");
}

@test
public func an_at_sign_before_a_name_is_not_a_sigil(env : &mut TestEnv) {
    // a decorator: '@' followed by a letter is the existing @if/@else path
    var page = HtmlPage()
    #html {
        <p>@Override and @app.route</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>@Override and @app.route</p>");
}

@test
public func an_at_sign_inside_pre_is_not_a_sigil(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>@app.route("/x")</pre>
    }
    // text is entity-escaped on output, so the quotes become &quot; in the html
    // and the browser renders the original characters
    string_equals(env, page.toStringHtmlOnly(), "<pre>@app.route(&quot;/x&quot;)</pre>");
}

@test
public func an_at_paren_inside_pre_is_a_sigil_even_next_to_braces(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 11
    #html {
        <pre>if (x) { f(@(v)); }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (x) { f(11); }</pre>");
}
