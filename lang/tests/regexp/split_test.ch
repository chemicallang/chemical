// split(text, &out) — splits text on every non-overlapping match.
//
// The segments are string_views into the original text (including leading and
// trailing empty segments). Zero-width matches advance one char at a time.

@test
public func regex_split_on_runs(env : &mut TestEnv) {
    var re = regex::compile(",+")
    rx_split_size(env, &re, "a,,b,c", 3, "split on comma runs count")
    rx_split_part(env, &re, "a,,b,c", 0, "a", "split first part")
    rx_split_part(env, &re, "a,,b,c", 1, "b", "split second part")
    rx_split_part(env, &re, "a,,b,c", 2, "c", "split third part")
}

@test
public func regex_split_keeps_edge_empty_segments(env : &mut TestEnv) {
    var re = regex::compile(",")
    rx_split_size(env, &re, ",a,", 3, "split edge part count")
    rx_split_part(env, &re, ",a,", 0, "", "split leading empty segment")
    rx_split_part(env, &re, ",a,", 1, "a", "split middle segment")
    rx_split_part(env, &re, ",a,", 2, "", "split trailing empty segment")
}

@test
public func regex_split_no_match_single_part(env : &mut TestEnv) {
    var re = regex::compile("xyz")
    rx_split_size(env, &re, "abc", 1, "split with no match yields one part")
    rx_split_part(env, &re, "abc", 0, "abc", "split keeps the whole text")
}

@test
public func regex_split_empty_pattern(env : &mut TestEnv) {
    var re = regex::compile("")
    rx_split_size(env, &re, "ab", 5, "empty pattern split count")
    rx_split_part(env, &re, "ab", 0, "", "empty pattern part 0")
    rx_split_part(env, &re, "ab", 1, "a", "empty pattern part 1")
    rx_split_part(env, &re, "ab", 2, "", "empty pattern part 2")
    rx_split_part(env, &re, "ab", 3, "b", "empty pattern part 3")
    rx_split_part(env, &re, "ab", 4, "", "empty pattern part 4")
}

@test
public func regex_split_digits(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    rx_split_size(env, &re, "a1b22c", 3, "split on digit runs count")
    rx_split_part(env, &re, "a1b22c", 0, "a", "digit split part a")
    rx_split_part(env, &re, "a1b22c", 1, "b", "digit split part b")
    rx_split_part(env, &re, "a1b22c", 2, "c", "digit split part c")
}

@test
public func regex_split_whitespace(env : &mut TestEnv) {
    var re = regex::compile("\\s+")
    rx_split_size(env, &re, "a b  c", 3, "whitespace split count")
    rx_split_part(env, &re, "a b  c", 0, "a", "whitespace part a")
    rx_split_part(env, &re, "a b  c", 1, "b", "whitespace part b")
    rx_split_part(env, &re, "a b  c", 2, "c", "whitespace part c")
}

@test
public func regex_split_on_alternation(env : &mut TestEnv) {
    var re = regex::compile(",|;")
    rx_split_size(env, &re, "a,b;c", 3, "alternation split count")
    rx_split_part(env, &re, "a,b;c", 0, "a", "alternation split first")
    rx_split_part(env, &re, "a,b;c", 1, "b", "alternation split middle")
    rx_split_part(env, &re, "a,b;c", 2, "c", "alternation split last")
}

@test
public func regex_split_leading_separator(env : &mut TestEnv) {
    var re = regex::compile(";")
    rx_split_size(env, &re, ";a;b", 3, "leading separator count")
    rx_split_part(env, &re, ";a;b", 0, "", "leading separator empty first")
    rx_split_part(env, &re, ";a;b", 1, "a", "leading separator a")
    rx_split_part(env, &re, ";a;b", 2, "b", "leading separator b")
}

@test
public func regex_split_empty_text(env : &mut TestEnv) {
    var re = regex::compile(",")
    rx_split_size(env, &re, "", 1, "empty text yields one empty part")
    rx_split_part(env, &re, "", 0, "", "empty text part is empty")
}

@test
public func regex_split_group_pattern(env : &mut TestEnv) {
    var re = regex::compile("(,|;)")
    rx_split_size(env, &re, "x,y;z", 3, "grouped separator split count")
    rx_split_part(env, &re, "x,y;z", 1, "y", "grouped separator middle part")
}
