// Inside <pre> the text is displayed verbatim, and the thing people display
// verbatim is source code -- which is full of '{', '}' and '<'. Before this
// lexer mode existed each of those three characters was read as html or
// chemical syntax:
//
//   '{'  opened a chemical value, so "if (x) { return 1; }" tried to parse
//        "return 1;" as a chemical expression and failed with "expected a
//        rbrace after the chemical value";
//   '}'  at brace depth 1 ended the #html macro itself, so the failure was
//        reported hundreds of lines later and everything after the block was
//        dropped from the output;
//   '<'  entered tag mode unconditionally, so "1 < 2" produced a LessThan
//        followed by a raw Number token and "unknown symbol, expected text or
//        element".
//
// The rules now are:
//   * a '<' begins a tag only when followed by '!', '/' or an ASCII letter,
//     which is the html "tag open" state browsers implement;
//   * inside <pre> a '{' that does not open an @if/@else block is text;
//   * inside <pre> a '}' is text while it can be matched to a text '{', and a
//     real close otherwise;
//   * inside <pre> a '<' before a letter is still a tag, so a comparison such
//     as "a<b" is written with the "&lt;" entity, exactly as in html.
//
// KNOWN TRADE-OFF, pinned by the tests below: because a bare '{' inside <pre>
// is now literal text, the implicit value form "<pre>{value}</pre>" no longer
// interpolates -- it prints "{value}". This is the same rule JSX has, where a
// literal brace is written as a string. Control flow (@if / @else) and block
// interpolation (@{ ... }) both still work inside <pre>; see
// pre_supports_if_else_block and pre_supports_block_interpolation. Nothing in
// this repository relied on the implicit form inside <pre>.
//
// Note also that text is entity-escaped on output, so '<' in a code sample
// becomes "&lt;" and '>' becomes "&gt;" in the emitted html. That is correct --
// the browser renders the original characters -- but it means the expected
// strings below contain the entities, not the raw characters.

// ---------------------------------------------------------------- the basics

@test
public func pre_keeps_braces_of_a_literal_if_statement(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>if (x) { return 1; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (x) { return 1; }</pre>");
}

@test
public func pre_keeps_braces_of_a_function_body(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>int add(int a, int b) { return a + b; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>int add(int a, int b) { return a + b; }</pre>");
}

@test
public func pre_keeps_a_brace_that_has_no_opening_brace(env : &mut TestEnv) {
    // this is the case that used to end the whole #html macro
    var page = HtmlPage()
    #html {
        <pre>return 1; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>return 1; }</pre>");
}

@test
public func pre_keeps_an_unbalanced_opening_brace(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>struct S {</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>struct S {</pre>");
}

@test
public func pre_keeps_a_less_than_comparison(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>1 < 2</pre>
    }
    // '<' is escaped on output; the browser still renders "1 < 2"
    string_equals(env, page.toStringHtmlOnly(), "<pre>1 &lt; 2</pre>");
}

@test
public func pre_keeps_a_full_c_function(env : &mut TestEnv) {
    // braces, a less-than and a greater-than in one block of verbatim code
    var page = HtmlPage()
    #html {
        <pre>int cmp(int a, int b) { if (a < b) { return -1; } return 0; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<pre>int cmp(int a, int b) { if (a &lt; b) { return -1; } return 0; }</pre>");
}

@test
public func pre_keeps_a_greater_than_comparison(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>if (a > b) { return 1; }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (a &gt; b) { return 1; }</pre>");
}

@test
public func pre_keeps_a_multiline_code_sample(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>for (int i = 0; i < n; i++) {
    total += i;
}</pre>
    }
    // <pre> keeps every byte of whitespace, which is the point of the element.
    // This file is committed with LF line endings, so the newlines inside the
    // code sample are LF. (to_string.ch is committed with CRLF and its
    // equivalent assertion expects "\r\n" -- a <pre> block faithfully
    // reproduces whatever the source file uses.)
    string_equals(env, page.toStringHtmlOnly(),
        "<pre>for (int i = 0; i &lt; n; i++) {\n    total += i;\n}</pre>");
}

// ------------------------------------------- content after the block survives

@test
public func markup_after_a_pre_with_braces_is_not_swallowed(env : &mut TestEnv) {
    // the regression that made this worth fixing: a '}' in <pre> used to end
    // the macro, so everything after the block was dropped from the output
    var page = HtmlPage()
    #html {
        <pre>if (x) { return 1; }</pre>
        <p>after</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>if (x) { return 1; }</pre><p>after</p>");
}

@test
public func markup_after_a_pre_with_a_stray_brace_is_not_swallowed(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>}</pre>
        <p>after</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>}</pre><p>after</p>");
}

