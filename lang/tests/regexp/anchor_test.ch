// Anchors: ^ and $.

@test
public func regex_anchor_start(env : &mut TestEnv) {
    var re = regex::compile("^hello")
    rx_match(env, &re, "hello", true, "^hello matches at the start")
    rx_match(env, &re, "hello world", true, "^hello matches a prefix")
    rx_match(env, &re, "say hello", false, "^hello rejects a mid-string match")
}

@test
public func regex_anchor_end(env : &mut TestEnv) {
    var re = regex::compile("world$")
    rx_match(env, &re, "world", true, "world$ matches on its own")
    rx_match(env, &re, "hello world", true, "world$ matches a suffix")
    rx_match(env, &re, "world!", false, "world$ rejects a trailing char")
}

@test
public func regex_anchor_both(env : &mut TestEnv) {
    var re = regex::compile("^hello$")
    rx_match(env, &re, "hello", true, "^hello$ full match")
    rx_match(env, &re, "hello ", false, "^hello$ rejects a trailing space")
    rx_match(env, &re, " hello", false, "^hello$ rejects a leading space")
}

@test
public func regex_anchor_empty_string(env : &mut TestEnv) {
    var caret = regex::compile("^")
    rx_match(env, &caret, "", true, "^ matches empty text")
    rx_match(env, &caret, "abc", false, "^ alone does not match non-empty text")
    var both = regex::compile("^$")
    rx_match(env, &both, "", true, "^$ matches empty")
    rx_match(env, &both, "abc", false, "^$ rejects non-empty")
    var dollar = regex::compile("$")
    rx_match(env, &dollar, "abc", true, "$ matches the end of any text")
}

@test
public func regex_anchor_dot_star(env : &mut TestEnv) {
    var re = regex::compile("^.*$")
    rx_match(env, &re, "hello", true, "^.*$ matches a whole line")
    rx_match(env, &re, "", true, "^.*$ matches an empty line")
}

@test
public func regex_anchor_identifier(env : &mut TestEnv) {
    var re = regex::compile("^[a-zA-Z_][a-zA-Z0-9_]*$")
    rx_match(env, &re, "Hello", true, "identifier: Hello")
    rx_match(env, &re, "var123", true, "identifier: var123")
    rx_match(env, &re, "_x", true, "identifier: _x")
    rx_match(env, &re, "123abc", false, "identifier: leading digit rejected")
    rx_match(env, &re, "", false, "identifier: empty rejected")
}

@test
public func regex_anchor_with_alternation(env : &mut TestEnv) {
    var re = regex::compile("^(cat|dog)$")
    rx_match(env, &re, "cat", true, "anchored alternation cat")
    rx_match(env, &re, "dog", true, "anchored alternation dog")
    rx_match(env, &re, "catdog", false, "anchored alternation rejects combined")
    rx_match(env, &re, "a cat", false, "anchored alternation rejects prefix")
}
