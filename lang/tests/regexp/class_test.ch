// Character classes: [abc], ranges, negation and escape shorthands.

@test
public func regex_class_simple(env : &mut TestEnv) {
    var re = regex::compile("[abc]")
    rx_match(env, &re, "a", true, "[abc] matches a")
    rx_match(env, &re, "b", true, "[abc] matches b")
    rx_match(env, &re, "c", true, "[abc] matches c")
    rx_match(env, &re, "d", false, "[abc] rejects d")
}

@test
public func regex_class_range(env : &mut TestEnv) {
    var re = regex::compile("[a-z]")
    rx_match(env, &re, "a", true, "[a-z] matches a")
    rx_match(env, &re, "m", true, "[a-z] matches m")
    rx_match(env, &re, "z", true, "[a-z] matches z")
    rx_match(env, &re, "A", false, "[a-z] rejects uppercase")
    rx_match(env, &re, "3", false, "[a-z] rejects a digit")
}

@test
public func regex_class_multiple_ranges(env : &mut TestEnv) {
    var re = regex::compile("[a-zA-Z0-9_]")
    rx_match(env, &re, "Q", true, "multi-range matches uppercase")
    rx_match(env, &re, "7", true, "multi-range matches digit")
    rx_match(env, &re, "_", true, "multi-range matches underscore")
    rx_match(env, &re, "-", false, "multi-range rejects dash")
}

@test
public func regex_class_negated(env : &mut TestEnv) {
    var re = regex::compile("[^abc]")
    rx_match(env, &re, "d", true, "[^abc] matches d")
    rx_match(env, &re, "z", true, "[^abc] matches z")
    rx_match(env, &re, "a", false, "[^abc] rejects a")
}

@test
public func regex_class_reversed_range(env : &mut TestEnv) {
    // Reversed ranges are normalized by the compiler.
    var re = regex::compile("[z-a]")
    rx_match(env, &re, "m", true, "[z-a] normalized to a-z")
    rx_match(env, &re, "3", false, "[z-a] rejects a digit")
}

@test
public func regex_class_digit_shorthand(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    rx_match(env, &re, "123", true, "\\d+ matches digits")
    rx_match(env, &re, "abc", false, "\\d+ rejects letters")
}

@test
public func regex_class_word_shorthand(env : &mut TestEnv) {
    var re = regex::compile("\\w+")
    rx_match(env, &re, "hello", true, "\\w+ matches a word")
    rx_match(env, &re, "abc123", true, "\\w+ matches alnum")
    rx_match(env, &re, "a_b", true, "\\w+ matches underscore")
    rx_match(env, &re, "!", false, "\\w+ rejects punctuation")
}

@test
public func regex_class_space_shorthand(env : &mut TestEnv) {
    var re = regex::compile("\\s+")
    rx_match(env, &re, "   ", true, "\\s+ matches spaces")
    rx_match(env, &re, "\t", true, "\\s+ matches a tab")
    rx_match(env, &re, "abc", false, "\\s+ rejects letters")
}

@test
public func regex_class_negated_shorthands(env : &mut TestEnv) {
    var non_digit = regex::compile("\\D")
    rx_match(env, &non_digit, "a", true, "\\D matches a letter")
    rx_match(env, &non_digit, "1", false, "\\D rejects a digit")
    var non_word = regex::compile("\\W")
    rx_match(env, &non_word, "!", true, "\\W matches punctuation")
    rx_match(env, &non_word, "a", false, "\\W rejects a word char")
    var non_space = regex::compile("\\S")
    rx_match(env, &non_space, "x", true, "\\S matches a letter")
    rx_match(env, &non_space, " ", false, "\\S rejects a space")
}

@test
public func regex_class_shorthand_inside_brackets(env : &mut TestEnv) {
    var digits = regex::compile("[\\d]+")
    rx_match(env, &digits, "12345", true, "[\\d]+ matches digits")
    rx_match(env, &digits, "abc", false, "[\\d]+ rejects letters")
    var words = regex::compile("[\\w]+")
    rx_match(env, &words, "abc_123", true, "[\\w]+ matches word chars")
    var spaces = regex::compile("[\\s]+")
    rx_match(env, &spaces, " \t", true, "[\\s]+ matches whitespace")
    var non_digit = regex::compile("[\\D]")
    rx_match(env, &non_digit, "a", true, "[\\D] matches a non-digit")
    rx_match(env, &non_digit, "1", false, "[\\D] rejects a digit")
}

@test
public func regex_class_negated_range(env : &mut TestEnv) {
    var re = regex::compile("[^0-9]")
    rx_match(env, &re, "a", true, "[^0-9] matches a letter")
    rx_match(env, &re, "5", false, "[^0-9] rejects a digit")
}

@test
public func regex_class_leading_dash_literal(env : &mut TestEnv) {
    var re = regex::compile("[-a]")
    rx_match(env, &re, "-", true, "[-a] matches a dash")
    rx_match(env, &re, "a", true, "[-a] matches a")
    rx_match(env, &re, "b", false, "[-a] rejects b")
}

@test
public func regex_class_trailing_dash_literal(env : &mut TestEnv) {
    var re = regex::compile("[a-]")
    rx_match(env, &re, "a", true, "[a-] matches a")
    rx_match(env, &re, "-", true, "[a-] matches a dash")
    rx_match(env, &re, "b", false, "[a-] rejects b")
}

@test
public func regex_class_leading_close_bracket_literal(env : &mut TestEnv) {
    var re = regex::compile("[]a]")
    rx_match(env, &re, "]", true, "[]a] matches a close bracket")
    rx_match(env, &re, "a", true, "[]a] matches a")
    rx_match(env, &re, "b", false, "[]a] rejects b")
}

@test
public func regex_class_escaped_chars(env : &mut TestEnv) {
    var re = regex::compile("[\\t\\n]")
    rx_match(env, &re, "\t", true, "class matches an escaped tab")
    rx_match(env, &re, "\n", true, "class matches an escaped newline")
    rx_match(env, &re, "a", false, "class rejects a letter")
}

@test
public func regex_class_with_quantifier(env : &mut TestEnv) {
    var re = regex::compile("[0-9]+\\.[0-9]+")
    rx_match(env, &re, "3.14", true, "class pattern matches a decimal")
    rx_match(env, &re, "3.14.15", true, "class pattern finds a decimal inside text")
    rx_match(env, &re, "abc", false, "class pattern rejects letters")
}
