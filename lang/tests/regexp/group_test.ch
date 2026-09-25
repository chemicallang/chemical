// Groups and alternation.

@test
public func regex_alternation(env : &mut TestEnv) {
    var re = regex::compile("cat|dog")
    rx_match(env, &re, "cat", true, "alternation matches cat")
    rx_match(env, &re, "dog", true, "alternation matches dog")
    rx_match(env, &re, "bird", false, "alternation rejects bird")
}

@test
public func regex_alternation_three_ways(env : &mut TestEnv) {
    var re = regex::compile("red|green|blue")
    rx_match(env, &re, "red", true, "three-way alternation red")
    rx_match(env, &re, "green", true, "three-way alternation green")
    rx_match(env, &re, "blue", true, "three-way alternation blue")
    rx_match(env, &re, "yellow", false, "three-way alternation rejects yellow")
}

@test
public func regex_alternation_inside_text(env : &mut TestEnv) {
    var re = regex::compile("cat|dog")
    rx_match(env, &re, "I have a dog", true, "alternation found inside text")
    rx_match(env, &re, "I have a cat and a dog", true, "alternation found among words")
    rx_match(env, &re, "I have a bird", false, "alternation absent")
}

@test
public func regex_group_plus(env : &mut TestEnv) {
    var re = regex::compile("(ab)+")
    rx_match(env, &re, "ab", true, "group plus matches ab")
    rx_match(env, &re, "abab", true, "group plus matches abab")
    rx_match(env, &re, "a", false, "group plus rejects a")
}

@test
public func regex_nested_groups(env : &mut TestEnv) {
    var re = regex::compile("((a)(b))+")
    rx_match(env, &re, "ab", true, "nested groups match ab")
    rx_match(env, &re, "abab", true, "nested groups match abab")
    rx_match(env, &re, "ba", false, "nested groups reject ba")
}

@test
public func regex_group_email_like(env : &mut TestEnv) {
    var re = regex::compile("(\\w+)@(\\w+)\\.(\\w+)")
    rx_match(env, &re, "a@b.com", true, "email-like pattern matches")
    rx_match(env, &re, "first.last@example.org", true, "email-like pattern in text")
    rx_match(env, &re, "no-at-sign", false, "email-like pattern rejects no-at-sign")
}

@test
public func regex_group_time_pattern(env : &mut TestEnv) {
    var re = regex::compile("([0-9]+):([0-9]+)")
    rx_match(env, &re, "12:34", true, "time pattern matches")
    rx_match(env, &re, "at 9:5 today", true, "time pattern found inside text")
    rx_match(env, &re, "abc", false, "time pattern rejects letters")
}

@test
public func regex_group_leading_pipe_is_literal(env : &mut TestEnv) {
    // A leading '|' is consumed as a literal char by the parser, so `|a`
    // matches the text "|a" and not an empty alternative followed by `a`.
    var re = regex::compile("|a")
    rx_match(env, &re, "|a", true, "leading pipe parsed as a literal")
    rx_match(env, &re, "a", false, "leading pipe is not an empty alternative")
}

@test
public func regex_group_alternation_prefix_order(env : &mut TestEnv) {
    var re = regex::compile("foo|foobar")
    rx_match(env, &re, "foobar", true, "first alternative prefix still matches")
    rx_match(env, &re, "foo", true, "first alternative exact match")
}

@test
public func regex_group_nested_alternation(env : &mut TestEnv) {
    var re = regex::compile("(a|(bc))+")
    rx_match(env, &re, "a", true, "nested alternation matches a")
    rx_match(env, &re, "bc", true, "nested alternation matches bc")
    rx_match(env, &re, "abc", true, "nested alternation matches abc")
    rx_match(env, &re, "d", false, "nested alternation rejects d")
}
