// replace(text, replacement) — replaces every non-overlapping match.
//
// Backreferences are `$N` or `\N`. Only a single digit is consumed; a
// backreference to an unmatched group (or an out-of-range group) expands to
// the empty string.

@test
public func regex_replace_all_matches(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    rx_replace(env, &re, "abc123def45", "#", "abc#def#", "replace all digit runs")
}

@test
public func regex_replace_no_match_keeps_text(env : &mut TestEnv) {
    var re = regex::compile("x")
    rx_replace(env, &re, "abc", "Z", "abc", "replace without a match keeps the original")
}

@test
public func regex_replace_every_occurrence(env : &mut TestEnv) {
    var re = regex::compile("a")
    rx_replace(env, &re, "aaa", "X", "XXX", "replace every occurrence")
    rx_replace(env, &re, "banana", "X", "bXnXnX", "replace inside a word")
}

@test
public func regex_replace_supports_backreferences(env : &mut TestEnv) {
    var re = regex::compile("(\\w+)-(\\w+)")
    rx_replace(env, &re, "left-right", "$2:$1", "right:left", "dollar backreferences")
    rx_replace(env, &re, "left-right", "\\2:\\1", "right:left", "backslash backreferences")
}

@test
public func regex_replace_full_match_backref(env : &mut TestEnv) {
    var re = regex::compile("x")
    rx_replace(env, &re, "axb", "[$0]", "a[x]b", "$0 expands to the full match")
}

@test
public func regex_replace_group_backref(env : &mut TestEnv) {
    var re = regex::compile("(\\w+)")
    rx_replace(env, &re, "ab-cd", "<$1>", "<ab>-<cd>", "group backreference per match")
    rx_replace(env, &re, "ab", "[$1]", "[ab]", "group backreference around a word")
}

@test
public func regex_replace_out_of_range_backref(env : &mut TestEnv) {
    var re = regex::compile("x")
    rx_replace(env, &re, "axb", "$9", "ab", "out-of-range backreference expands to nothing")
}

@test
public func regex_replace_single_digit_consumed(env : &mut TestEnv) {
    // Only one digit is consumed for a backreference; the rest stays literal.
    var re = regex::compile("(\\w+)")
    rx_replace(env, &re, "ab", "$10", "ab0", "$10 is $1 followed by a literal 0")
}

@test
public func regex_replace_non_digit_dollar_is_literal(env : &mut TestEnv) {
    var re = regex::compile("x")
    rx_replace(env, &re, "axb", "$x", "a$xb", "$ followed by a non-digit is literal")
    rx_replace(env, &re, "axb", "$$", "a$$b", "$$ is literal")
}

@test
public func regex_replace_empty_pattern(env : &mut TestEnv) {
    var re = regex::compile("")
    rx_replace(env, &re, "ab", "X", "XaXbX", "empty pattern wraps every boundary")
    rx_replace(env, &re, "", "X", "X", "empty pattern on empty text")
}

@test
public func regex_replace_anchored(env : &mut TestEnv) {
    var end = regex::compile("a$")
    rx_replace(env, &end, "a", "A", "A", "anchored end replacement")
    var start = regex::compile("^a")
    rx_replace(env, &start, "abc", "X", "Xbc", "anchored start replacement")
}

@test
public func regex_replace_whitespace_runs(env : &mut TestEnv) {
    var re = regex::compile("\\s+")
    rx_replace(env, &re, "a  b", "_", "a_b", "collapse whitespace runs")
    rx_replace(env, &re, "  lead and trail  ", "_", "_lead_and_trail_", "trim via replace")
}

@test
public func regex_replace_unmatched_group_backref(env : &mut TestEnv) {
    var re = regex::compile("(a)|(b)")
    rx_replace(env, &re, "b", "<$1>", "<>", "unmatched backreference expands to empty")
    rx_replace(env, &re, "a", "<$2>", "<>", "the other unmatched backreference is empty")
}

@test
public func regex_replace_repeated_group(env : &mut TestEnv) {
    var re = regex::compile("(ab)+")
    rx_replace(env, &re, "xababy", "[$1]", "x[ab]y", "repeated group replaced once per run")
}

@test
public func regex_replace_no_match_with_backref(env : &mut TestEnv) {
    var re = regex::compile("(\\w+)-(\\w+)")
    rx_replace(env, &re, "no match here", "$1", "no match here", "no-match input is unchanged")
}
