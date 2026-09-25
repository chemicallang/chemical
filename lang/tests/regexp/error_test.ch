// Compile errors: `compile()` never throws, it records a message in
// `Regex.compile_error`. These tests pin the exact messages.

@test
public func regex_error_unclosed_class(env : &mut TestEnv) {
    rx_error_is(env, "[abc", "unclosed character class", "unclosed class error")
    rx_error_is(env, "[", "unclosed character class", "lone open bracket error")
}

@test
public func regex_error_empty_class(env : &mut TestEnv) {
    // An empty class has no closing bracket, so it is reported as unclosed.
    rx_error_is(env, "[]", "unclosed character class", "empty class is an error")
}

@test
public func regex_error_unmatched_close_paren(env : &mut TestEnv) {
    rx_error(env, "abc)", "unmatched close paren is an error")
    rx_error_is(env, "abc)", "unexpected character at pos 3", "unmatched close paren message")
}

@test
public func regex_error_leading_quantifier(env : &mut TestEnv) {
    rx_error_is(env, "*abc", "nothing to repeat at pos 0", "* cannot start a pattern")
    rx_error_is(env, "+a", "nothing to repeat at pos 0", "+ cannot start a pattern")
    rx_error_is(env, "?a", "nothing to repeat at pos 0", "? cannot start a pattern")
}

@test
public func regex_error_multiple_repeat(env : &mut TestEnv) {
    rx_error_is(env, "a**", "multiple repeat at pos 2", "a** is a double star")
    rx_error_is(env, "a*?", "multiple repeat at pos 2", "a*? lazy star is unsupported")
    rx_error_is(env, "a+*", "multiple repeat at pos 2", "a+* has a star after plus")
    rx_error_is(env, "a??", "multiple repeat at pos 2", "a?? lazy optional is unsupported")
}

@test
public func regex_error_trailing_backslash(env : &mut TestEnv) {
    rx_error_is(env, "abc\\", "trailing backslash", "trailing backslash")
    rx_error_is(env, "[a\\", "trailing backslash", "trailing backslash inside a class")
}

@test
public func regex_error_unclosed_group(env : &mut TestEnv) {
    rx_error_is(env, "(abc", "expected ')' at pos 4", "unclosed group")
    rx_error_is(env, "a(b", "expected ')' at pos 3", "unclosed group mid pattern")
}

@test
public func regex_error_empty_alternative(env : &mut TestEnv) {
    rx_error_is(env, "a|", "unexpected end of pattern", "a trailing pipe is an error")
}

@test
public func regex_error_non_capturing_group(env : &mut TestEnv) {
    // (?:...) is not supported; the '?' is seen as a stray quantifier.
    rx_error(env, "(?:a)", "(?:...) is unsupported")
}

@test
public func regex_valid_patterns_compile(env : &mut TestEnv) {
    rx_compiles(env, "abc", "plain literal compiles")
    rx_compiles(env, "a+b*c?", "quantifiers compile")
    rx_compiles(env, "(a|b)+", "groups compile")
    rx_compiles(env, "[a-z0-9_]+", "classes compile")
    rx_compiles(env, "^\\w+$", "anchors and shorthand compile")
    rx_compiles(env, "a{2}", "braces are literal, not an error")
    rx_compiles(env, "", "empty pattern compiles")
    rx_compiles(env, "[]]", "leading close bracket in a class compiles")
    rx_compiles(env, "a\\.b\\\\c", "escaped metacharacters compile")
}

@test
public func regex_error_records_pattern(env : &mut TestEnv) {
    var re = regex::compile("[abc")
    var expected = std::string_view("[abc")
    rx_check(env, re.pattern.equals_view(&expected), "pattern is recorded even on error")
}
