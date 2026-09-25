// General properties and realistic patterns.

@test
public func regex_pattern_is_recorded(env : &mut TestEnv) {
    var re = regex::compile("a+b*c?")
    var expected = std::string_view("a+b*c?")
    rx_check(env, re.pattern.equals_view(&expected), "pattern text is recorded")
    rx_check(env, re.compile_error.empty(), "valid pattern has no error")
}

@test
public func regex_reusable_across_inputs(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    rx_match(env, &re, "abc123", true, "reuse 1 matches")
    rx_match(env, &re, "000", true, "reuse 2 matches")
    rx_match(env, &re, "abc", false, "reuse 3 rejects")
    rx_match(env, &re, "x9y", true, "reuse 4 matches a single digit")
}

@test
public func regex_reuse_find_and_captures(env : &mut TestEnv) {
    var re = regex::compile("(\\d+)-(\\d+)")
    rx_find(env, &re, "10-20", 0, 5, "reuse find")
    rx_capture_pair(env, &re, "10-20", 0, 0, 5, "reuse captures full match")
    rx_capture_pair(env, &re, "10-20", 2, 0, 2, "reuse captures group 1")
    rx_capture_pair(env, &re, "10-20", 4, 3, 5, "reuse captures group 2")
    var replaced = re.replace("7-8 and 9-10", "$2/$1")
    var expected = std::string_view("8/7 and 10/9")
    rx_check(env, replaced.equals_view(&expected), "reuse replace")
}

@test
public func regex_is_match_finds_substring_anywhere(env : &mut TestEnv) {
    var re = regex::compile("b+")
    rx_match(env, &re, "bbba", true, "is_match finds a match at the start")
    rx_match(env, &re, "abbb", true, "is_match finds a match at the end")
    rx_match(env, &re, "a:b", true, "is_match finds a single char in the middle")
    rx_match(env, &re, "aaa", false, "is_match rejects absent")
}

@test
public func regex_case_sensitive(env : &mut TestEnv) {
    var lower = regex::compile("abc")
    rx_match(env, &lower, "abc", true, "lowercase matches")
    rx_match(env, &lower, "ABC", false, "case sensitive by default")
    rx_match(env, &lower, "Abc", false, "mixed case rejected")
}

@test
public func regex_float_pattern(env : &mut TestEnv) {
    var re = regex::compile("[0-9]+\\.[0-9]+")
    rx_match(env, &re, "3.14159", true, "float pattern matches")
    rx_match(env, &re, "42", false, "float pattern needs a decimal point")
}

@test
public func regex_ipv4_like_pattern(env : &mut TestEnv) {
    var re = regex::compile("[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
    rx_match(env, &re, "192.168.0.1", true, "ipv4-like pattern matches")
    rx_match(env, &re, "192.168.0", false, "ipv4-like pattern needs four octets")
}

@test
public func regex_log_line_pattern(env : &mut TestEnv) {
    var re = regex::compile("^(\\w+) (\\d+)$")
    rx_captures_matched(env, &re, "ERROR 404", true, "log line matches")
    rx_capture_pair(env, &re, "ERROR 404", 2, 0, 5, "log level group")
    rx_capture_pair(env, &re, "ERROR 404", 4, 6, 9, "log code group")
    rx_find_none(env, &re, " ERROR 404", "log line with a leading space fails the anchor")
}

@test
public func regex_snake_case_pattern(env : &mut TestEnv) {
    var re = regex::compile("^[a-z][a-z0-9_]*$")
    rx_match(env, &re, "my_variable_1", true, "snake_case identifier matches")
    rx_match(env, &re, "MyVariable", false, "snake_case rejects uppercase")
    rx_match(env, &re, "1var", false, "snake_case rejects a leading digit")
}

@test
public func regex_hex_color_pattern(env : &mut TestEnv) {
    var re = regex::compile("#[0-9a-f]+")
    rx_match(env, &re, "#ff00aa", true, "hex color matches")
    rx_match(env, &re, "color: #abc;", true, "hex color inside CSS")
    rx_match(env, &re, "not a color", false, "hex color rejects plain text")
}

@test
public func regex_repeated_large_input(env : &mut TestEnv) {
    var re = regex::compile("(ab)+c")
    rx_match(env, &re, "ababababababababc", true, "long repeated run with terminator")
    rx_captures_matched(env, &re, "ababababababababc", true, "long repeated run captures")
    rx_find_none(env, &re, "abababababababab", "run without the terminator fails")
}
