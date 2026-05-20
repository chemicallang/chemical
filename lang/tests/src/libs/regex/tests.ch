@test
public func regex_literal_match(env : &mut TestEnv) {
    var re = regex::compile("hello")
    if(re.is_match("hello")) { env.success("literal hello") } else { env.error("should match hello") }
    if(re.is_match("hello world")) { env.success("hello in string") } else { env.error("should match hello in string") }
    if(!re.is_match("world")) { env.success("not match world") } else { env.error("should not match world") }
}

@test
public func regex_literal_empty(env : &mut TestEnv) {
    var re = regex::compile("")
    if(re.is_match("")) { env.success("empty matches empty") } else { env.error("empty should match empty") }
    if(re.is_match("a")) { env.success("empty matches anything") } else { env.error("empty should match anything") }
}

@test
public func regex_anchor_start(env : &mut TestEnv) {
    var re = regex::compile("^hello")
    if(re.is_match("hello")) { env.success("^hello matches hello") } else { env.error("^hello should match hello") }
    if(re.is_match("hello world")) { env.success("^hello at start") } else { env.error("^hello should match at start") }
    if(!re.is_match("say hello")) { env.success("not mid string") } else { env.error("^hello should not match mid string") }
}

@test
public func regex_anchor_end(env : &mut TestEnv) {
    var re = regex::compile("world$")
    if(re.is_match("world")) { env.success("world$ matches world") } else { env.error("world$ should match world") }
    if(re.is_match("hello world")) { env.success("world$ at end") } else { env.error("world$ should match at end") }
    if(!re.is_match("world!")) { env.success("not with extra") } else { env.error("world$ should not match world!") }
}

@test
public func regex_dot(env : &mut TestEnv) {
    var re = regex::compile("h.llo")
    if(re.is_match("hello")) { env.success("h.llo matches hello") } else { env.error("h.llo should match hello") }
    if(re.is_match("hxllo")) { env.success("h.llo matches hxllo") } else { env.error("h.llo should match hxllo") }
    if(!re.is_match("hllo")) { env.success("not too short") } else { env.error("h.llo should not match hllo") }
}

@test
public func regex_alternation(env : &mut TestEnv) {
    var re = regex::compile("cat|dog")
    if(re.is_match("cat")) { env.success("alt cat") } else { env.error("alt should match cat") }
    if(re.is_match("dog")) { env.success("alt dog") } else { env.error("alt should match dog") }
    if(!re.is_match("bird")) { env.success("not bird") } else { env.error("alt should not match bird") }
}

@test
public func regex_star(env : &mut TestEnv) {
    var re = regex::compile("a*")
    if(re.is_match("")) { env.success("a* matches empty") } else { env.error("a* should match empty") }
    if(re.is_match("a")) { env.success("a* matches a") } else { env.error("a* should match a") }
    if(re.is_match("aaa")) { env.success("a* matches aaa") } else { env.error("a* should match aaa") }
}

@test
public func regex_plus(env : &mut TestEnv) {
    var re = regex::compile("a+")
    if(!re.is_match("")) { env.success("a+ not empty") } else { env.error("a+ should not match empty") }
    if(re.is_match("a")) { env.success("a+ matches a") } else { env.error("a+ should match a") }
    if(re.is_match("aaa")) { env.success("a+ matches aaa") } else { env.error("a+ should match aaa") }
}

@test
public func regex_optional(env : &mut TestEnv) {
    var re = regex::compile("ab?c")
    if(re.is_match("ac")) { env.success("ab?c matches ac") } else { env.error("ab?c should match ac") }
    if(re.is_match("abc")) { env.success("ab?c matches abc") } else { env.error("ab?c should match abc") }
    if(!re.is_match("abbc")) { env.success("not abbc") } else { env.error("ab?c should not match abbc") }
}

@test
public func regex_group(env : &mut TestEnv) {
    var re = regex::compile("(ab)+")
    if(re.is_match("ab")) { env.success("(ab)+ matches ab") } else { env.error("(ab)+ should match ab") }
    if(re.is_match("abab")) { env.success("(ab)+ matches abab") } else { env.error("(ab)+ should match abab") }
    if(!re.is_match("a")) { env.success("(ab)+ not a") } else { env.error("(ab)+ should not match a") }
}

