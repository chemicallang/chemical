// Safe regex API: find/captures return Option, split returns owned strings,
// replace takes &str, and matches/captures expose borrowed views.
//
// These tests cover empty input, no-match, malformed patterns, unicode, and
// the fact that a returned Match view points into the searched text.

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

@test
public func regex_safe_find_returns_borrowed_match(env : &mut TestEnv) {
    var re = regex::compile("world")
    var found = re.find("hello world")
    if(found is Option.None) {
        env.error("safe find should find world")
        return
    }
    var Some(hit) = found else unsafe { unreachable }
    if(hit.start != 6u) { env.error("safe find start should be 6") }
    if(hit.end != 11u) { env.error("safe find end should be 11") }
    if(hit.size() != 5u) { env.error("safe find size should be 5") }
    if(!hit.value().equals(string_view("world"))) {
        env.error("safe find value should be a view of the match")
    }
}

@test
public func regex_safe_find_none(env : &mut TestEnv) {
    var re = regex::compile("xyz")
    var found = re.find("abc")
    if(found is Option.Some) { env.error("safe find should return None when absent") }
}

@test
public func regex_safe_find_empty_text_and_pattern(env : &mut TestEnv) {
    var empty_pattern = regex::compile("")
    var found = empty_pattern.find("")
    if(found is Option.None) {
        env.error("empty pattern should match empty text")
        return
    }
    var Some(hit) = found else unsafe { unreachable }
    if(hit.start != 0u || hit.end != 0u) { env.error("empty match should be 0..0") }
    if(hit.value().size() != 0u) { env.error("empty match view should be empty") }

    var literal = regex::compile("a")
    if(literal.find("") is Option.Some) {
        env.error("a non-empty pattern must not match empty text")
    }
}

@test
public func regex_safe_find_unicode_text(env : &mut TestEnv) {
    // The matcher is byte-oriented. Search ASCII inside UTF-8 text and check
    // that offsets are byte offsets.
    var re = regex::compile("f")
    var found = re.find("café")
    if(found is Option.None) {
        env.error("safe find should match an ASCII byte in UTF-8 text")
        return
    }
    var Some(hit) = found else unsafe { unreachable }
    if(hit.start != 2u) { env.error("unicode-text match should start at byte 2") }
    if(hit.end != 3u) { env.error("unicode-text match should end at byte 3") }
    if(!hit.value().equals(string_view("f"))) {
        env.error("unicode-text match view should be the exact slice")
    }

    // A capturing view can span the whole UTF-8 value unchanged.
    var whole = regex::compile("(.+)")
    var caps = whole.captures("café")
    if(caps is Option.None) {
        env.error("captures should match the whole unicode text")
        return
    }
    var Some(c) = caps else unsafe { unreachable }
    var group = c.group(0u)
    if(group is Option.None) {
        env.error("group 0 should span the unicode text")
    } else {
        var Some(view) = group else unsafe { unreachable }
        if(!view.equals(string_view("café"))) {
            env.error("capture view should preserve UTF-8 bytes")
        }
    }
}

@test
public func regex_safe_captures_option_and_group_views(env : &mut TestEnv) {
    var re = regex::compile("(\\d+)-(\\w+)")
    var found = re.captures("id 42-abc!")
    if(found is Option.None) {
        env.error("safe captures should match")
        return
    }
    var Some(caps) = found else unsafe { unreachable }
    if(!caps.matched()) { env.error("safe captures should be marked matched") }
    if(caps.size() != 6u) { env.error("safe captures should have six slots") }

    var whole = caps.group(0u)
    if(whole is Option.None) {
        env.error("group 0 should be present")
    } else {
        var Some(view) = whole else unsafe { unreachable }
        if(!view.equals(string_view("42-abc"))) { env.error("group 0 should be the full match") }
    }

    var second = caps.group(2u)
    if(second is Option.None) {
        env.error("group 2 should be present")
    } else {
        var Some(view) = second else unsafe { unreachable }
        if(!view.equals(string_view("abc"))) { env.error("group 2 should be abc") }
    }

    if(caps.group(9u) is Option.Some) {
        env.error("out-of-range group should be None")
    }
}

@test
public func regex_safe_captures_none_on_no_match(env : &mut TestEnv) {
    var re = regex::compile("z")
    if(re.captures("abc") is Option.Some) {
        env.error("safe captures should return None when there is no match")
    }
}

