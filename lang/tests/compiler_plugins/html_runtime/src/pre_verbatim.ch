// The runtime html parser shares getNextToken2() with the #html macro, so the
// <pre> rules are the same on both sides: inside <pre> a '{', a '}' and a '<'
// that cannot begin a tag are ordinary text, not syntax.
//
// These cases are worth pinning here separately because the runtime tokenizer
// differs from the macro in one important way: it sets preserve_whitespace, so
// the whitespace branch in the lexer takes its other path, and the '@{...}'
// chemical modes are not available at all (tokenizer.ch) -- so <pre> at
// runtime can only ever contain literal text.

@test
public func test_html_pre_keeps_braces(env : &mut TestEnv) {
    var input = std::string_view("<pre>if (x) { return 1; }</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_pre_keeps_stray_closing_brace(env : &mut TestEnv) {
    var input = std::string_view("<pre>return 1; }</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_pre_keeps_unbalanced_opening_brace(env : &mut TestEnv) {
    var input = std::string_view("<pre>struct S {</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_pre_keeps_less_than_comparison(env : &mut TestEnv) {
    var input = std::string_view("<pre>1 < 2</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_pre_keeps_a_code_sample(env : &mut TestEnv) {
    var input = std::string_view("<pre>int cmp(int a, int b) { if (a &lt; b) { return -1; } return 0; }</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_pre_preserves_newlines_and_indentation(env : &mut TestEnv) {
    var input = std::string_view("<pre>for (int i = 0; i &lt; n; i++) {\n    total += i;\n}</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_markup_after_pre_with_braces_survives(env : &mut TestEnv) {
    var input = std::string_view("<pre>if (x) { return 1; }</pre><p>after</p>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_sibling_markup_around_pre_is_preserved(env : &mut TestEnv) {
    var input = std::string_view("<h1>Title</h1><pre>{ }</pre><h2>Sub</h2>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_text_with_bare_less_than_is_text(env : &mut TestEnv) {
    // the html "tag open" rule: '<' only starts a tag before '!', '/' or a
    // letter. This holds outside <pre> too.
    var input = std::string_view("<p>1 < 2</p>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_text_with_bare_greater_than_is_text(env : &mut TestEnv) {
    var input = std::string_view("<p>a > b</p>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_nested_pre_keeps_braces(env : &mut TestEnv) {
    // pre_depth counts, and the literal-brace counter is cleared only when the
    // outermost </pre> is reached
    var input = std::string_view("<div><pre>a { b }</pre><p>x</p></div>")
    test_html_roundtrip(env, &input)
}