@test
public func regex_class_simple(env : &mut TestEnv) {
    var re = regex::compile("[abc]")
    if(re.is_match("a")) { env.success("[abc] matches a") } else { env.error("[abc] should match a") }
    if(re.is_match("b")) { env.success("[abc] matches b") } else { env.error("[abc] should match b") }
    if(!re.is_match("d")) { env.success("[abc] not d") } else { env.error("[abc] should not match d") }
}

@test
public func regex_class_range(env : &mut TestEnv) {
    var re = regex::compile("[a-z]")
    if(re.is_match("m")) { env.success("[a-z] matches m") } else { env.error("[a-z] should match m") }
    if(!re.is_match("3")) { env.success("[a-z] not 3") } else { env.error("[a-z] should not match 3") }
}

@test
public func regex_class_negated(env : &mut TestEnv) {
    var re = regex::compile("[^abc]")
    if(re.is_match("d")) { env.success("[^abc] matches d") } else { env.error("[^abc] should match d") }
    if(!re.is_match("a")) { env.success("[^abc] not a") } else { env.error("[^abc] should not match a") }
}

@test
public func regex_digit_class(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    if(re.is_match("123")) { env.success("\\d+ matches 123") } else { env.error("\\d+ should match 123") }
    if(!re.is_match("abc")) { env.success("\\d+ not abc") } else { env.error("\\d+ should not match abc") }
}

@test
public func regex_word_class(env : &mut TestEnv) {
    var re = regex::compile("\\w+")
    if(re.is_match("hello")) { env.success("\\w+ matches hello") } else { env.error("\\w+ should match hello") }
    if(re.is_match("abc123")) { env.success("\\w+ matches abc123") } else { env.error("\\w+ should match abc123") }
    if(!re.is_match("")) { env.success("\\w+ not empty") } else { env.error("\\w+ should not match empty") }
}

@test
public func regex_find(env : &mut TestEnv) {
    var re = regex::compile("world")
    var start : i64 = 0
    var end : i64 = 0
    var found = re.find("hello world", &mut start, &mut end)
    if(found) { env.success("find world") } else { env.error("should find world") }
    if(start == 6) { env.success("start at 6") } else { env.error("start should be 6") }
}

@test
public func regex_find_no_match(env : &mut TestEnv) {
    var re = regex::compile("xyz")
    var start : i64 = 0
    var end : i64 = 0
    var found = re.find("hello world", &mut start, &mut end)
    if(!found) { env.success("find no match") } else { env.error("should not find xyz") }
}

@test
public func regex_complex_pattern(env : &mut TestEnv) {
    var re = regex::compile("^[a-zA-Z][a-zA-Z0-9]*$")
    if(re.is_match("Hello")) { env.success("id Hello") } else { env.error("should match Hello") }
    if(re.is_match("var123")) { env.success("id var123") } else { env.error("should match var123") }
    if(!re.is_match("123abc")) { env.success("not 123abc") } else { env.error("should not match 123abc") }
    if(!re.is_match("")) { env.success("not empty") } else { env.error("should not match empty") }
}

@test
public func regex_alternation_group(env : &mut TestEnv) {
    var re = regex::compile("(ab|cd)+")
    if(re.is_match("ab")) { env.success("(ab|cd)+ ab") } else { env.error("should match ab") }
    if(re.is_match("cd")) { env.success("(ab|cd)+ cd") } else { env.error("should match cd") }
    if(re.is_match("abcd")) { env.success("(ab|cd)+ abcd") } else { env.error("should match abcd") }
}

@test
public func regex_escape_dot(env : &mut TestEnv) {
    var re = regex::compile("a\\.b")
    if(re.is_match("a.b")) { env.success("a\\.b matches a.b") } else { env.error("should match a.b") }
    if(!re.is_match("axb")) { env.success("a\\.b not axb") } else { env.error("should not match axb") }
}