@test
public func regex_safe_replace_accepts_str(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    var got = re.replace("a1b22c", "#")
    if(!got.to_view().equals(string_view("a#b#c"))) {
        env.error("safe replace should replace every digit run")
    }
    var unchanged = re.replace("abc", "#")
    if(!unchanged.to_view().equals(string_view("abc"))) {
        env.error("safe replace without a match should return the original text")
    }
    var backref = regex::compile("(\\w+)-(\\w+)")
    var swapped = backref.replace("left-right", "$2:$1")
    if(!swapped.to_view().equals(string_view("right:left"))) {
        env.error("safe replace should support backreferences")
    }
}

@test
public func regex_safe_split_returns_owned_strings(env : &mut TestEnv) {
    var re = regex::compile(",")
    var parts = re.split("a,b,c")
    if(parts.size() != 3u) {
        env.error("safe split should return three parts")
        return
    }
    if(!parts.at_ref(0u).to_view().equals(string_view("a"))) { env.error("split part 0 should be a") }
    if(!parts.at_ref(1u).to_view().equals(string_view("b"))) { env.error("split part 1 should be b") }
    if(!parts.at_ref(2u).to_view().equals(string_view("c"))) { env.error("split part 2 should be c") }

    // Owned, not borrowed: mutating the source after the split must not change
    // the returned strings.
    var source = string("x,y")
    var view = std::str.make_string(&source)
    var owned = re.split(view)
    source.clear()
    source.append_view("changed")
    if(owned.size() != 2u) { env.error("safe split should return two parts") }
    if(!owned.at_ref(0u).to_view().equals(string_view("x"))) {
        env.error("safe split must copy segments, not borrow the source")
    }
}

@test
public func regex_safe_split_edge_cases(env : &mut TestEnv) {
    var comma = regex::compile(",")
    var empty = comma.split("")
    if(empty.size() != 1u) { env.error("empty text should split into one empty part") }
    if(empty.size() > 0u && empty.at_ref(0u).size() != 0u) {
        env.error("the single empty-text part should be empty")
    }

    var edge = comma.split(",a,")
    if(edge.size() != 3u) { env.error("edge split should yield three parts") }
    if(edge.size() == 3u && edge.at_ref(0u).size() != 0u) { env.error("leading part should be empty") }
    if(edge.size() == 3u && edge.at_ref(2u).size() != 0u) { env.error("trailing part should be empty") }

    var unicode = regex::compile(",")
    var parts = unicode.split("α,β")
    if(parts.size() != 2u) {
        env.error("unicode split should yield two parts")
    } else if(!parts.at_ref(1u).to_view().equals(string_view("β"))) {
        env.error("unicode split should preserve UTF-8 bytes")
    }
}

@test
public func regex_safe_apis_on_malformed_pattern(env : &mut TestEnv) {
    var bad = regex::compile("[abc")
    if(bad.is_valid()) {
        env.error("test expects a malformed pattern")
        return
    }
    if(bad.find("abc") is Option.Some) { env.error("malformed regex find should be None") }
    if(bad.captures("abc") is Option.Some) { env.error("malformed regex captures should be None") }
    var replaced = bad.replace("abc", "x")
    if(!replaced.to_view().equals(string_view("abc"))) {
        env.error("malformed regex replace should leave the text unchanged")
    }
    var parts = bad.split("abc")
    if(parts.size() != 1u) { env.error("malformed regex split should yield one part") }
}

@test
public func regex_safe_try_compile_typed_error(env : &mut TestEnv) {
    var good = regex::try_compile("a+b")
    if(good is Result.Err) {
        env.error("try_compile should accept a valid pattern")
        return
    }
    var Ok(re) = good else unsafe { unreachable }
    if(!re.is_valid()) { env.error("a valid Regex should report is_valid") }
    if(!re.error().empty()) { env.error("a valid Regex should have an empty error") }

    var bad = regex::try_compile("[abc")
    if(bad is Result.Ok) {
        env.error("try_compile should reject a malformed pattern")
        return
    }
    var Err(e) = bad else unsafe { unreachable }
    if(e.message().empty()) { env.error("RegexError should carry a message") }
    if(e.message().equals(string_view("unclosed character class")) == false) {
        env.error("RegexError message should be the parser message")
    }
}
