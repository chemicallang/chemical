// find(text, &start, &end) — leftmost-longest match positions.
//
// Returns false and sets both outputs to -1 when there is no match.

@test
public func regex_find_basic(env : &mut TestEnv) {
    var re = regex::compile("world")
    rx_find(env, &re, "hello world", 6, 11, "find world start/end")
    rx_find(env, &re, "world", 0, 5, "find world at start")
    rx_find_none(env, &re, "hello there", "find world absent")
}

@test
public func regex_find_leftmost(env : &mut TestEnv) {
    var re = regex::compile("ab")
    rx_find(env, &re, "xxabyyabzz", 2, 4, "find returns the leftmost match")
}

@test
public func regex_find_longest(env : &mut TestEnv) {
    var re = regex::compile("a+")
    rx_find(env, &re, "aaab", 0, 3, "find is greedy on length")
    rx_find(env, &re, "baaa", 1, 4, "find skips leading non-matching chars")
}

@test
public func regex_find_non_overlapping(env : &mut TestEnv) {
    var re = regex::compile("aa")
    rx_find(env, &re, "aaaa", 0, 2, "find returns the first non-overlapping match")
}

@test
public func regex_find_digit_run(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    rx_find(env, &re, "ab123cd456", 2, 5, "find the first digit run")
    rx_find(env, &re, "42", 0, 2, "find a digit run at the start")
}

@test
public func regex_find_word_run(env : &mut TestEnv) {
    var re = regex::compile("\\w+")
    rx_find(env, &re, "  hello  world ", 2, 7, "find the first word")
}

@test
public func regex_find_alternation(env : &mut TestEnv) {
    var re = regex::compile("cat|dog")
    rx_find(env, &re, "a dog and a cat", 2, 5, "find picks the leftmost alternative")
    rx_find(env, &re, "a cat and a dog", 2, 5, "find picks the leftmost cat")
}

@test
public func regex_find_group_repeats(env : &mut TestEnv) {
    var re = regex::compile("(ab)+")
    rx_find(env, &re, "zzababab", 2, 8, "find repeated group spans all iterations")
    var pairs = regex::compile("(a+)(b+)")
    rx_find(env, &pairs, "xaabb", 1, 5, "find pair groups span both runs")
    var grp = regex::compile("(ab)(cd)")
    rx_find(env, &grp, "xxabcdyy", 2, 6, "find grouped pair positions")
}

@test
public func regex_find_dot_star(env : &mut TestEnv) {
    var re = regex::compile(".*")
    rx_find(env, &re, "hello", 0, 5, "find .* spans the whole line")
    var anchored = regex::compile("^.*$")
    rx_find(env, &anchored, "hello", 0, 5, "find ^.*$ spans the whole line")
    var ident = regex::compile("^[a-z]+$")
    rx_find(env, &ident, "abc", 0, 3, "find an anchored identifier")
}

@test
public func regex_find_empty_pattern(env : &mut TestEnv) {
    var re = regex::compile("")
    rx_find(env, &re, "abc", 0, 0, "find empty pattern at the start")
    rx_find(env, &re, "", 0, 0, "find empty pattern in empty text")
}

@test
public func regex_find_anchors(env : &mut TestEnv) {
    var caret = regex::compile("^")
    rx_find(env, &caret, "abc", 0, 0, "find ^ yields a zero-width match at the start")
    var caret_b = regex::compile("^b")
    rx_find_none(env, &caret_b, "abc", "find anchored ^b fails")
    var b_end = regex::compile("b$")
    rx_find(env, &b_end, "ab", 1, 2, "find b$ at the end")
    var dollar = regex::compile("$")
    rx_find(env, &dollar, "ab", 2, 2, "find $ yields a zero-width match at the end")
}

@test
public func regex_find_no_match_sets_sentinels(env : &mut TestEnv) {
    var re = regex::compile("xyz")
    rx_find_none(env, &re, "abc", "find no match sets both outputs to -1")
}

@test
public func regex_find_whitespace(env : &mut TestEnv) {
    var re = regex::compile("\\s")
    rx_find(env, &re, "ab cd", 2, 3, "find the first whitespace char")
}

@test
public func regex_find_class_in_text(env : &mut TestEnv) {
    var re = regex::compile("[0-9]+")
    rx_find(env, &re, "abc 42 def 7", 4, 6, "find the first number")
}