@test
public func a_stray_brace_in_pre_does_not_affect_a_later_block(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>}</pre>
        <div>{1 + 1}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>}</pre><div>2</div>");
}

@test
public func sibling_markup_around_pre_is_preserved(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <h1>Title</h1>
        <pre>{ }</pre>
        <h2>Sub</h2>
    }
    string_equals(env, page.toStringHtmlOnly(), "<h1>Title</h1><pre>{ }</pre><h2>Sub</h2>");
}

@test
public func brace_in_pre_does_not_leak_into_the_next_block(env : &mut TestEnv) {
    // leaving <pre> with an unbalanced literal '{' must not change how the
    // following braces are read
    var page = HtmlPage()
    #html {
        <pre>{ unbalanced</pre>
        <div>{1 + 1}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>{ unbalanced</pre><div>2</div>");
}

// ------------------------------- control flow mixed with literal braces

@test
public func pre_if_block_may_contain_literal_braces(env : &mut TestEnv) {
    // this is the case the pre_brace_depth counter exists for: inside the
    // @if body the literal "{" and "}" must pair with each other, and the "}"
    // that closes the @if block must still close the block
    var page = HtmlPage()
    var condition = true
    #html {
        <pre>@if(condition) { f() { return 1; } }</pre>
    }
    // the space before the block's own '}' belongs to the block, so it is kept
    string_equals(env, page.toStringHtmlOnly(), "<pre>f() { return 1; } </pre>");
}

@test
public func pre_if_else_block_may_contain_literal_braces(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = true
    #html {
        <pre>@if(condition) { a() { x(); } } @else { b() { y(); } }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>a() { x(); } </pre>");
}

@test
public func pre_if_else_block_with_literal_braces_takes_the_else_branch(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = false
    #html {
        <pre>@if(condition) { a() { x(); } } @else { b() { y(); } }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>b() { y(); } </pre>");
}

@test
public func pre_markup_inside_if_block_still_renders(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = true
    #html {
        <pre>@if(condition) { <span>yes</span> }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre><span>yes</span></pre>");
}

// ------------------------------------------------ interpolation in and out

@test
public func pre_supports_block_interpolation(env : &mut TestEnv) {
    // "@{ ... }" is the nested-statement form, and it still works in <pre>.
    // Html inside the loop is itself a nested #html block, so it gets its own
    // lexer and its own (zero) <pre> depth.
    var page = HtmlPage()
    #html {
        <pre>x = @{ for(var i = 0; i < 2; i++) { #html { <b>1</b> } } };</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>x = <b>1</b><b>1</b>;</pre>");
}

@test
public func pre_supports_interpolation_among_literal_braces(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <pre>@if(true) { f() { <b>x</b>; } }</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>f() { <b>x</b>; } </pre>");
}

@test
public func interpolation_outside_pre_is_still_implicit(env : &mut TestEnv) {
    // the ordinary form must be untouched by any of this
    var page = HtmlPage()
    var v = 9
    #html {
        <div>{v}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>9</div>");
}

@test
public func implicit_interpolation_after_a_pre_still_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var v = 5
    #html {
        <pre>{ }</pre>
        <div>{v}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>{ }</pre><div>5</div>");
}

@test
public func a_bare_brace_in_pre_is_literal_not_an_interpolation(env : &mut TestEnv) {
    // pins the documented trade-off: inside <pre> a bare '{' is text
    var page = HtmlPage()
    var v = 9
    #html {
        <pre>{v}</pre>
    }
    string_equals(env, page.toStringHtmlOnly(), "<pre>{v}</pre>");
}

// ------------------------------------------------ outside <pre> is unaffected

@test
public func less_than_comparison_outside_pre_is_text(env : &mut TestEnv) {
    // the "tag open" rule is not limited to <pre>; a bare '<' in text is text
    var page = HtmlPage()
    #html {
        <p>1 < 2</p>
    }
    string_equals(env, page.toStringHtmlOnly(), "<p>1 &lt; 2</p>");
}

@test
public func tags_adjacent_to_text_still_parse(env : &mut TestEnv) {
    // a '<' followed by a letter is still a tag
    var page = HtmlPage()
    #html {
        <div><span>a</span><span>b</span></div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><span>a</span><span>b</span></div>");
}

@test
public func end_tag_still_lexes_after_text(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>text<span>x</span></div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>text<span>x</span></div>");
}

@test
public func script_raw_text_is_unaffected(env : &mut TestEnv) {
    // <script> is raw text and is NOT entity-escaped; <pre> is not raw text
    // and IS. Both behaviours have to hold at once.
    var page = HtmlPage()
    #html {
        <pre>a &lt; b</pre>
        <script>var s = "a & b";</script>
    }
    string_equals(env, page.toStringHtmlOnly(),
        "<pre>a &lt; b</pre><script>var s = \"a & b\";</script>");
}
