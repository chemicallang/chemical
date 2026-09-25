// Literal characters, the `.` wildcard, and escape sequences.

@test
public func regex_literal_match(env : &mut TestEnv) {
    var re = regex::compile("hello")
    rx_match(env, &re, "hello", true, "literal: exact match")
    rx_match(env, &re, "hello world", true, "literal: substring match")
    rx_match(env, &re, "say hello!", true, "literal: middle match")
    rx_match(env, &re, "world", false, "literal: absent")
    rx_match(env, &re, "HELLO", false, "literal: case sensitive")
}

@test
public func regex_literal_phrase(env : &mut TestEnv) {
    var re = regex::compile("the quick brown fox")
    rx_match(env, &re, "the quick brown fox jumps", true, "literal phrase present")
    rx_match(env, &re, "the quick brown dog", false, "literal phrase absent")
}

@test
public func regex_literal_empty_pattern(env : &mut TestEnv) {
    var re = regex::compile("")
    rx_match(env, &re, "", true, "empty pattern matches empty text")
    rx_match(env, &re, "anything", true, "empty pattern matches any text")
    rx_check(env, re.compile_error.empty(), "empty pattern compiles cleanly")
}

@test
public func regex_literal_single_char(env : &mut TestEnv) {
    var re = regex::compile("a")
    rx_match(env, &re, "a", true, "single char exact")
    rx_match(env, &re, "banana", true, "single char substring")
    rx_match(env, &re, "b", false, "single char absent")
}

@test
public func regex_literal_long(env : &mut TestEnv) {
    var re = regex::compile("abcdefghijklmnopqrstuvwxyz")
    rx_match(env, &re, "abcdefghijklmnopqrstuvwxyz", true, "long literal exact")
    rx_match(env, &re, "xxabcdefghijklmnopqrstuvwxyzxx", true, "long literal inside text")
    rx_match(env, &re, "abcdefghijklmnopqrstuvwxy", false, "long literal one char short")
}

@test
public func regex_dot_matches_any_char(env : &mut TestEnv) {
    var re = regex::compile(".")
    rx_match(env, &re, "a", true, "dot matches letter")
    rx_match(env, &re, "1", true, "dot matches digit")
    rx_match(env, &re, " ", true, "dot matches space")
    rx_match(env, &re, "", false, "dot requires a char")
}

@test
public func regex_dot_single_width(env : &mut TestEnv) {
    var re = regex::compile("...")
    rx_match(env, &re, "abc", true, "three dots match three chars")
    rx_match(env, &re, "a", false, "three dots need three chars")
    rx_match(env, &re, "abcde", true, "three dots match inside longer text")
}

@test
public func regex_dot_hello_shape(env : &mut TestEnv) {
    var re = regex::compile("h.llo")
    rx_match(env, &re, "hello", true, "h.llo matches hello")
    rx_match(env, &re, "hxllo", true, "h.llo matches hxllo")
    rx_match(env, &re, "h llo", true, "h.llo matches h llo")
    rx_match(env, &re, "hllo", false, "h.llo needs a char in the middle")
}

@test
public func regex_dot_does_not_match_newline(env : &mut TestEnv) {
    var re = regex::compile("a.b")
    rx_match(env, &re, "axb", true, "a.b matches axb")
    rx_match(env, &re, "a\nb", false, "a.b does not cross a newline")
    rx_match(env, &re, "a.b", true, "a.b matches a literal-dot span")
}

@test
public func regex_dot_star_matches_line(env : &mut TestEnv) {
    var re = regex::compile(".*")
    rx_match(env, &re, "hello world", true, ".* matches a line")
    rx_match(env, &re, "", true, ".* matches empty line")
}

@test
public func regex_escape_dot(env : &mut TestEnv) {
    var re = regex::compile("a\\.b")
    rx_match(env, &re, "a.b", true, "escaped dot is literal")
    rx_match(env, &re, "axb", false, "escaped dot does not match other chars")
}

@test
public func regex_escape_backslash(env : &mut TestEnv) {
    var re = regex::compile("\\\\")
    rx_match(env, &re, "\\", true, "escaped backslash matches a backslash")
    rx_match(env, &re, "a", false, "escaped backslash does not match a letter")
}

@test
public func regex_escape_parens(env : &mut TestEnv) {
    var re = regex::compile("\\(\\)")
    rx_match(env, &re, "()", true, "escaped parens match literal parens")
    rx_match(env, &re, "ab", false, "escaped parens do not match letters")
}

@test
public func regex_escape_brackets_and_question(env : &mut TestEnv) {
    var re = regex::compile("\\[abc\\]\\?")
    rx_match(env, &re, "[abc]?", true, "escaped brackets and question match literally")
    rx_match(env, &re, "abc", false, "escaped brackets do not overmatch")
}

@test
public func regex_escape_quantifier_chars(env : &mut TestEnv) {
    var plus = regex::compile("\\+")
    rx_match(env, &plus, "+", true, "escaped plus is literal")
    rx_match(env, &plus, "a", false, "escaped plus is not a quantifier")
    var star = regex::compile("\\*")
    rx_match(env, &star, "*", true, "escaped star is literal")
    rx_match(env, &star, "aaa", false, "escaped star is not a quantifier")
}

@test
public func regex_escape_anchor_chars(env : &mut TestEnv) {
    var caret = regex::compile("\\^")
    rx_match(env, &caret, "^", true, "escaped caret is literal")
    var dollar = regex::compile("\\$")
    rx_match(env, &dollar, "$", true, "escaped dollar is literal")
    var pipe = regex::compile("\\|")
    rx_match(env, &pipe, "|", true, "escaped pipe is literal")
    var brace = regex::compile("\\{")
    rx_match(env, &brace, "{", true, "escaped brace is literal")
}

@test
public func regex_escape_control_chars(env : &mut TestEnv) {
    var nl = regex::compile("\\n")
    rx_match(env, &nl, "\n", true, "\\n matches a newline")
    rx_match(env, &nl, "n", false, "\\n does not match a literal n")
    var tab = regex::compile("a\\tb")
    rx_match(env, &tab, "a\tb", true, "\\t matches a tab")
    var cr = regex::compile("\\r")
    rx_match(env, &cr, "\r", true, "\\r matches a carriage return")
    var ff = regex::compile("\\f")
    rx_match(env, &ff, "\f", true, "\\f matches a form feed")
    var vt = regex::compile("\\v")
    rx_match(env, &vt, "\v", true, "\\v matches a vertical tab")
}

@test
public func regex_escape_unknown_is_literal(env : &mut TestEnv) {
    var re = regex::compile("\\q")
    rx_match(env, &re, "q", true, "unknown escape falls back to the literal char")
    rx_match(env, &re, "x", false, "unknown escape does not match an unrelated char")
}

@test
public func regex_escape_b_is_not_word_boundary(env : &mut TestEnv) {
    // Word boundaries are not implemented: \b is treated as a literal 'b'.
    var re = regex::compile("\\b")
    rx_match(env, &re, "b", true, "\\b is a literal b")
    rx_match(env, &re, " ", false, "\\b is not a word boundary")
}

@test
public func regex_curly_braces_are_literal(env : &mut TestEnv) {
    // {n,m} repetition is not implemented: braces are ordinary characters.
    var re = regex::compile("a{2}")
    rx_match(env, &re, "a{2}", true, "braces are literal")
    rx_match(env, &re, "aa", false, "braces do not repeat")
    rx_check(env, re.compile_error.empty(), "brace pattern compiles cleanly")
}
