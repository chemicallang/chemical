// "@(expr)" is a compile-time construct of the #html macro. The runtime html
// parser has no chemical mode at all -- "@{ ... }" is already plain text there
// (see tokenizer.ch) -- so "@( ... )" must be plain text too, and it must
// survive a parse/serialise round trip unchanged.
//
// The practical consequence: markup produced by the runtime parser, and markup
// handed to it, is unaffected by the sigil. A document that happens to contain
// "@(" is not reinterpreted.

@test
public func test_html_paren_value_is_plain_text(env : &mut TestEnv) {
    var input = std::string_view("<div>@(value)</div>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_paren_value_in_pre_is_plain_text(env : &mut TestEnv) {
    var input = std::string_view("<pre>let v = @(v);</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_paren_value_among_braces_in_pre(env : &mut TestEnv) {
    var input = std::string_view("<pre>if (n > @(v)) { return 1; }</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_paren_value_with_nested_parens(env : &mut TestEnv) {
    var input = std::string_view("<pre>v = @(add(1, 2));</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_at_sign_and_paren_are_not_merged(env : &mut TestEnv) {
    // '@' then '(' must not be reinterpreted as anything; it is just text
    var input = std::string_view("<p>a @ b (c)</p>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_brace_statements_are_also_plain_text_at_runtime(env : &mut TestEnv) {
    // the pre-existing behaviour, pinned next to the new sigil so the two are
    // not confused: neither "@{ }" nor "@( )" means anything at runtime
    var input = std::string_view("<div>@{ for(var i = 0; i < 2; i++) { } }</div>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_bare_braces_in_text_are_preserved(env : &mut TestEnv) {
    // regression for the data loss this file's other cases exposed: the runtime
    // parser has no chemical mode, so "{ x }" in text used to come back as
    // " x " with both braces silently dropped
    var input = std::string_view("<div>{ x }</div>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_braces_around_text_in_pre_keep_their_spaces(env : &mut TestEnv) {
    var input = std::string_view("<pre>{ }</pre>")
    test_html_roundtrip(env, &input)
}

@test
public func test_html_at_if_is_plain_text_at_runtime(env : &mut TestEnv) {
    // control flow is a macro-only construct too
    var input = std::string_view("<div>@if(x) { y }</div>")
    test_html_roundtrip(env, &input)
}
