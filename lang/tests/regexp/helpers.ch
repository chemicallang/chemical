// Shared assertion helpers for the regex test suite.
//
// NOTE: always bind the result of `regex::compile(...)` to a local variable
// before calling methods on it (e.g. `var re = regex::compile("a")`).
// Using a `regex::Regex` temporary directly in an expression can corrupt the
// heap under the C backend, so the helpers below never take temporaries.

func rx_check(env : &mut TestEnv, cond : bool, name : *char) {
    if(cond) { env.success(name) } else { env.error(name) }
}

func rx_match(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected : bool, name : *char) {
    rx_check(env, re.is_match(text) == expected, name)
}

func rx_find(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, exp_start : i64, exp_end : i64, name : *char) {
    var s : i64 = 0
    var e : i64 = 0
    var found = re.find(text, &raw mut s, &raw mut e)
    rx_check(env, found && s == exp_start && e == exp_end, name)
}

func rx_find_none(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, name : *char) {
    var s : i64 = 0
    var e : i64 = 0
    var found = re.find(text, &raw mut s, &raw mut e)
    rx_check(env, !found && s == -1 && e == -1, name)
}

func rx_replace(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, replacement : std::string_view, expected : std::string_view, name : *char) {
    var got = re.replace(text, replacement)
    rx_check(env, got.equals_view(&expected), name)
}

func rx_captures_size(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected_size : int, name : *char) {
    var caps = re.captures(text)
    rx_check(env, caps.size() as int == expected_size, name)
}

func rx_capture_pair(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, slot : int, exp_start : i64, exp_end : i64, name : *char) {
    var caps = re.captures(text)
    var idx = slot as size_t
    rx_check(env, idx + 1 < caps.size() && caps.positions.get(idx) == exp_start && caps.positions.get(idx + 1) == exp_end, name)
}

func rx_captures_matched(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected : bool, name : *char) {
    var caps = re.captures(text)
    rx_check(env, caps.matched == expected, name)
}

func rx_split_size(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected_size : int, name : *char) {
    var parts = std::vector<std::string_view>()
    re.split(text, &raw mut parts)
    rx_check(env, parts.size() as int == expected_size, name)
}

func rx_split_part(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, index : int, expected : std::string_view, name : *char) {
    var parts = std::vector<std::string_view>()
    re.split(text, &raw mut parts)
    var idx = index as size_t
    rx_check(env, idx < parts.size() && parts.get(idx).equals(&expected), name)
}

func rx_compiles(env : &mut TestEnv, pattern : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, re.compile_error.empty(), name)
}

func rx_error(env : &mut TestEnv, pattern : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, !re.compile_error.empty(), name)
}

func rx_error_is(env : &mut TestEnv, pattern : std::string_view, expected : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, re.compile_error.equals_view(&expected), name)
}