@test
public func regex_whitespace_class(env : &mut TestEnv) {
    var re = regex::compile("\\s+")
    if(re.is_match("   ")) { env.success("\\s+ spaces") } else { env.error("should match spaces") }
    if(re.is_match("\t")) { env.success("\\s+ tab") } else { env.error("should match tab") }
}

@test
public func regex_non_digit_class(env : &mut TestEnv) {
    var re = regex::compile("\\D+")
    if(re.is_match("abc")) { env.success("\\D+ abc") } else { env.error("should match abc") }
    if(!re.is_match("123")) { env.success("\\D+ not 123") } else { env.error("should not match 123") }
}

@test
public func regex_compile_error(env : &mut TestEnv) {
    var re = regex::compile("[abc")
    if(!re.compile_error.empty()) { env.success("unclosed class error") } else { env.error("should report error for unclosed class") }
}

@test
public func regex_dot_not_newline(env : &mut TestEnv) {
    var re = regex::compile("a.b")
    if(!re.is_match("a\nb")) { env.success("dot not newline") } else { env.error("dot should not match newline") }
}

@test
public func regex_star_plus_combined(env : &mut TestEnv) {
    var re = regex::compile("a*b+")
    if(re.is_match("b")) { env.success("a*b+ matches b") } else { env.error("should match b") }
    if(re.is_match("aaab")) { env.success("a*b+ matches aaab") } else { env.error("should match aaab") }
    if(!re.is_match("")) { env.success("a*b+ not empty") } else { env.error("should not match empty") }
}

@test
public func regex_captures_group_positions(env : &mut TestEnv) {
    var re = regex::compile("(ab)(cd)")
    var caps = re.captures("xxabcdyy")
    if(caps.matched) { env.success("captures matched") } else { env.error("captures should match") }
    if(caps.size() == 6) { env.success("capture slot count") } else { env.error("captures should include full match plus groups") }
    if(caps.positions.get(0) == 2 && caps.positions.get(1) == 6) { env.success("full match positions") } else { env.error("full match positions should be 2..6") }
    if(caps.positions.get(2) == 2 && caps.positions.get(3) == 4) { env.success("first group positions") } else { env.error("first group positions should be 2..4") }
    if(caps.positions.get(4) == 4 && caps.positions.get(5) == 6) { env.success("second group positions") } else { env.error("second group positions should be 4..6") }
}

@test
public func regex_replace_replaces_all_matches(env : &mut TestEnv) {
    var re = regex::compile("\\d+")
    var replaced = re.replace("abc123def45", "#")
    if(replaced.equals(std::string("abc#def#"))) { env.success("replace all matches") } else { env.error("replace should replace all matches") }
}

@test
public func regex_split_splits_on_matches(env : &mut TestEnv) {
    var re = regex::compile(",+")
    var parts = std::vector<std::string_view>()
    re.split("a,,b,c", &mut parts)
    if(parts.size() == 3) { env.success("split part count") } else { env.error("split should produce three parts") }
    if(parts.get(0).equals("a")) { env.success("split first part") } else { env.error("split first part should be a") }
    if(parts.get(1).equals("b")) { env.success("split second part") } else { env.error("split second part should be b") }
    if(parts.get(2).equals("c")) { env.success("split third part") } else { env.error("split third part should be c") }
}

@test
public func regex_find_empty_pattern_on_empty_text(env : &mut TestEnv) {
    var re = regex::compile("")
    var start : i64 = -1
    var end : i64 = -1
    var found = re.find("", &mut start, &mut end)
    if(found) { env.success("empty find matched") } else { env.error("empty pattern should find in empty text") }
    if(start == 0 && end == 0) { env.success("empty find positions") } else { env.error("empty find positions should be 0..0") }
}

@test
public func regex_unmatched_close_paren_is_error(env : &mut TestEnv) {
    var re = regex::compile("abc)")
    if(!re.compile_error.empty()) { env.success("unmatched close paren error") } else { env.error("should report error for unmatched close paren") }
}
