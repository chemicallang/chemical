// Shared assertion helpers for the regex test suite.
//
// NOTE: always bind the result of `regex::compile(...)` to a local variable
// before calling methods on it (e.g. `var re = regex::compile("a")`).
// Using a `regex::Regex` temporary directly in an expression can corrupt the
// heap under the C backend, so the helpers below never take temporaries.

func rx_check(env : &mut TestEnv, cond : bool, name : *char) {
    if(cond) { test_env::ffi::log_cstr(env, LogType.Success, name, 0, 0) } else { test_env::ffi::log_cstr(env, LogType.Error, name, 0, 0) }
}

func rx_match(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected : bool, name : *char) {
    rx_check(env, re.is_match(text) == expected, name)
}

func rx_find(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, exp_start : i64, exp_end : i64, name : *char) {
    var found = re.find(text)
    if(found is std::Option.None) {
        rx_check(env, false, name)
        return
    }
    var Some(hit) = found else unsafe { unreachable }
    rx_check(env, (hit.start as i64) == exp_start && (hit.end as i64) == exp_end, name)
}

func rx_find_none(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, name : *char) {
    rx_check(env, re.find(text) is std::Option.None, name)
}

func rx_replace(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, replacement : std::string_view, expected : std::string_view, name : *char) {
    var got = re.replace(text, replacement)
    rx_check(env, got.equals_view(&expected), name)
}

func rx_captures_size(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected_size : int, name : *char) {
    var caps = re.captures(text)
    if(caps is std::Option.None) {
        rx_check(env, expected_size == 0, name)
        return
    }
    var Some(c) = caps else unsafe { unreachable }
    rx_check(env, c.size() as int == expected_size, name)
}

func rx_capture_pair(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, slot : int, exp_start : i64, exp_end : i64, name : *char) {
    var caps = re.captures(text)
    if(caps is std::Option.None) {
        rx_check(env, exp_start < 0, name)
        return
    }
    var Some(c) = caps else unsafe { unreachable }
    // `slot` is the historical flat position index (0,2,4,...);
    // Captures now takes a group index.
    var idx = (slot / 2) as size_t
    var start = c.capture_start(idx)
    var end = c.capture_end(idx)
    if(exp_start < 0) {
        rx_check(env, start is std::Option.None && end is std::Option.None, name)
        return
    }
    if(start is std::Option.None || end is std::Option.None) {
        rx_check(env, false, name)
        return
    }
    var Some(s) = start else unsafe { unreachable }
    var Some(e) = end else unsafe { unreachable }
    rx_check(env, (s as i64) == exp_start && (e as i64) == exp_end, name)
}

func rx_captures_matched(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected : bool, name : *char) {
    var matched = re.captures(text) is std::Option.Some
    rx_check(env, matched == expected, name)
}

func rx_split_size(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, expected_size : int, name : *char) {
    var parts = re.split(text)
    rx_check(env, parts.size() as int == expected_size, name)
}

func rx_split_part(env : &mut TestEnv, re : &regex::Regex, text : std::string_view, index : int, expected : std::string_view, name : *char) {
    var parts = re.split(text)
    var idx = index as size_t
    rx_check(env, idx < parts.size() && parts.get_ref(idx).to_view().equals(&expected), name)
}

func rx_compiles(env : &mut TestEnv, pattern : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, re.is_valid(), name)
}

func rx_error(env : &mut TestEnv, pattern : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, !re.is_valid(), name)
}

func rx_error_is(env : &mut TestEnv, pattern : std::string_view, expected : std::string_view, name : *char) {
    var re = regex::compile(pattern)
    rx_check(env, re.error().equals(&expected), name)
}
