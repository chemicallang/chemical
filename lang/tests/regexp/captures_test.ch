// Capture groups: slot layout and positions.
//
// `captures(text)` returns a Captures value whose `positions` vector holds
// pairs for the full match and then each group:
//     [full_start, full_end, g1_start, g1_end, g2_start, g2_end, ...]
// An unmatched group is recorded as (-1, -1).

@test
public func regex_captures_group_positions(env : &mut TestEnv) {
    var re = regex::compile("(ab)(cd)")
    rx_captures_matched(env, &re, "xxabcdyy", true, "captures matched")
    rx_captures_size(env, &re, "xxabcdyy", 6, "capture slot count is 6")
    rx_capture_pair(env, &re, "xxabcdyy", 0, 2, 6, "full match positions")
    rx_capture_pair(env, &re, "xxabcdyy", 2, 2, 4, "first group positions")
    rx_capture_pair(env, &re, "xxabcdyy", 4, 4, 6, "second group positions")
}

@test
public func regex_captures_nested_positions(env : &mut TestEnv) {
    var re = regex::compile("((ab)c)")
    rx_captures_matched(env, &re, "xxabcyy", true, "nested captures matched")
    rx_captures_size(env, &re, "xxabcyy", 6, "nested capture slot count")
    rx_capture_pair(env, &re, "xxabcyy", 0, 2, 5, "nested full match")
    rx_capture_pair(env, &re, "xxabcyy", 2, 2, 5, "nested outer group")
    rx_capture_pair(env, &re, "xxabcyy", 4, 2, 4, "nested inner group")
}

@test
public func regex_captures_unmatched_group(env : &mut TestEnv) {
    var re = regex::compile("(a)|(b)")
    rx_capture_pair(env, &re, "b", 2, -1, -1, "unmatched group is -1,-1")
    rx_capture_pair(env, &re, "b", 4, 0, 1, "matched alternative records positions")
    rx_capture_pair(env, &re, "a", 2, 0, 1, "first alternative records positions")
    rx_capture_pair(env, &re, "a", 4, -1, -1, "other alternative group is -1,-1")
}

@test
public func regex_captures_optional_group(env : &mut TestEnv) {
    var re = regex::compile("(a)?b")
    rx_capture_pair(env, &re, "b", 2, -1, -1, "optional group absent is -1,-1")
    rx_capture_pair(env, &re, "ab", 2, 0, 1, "optional group present positions")
}

@test
public func regex_captures_optional_group_empty(env : &mut TestEnv) {
    var re = regex::compile("(a)?")
    rx_capture_pair(env, &re, "", 2, -1, -1, "optional group on empty text is absent")
}

@test
public func regex_captures_no_match(env : &mut TestEnv) {
    var re = regex::compile("z")
    rx_captures_matched(env, &re, "abc", false, "no match reported")
    rx_captures_size(env, &re, "abc", 0, "no match has zero slots")
}

@test
public func regex_captures_three_groups(env : &mut TestEnv) {
    var re = regex::compile("(a)(b)(c)")
    rx_captures_size(env, &re, "abc", 8, "three groups produce eight slots")
    rx_capture_pair(env, &re, "abc", 0, 0, 3, "three groups full match")
    rx_capture_pair(env, &re, "abc", 2, 0, 1, "three groups first")
    rx_capture_pair(env, &re, "abc", 4, 1, 2, "three groups second")
    rx_capture_pair(env, &re, "abc", 6, 2, 3, "three groups third")
}

@test
public func regex_captures_alternation_subgroup(env : &mut TestEnv) {
    var re = regex::compile("(cat|dog)s?")
    rx_capture_pair(env, &re, "dogs", 2, 0, 3, "alternation subgroup excludes s")
    rx_capture_pair(env, &re, "dog", 2, 0, 3, "alternation subgroup without s")
}

@test
public func regex_captures_in_middle_of_text(env : &mut TestEnv) {
    var re = regex::compile("(\\d+)-(\\d+)")
    rx_capture_pair(env, &re, "a12-34b", 0, 1, 6, "full match in the middle")
    rx_capture_pair(env, &re, "a12-34b", 2, 1, 3, "first group in the middle")
    rx_capture_pair(env, &re, "a12-34b", 4, 4, 6, "second group in the middle")
}

@test
public func regex_captures_email_components(env : &mut TestEnv) {
    var re = regex::compile("(\\w+)@(\\w+)\\.(\\w+)")
    rx_captures_size(env, &re, "mail to a@b.com!", 8, "email capture slot count")
    rx_capture_pair(env, &re, "mail to a@b.com!", 0, 8, 15, "email full match")
    rx_capture_pair(env, &re, "mail to a@b.com!", 2, 8, 9, "email local part")
    rx_capture_pair(env, &re, "mail to a@b.com!", 4, 10, 11, "email domain")
    rx_capture_pair(env, &re, "mail to a@b.com!", 6, 12, 15, "email tld")
}

@test
public func regex_captures_no_groups(env : &mut TestEnv) {
    var re = regex::compile("abc")
    rx_captures_size(env, &re, "xxabcxx", 2, "no-group pattern has full-match slots only")
    rx_capture_pair(env, &re, "xxabcxx", 0, 2, 5, "no-group full match positions")
}

@test
public func regex_captures_digit_group(env : &mut TestEnv) {
    var re = regex::compile("(\\d+)")
    rx_captures_size(env, &re, "12abc34", 4, "single digit group slot count")
    rx_capture_pair(env, &re, "12abc34", 0, 0, 2, "single digit group full match")
    rx_capture_pair(env, &re, "12abc34", 2, 0, 2, "single digit group positions")
}

@test
public func regex_captures_repeated_group_last_iteration(env : &mut TestEnv) {
    // For a repeated group the last successful iteration is recorded.
    var re = regex::compile("(ab)+")
    rx_captures_size(env, &re, "zzabab", 4, "repeated group slot count")
    rx_capture_pair(env, &re, "zzabab", 0, 2, 6, "repeated group full match")
    rx_capture_pair(env, &re, "zzabab", 2, 4, 6, "repeated group records last iteration")

    var alt = regex::compile("(a|b)+")
    rx_capture_pair(env, &alt, "zzabab", 2, 5, 6, "repeated alternation records last iteration")
}

@test
public func regex_captures_adjacent_empty_quantifiers(env : &mut TestEnv) {
    var re = regex::compile("(a*)(b*)")
    rx_captures_size(env, &re, "aabb", 6, "adjacent star groups slot count")
    rx_capture_pair(env, &re, "aabb", 0, 0, 4, "adjacent star groups full match")
    rx_capture_pair(env, &re, "aabb", 2, 0, 2, "first star group positions")
    rx_capture_pair(env, &re, "aabb", 4, 2, 4, "second star group positions")
}

@test
public func regex_captures_anchored_pair(env : &mut TestEnv) {
    var re = regex::compile("^(a+)(b+)$")
    rx_captures_size(env, &re, "aabb", 6, "anchored pair slot count")
    rx_capture_pair(env, &re, "aabb", 0, 0, 4, "anchored pair full match")
    rx_capture_pair(env, &re, "aabb", 2, 0, 2, "anchored pair first group")
    rx_capture_pair(env, &re, "aabb", 4, 2, 4, "anchored pair second group")
}
