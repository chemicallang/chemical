// Quantifiers: *, +, ? and their interactions.

@test
public func regex_star_zero_or_more(env : &mut TestEnv) {
    var re = regex::compile("a*")
    rx_match(env, &re, "", true, "a* matches empty")
    rx_match(env, &re, "a", true, "a* matches a")
    rx_match(env, &re, "aaa", true, "a* matches aaa")
    rx_match(env, &re, "bbb", true, "a* matches the empty run inside bbb")
}

@test
public func regex_star_within_pattern(env : &mut TestEnv) {
    var re = regex::compile("ba*")
    rx_match(env, &re, "b", true, "ba* matches b")
    rx_match(env, &re, "baaa", true, "ba* matches baaa")
    rx_match(env, &re, "cab", true, "ba* finds the b later in the string")
    rx_match(env, &re, "aaa", false, "ba* requires a b somewhere")
}

@test
public func regex_plus_one_or_more(env : &mut TestEnv) {
    var re = regex::compile("a+")
    rx_match(env, &re, "", false, "a+ rejects empty")
    rx_match(env, &re, "a", true, "a+ matches a")
    rx_match(env, &re, "aaa", true, "a+ matches aaa")
    rx_match(env, &re, "bbb", false, "a+ rejects bbb")
}

@test
public func regex_optional(env : &mut TestEnv) {
    var re = regex::compile("ab?c")
    rx_match(env, &re, "ac", true, "ab?c matches ac")
    rx_match(env, &re, "abc", true, "ab?c matches abc")
    rx_match(env, &re, "abbc", false, "ab?c rejects abbc")
}

@test
public func regex_quantifier_on_group(env : &mut TestEnv) {
    var re = regex::compile("(ab)+")
    rx_match(env, &re, "ab", true, "(ab)+ matches ab")
    rx_match(env, &re, "abab", true, "(ab)+ matches abab")
    rx_match(env, &re, "ababab", true, "(ab)+ matches ababab")
    rx_match(env, &re, "a", false, "(ab)+ rejects a")
}

@test
public func regex_star_on_group(env : &mut TestEnv) {
    var re = regex::compile("(ab)*")
    rx_match(env, &re, "", true, "(ab)* matches empty")
    rx_match(env, &re, "ab", true, "(ab)* matches ab")
    rx_match(env, &re, "abab", true, "(ab)* matches abab")
}

@test
public func regex_optional_on_group(env : &mut TestEnv) {
    var re = regex::compile("(ab)?")
    rx_match(env, &re, "", true, "(ab)? matches empty")
    rx_match(env, &re, "ab", true, "(ab)? matches ab")
}

@test
public func regex_star_plus_combined(env : &mut TestEnv) {
    var re = regex::compile("a*b+")
    rx_match(env, &re, "b", true, "a*b+ matches b")
    rx_match(env, &re, "aaab", true, "a*b+ matches aaab")
    rx_match(env, &re, "aaabbb", true, "a*b+ matches aaabbb")
    rx_match(env, &re, "", false, "a*b+ rejects empty")
}

@test
public func regex_nested_quantifier(env : &mut TestEnv) {
    var re = regex::compile("(a+)+")
    rx_match(env, &re, "a", true, "(a+)+ matches a")
    rx_match(env, &re, "aaaa", true, "(a+)+ matches aaaa")
    rx_match(env, &re, "b", false, "(a+)+ rejects b")
}

@test
public func regex_alternation_with_quantifier(env : &mut TestEnv) {
    var re = regex::compile("(ab|cd)+")
    rx_match(env, &re, "ab", true, "(ab|cd)+ matches ab")
    rx_match(env, &re, "cd", true, "(ab|cd)+ matches cd")
    rx_match(env, &re, "abcd", true, "(ab|cd)+ matches abcd")
    rx_match(env, &re, "abcdab", true, "(ab|cd)+ matches abcdab")
    rx_match(env, &re, "xy", false, "(ab|cd)+ rejects xy")
}

@test
public func regex_quantified_alternation_repeats_independently(env : &mut TestEnv) {
    var re = regex::compile("^(a|b)*$")
    rx_match(env, &re, "", true, "(a|b)* matches empty")
    rx_match(env, &re, "abab", true, "(a|b)* matches abab")
    rx_match(env, &re, "bbaa", true, "(a|b)* matches bbaa")
    rx_match(env, &re, "abc", false, "(a|b)* rejects c")
}

@test
public func regex_quantified_class(env : &mut TestEnv) {
    var re = regex::compile("[abc]+")
    rx_match(env, &re, "abcabc", true, "class plus matches repeated chars")
    rx_match(env, &re, "d", false, "class plus rejects other chars")
}

@test
public func regex_optional_char_at_start(env : &mut TestEnv) {
    var re = regex::compile("^a?b$")
    rx_match(env, &re, "b", true, "a?b matches b")
    rx_match(env, &re, "ab", true, "a?b matches ab")
    rx_match(env, &re, "aab", false, "a?b rejects aab")
}

@test
public func regex_multiple_repeat_is_error(env : &mut TestEnv) {
    rx_error(env, "a**", "a** is a multiple-repeat error")
    rx_error(env, "a*?", "a*? is a multiple-repeat error")
    rx_error(env, "a+*", "a+* is a multiple-repeat error")
    rx_error(env, "a??", "a?? is a multiple-repeat error")
    rx_error(env, "a+?", "a+? is a multiple-repeat error")
}
