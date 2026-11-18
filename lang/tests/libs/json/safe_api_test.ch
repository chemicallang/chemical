// Safe JSON API: string-view/str parse entry points and the &str escaper.
//
// Covers empty input, unicode pass-through, control/quote/backslash escaping,
// and the malformed-input boundary of the safe parser alias.

using std::Result;
using std::string;
using std::string_view;

@test
func test_json_escape_into_empty(env : &mut TestEnv) {
    var out = string()
    json_escape_into(&mut out, "")
    if(!out.to_view().equals(string_view("\"\""))) {
        env.error("escaping an empty string should produce an empty JSON string")
    }
}

@test
func test_json_escape_into_unicode_passthrough(env : &mut TestEnv) {
    var out = string()
    json_escape_into(&mut out, "héllo 水 😀")
    if(!out.to_view().equals(string_view("\"héllo 水 😀\""))) {
        env.error("valid UTF-8 should pass through the escaper unchanged")
    }
}

@test
func test_json_escape_into_control_and_quotes(env : &mut TestEnv) {
    var out = string()
    json_escape_into(&mut out, "a\"b\\c\nd\te")
    if(!out.to_view().equals(string_view("\"a\\\"b\\\\c\\nd\\te\""))) {
        env.error("quotes, backslashes and control characters should be escaped")
    }
}

@test
func test_json_escape_into_accepts_string_views_and_owned_strings(env : &mut TestEnv) {
    var source = string("view me")
    var out = string()
    json_escape_into(&mut out, std::str.make_string(&source))
    if(!out.to_view().equals(string_view("\"view me\""))) {
        env.error("the safe escaper should accept an owned string")
    }

    var view = string_view("a/b")
    var out2 = string()
    json_escape_into(&mut out2, std::str.make_view(view))
    if(!out2.to_view().equals(string_view("\"a/b\""))) {
        env.error("the safe escaper should accept a string view")
    }
}

@test
func test_json_parser_parse_view_accepts_str(env : &mut TestEnv) {
    var handler = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var r = parser.parse_view("{\"a\": 1}", &mut handler)
    if(!r.ok) {
        env.error("parse_view should accept valid JSON text")
        return
    }
    var compact = json::stringify(&handler.root)
    if(!compact.to_view().equals(string_view("{\"a\":1}"))) {
        env.error("parse_view should produce the same tree as the raw parser")
    }
}

@test
func test_json_parser_parse_view_rejects_malformed(env : &mut TestEnv) {
    var handler = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var r = parser.parse_view("{\"a\":}", &mut handler)
    if(r.ok) {
        env.error("parse_view should reject a missing value")
    }
    if(r.pos == 0u) {
        env.error("parse_view should report the error position")
    }

    var handler2 = ASTJsonHandler()
    var parser2 = JsonParser(128, 4096)
    var trailing = parser2.parse_view("[1] trailing", &mut handler2)
    if(trailing.ok) {
        env.error("parse_view should reject trailing data")
    }
}

@test
func test_json_parser_parse_view_empty_input(env : &mut TestEnv) {
    var handler = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var r = parser.parse_view("", &mut handler)
    if(r.ok) {
        env.error("empty input is not valid JSON")
    }
}

// ---------------------------------------------------------------------------
// `parse` / `parse_with_limit`
//
// `parse` keeps a 4096-byte ceiling on any single string, which is right for a
// config document and wrong for every real upload: an 8 MiB profile picture is
// one base64 JSON string. `parse_with_limit` lets the caller state its own
// ceiling instead of the limit being baked into the parser.
// ---------------------------------------------------------------------------

/// A one-string document of `count` bytes: `{ "data" : "AAAA…" }`.
func string_document(count : size_t) : std::string {
    var doc = std::string()
    doc.append_view("{ \"data\" : \"")
    var i : size_t = 0
    while(i < count) {
        doc.append('A')
        i += 1
    }
    doc.append_view("\" }")
    return doc
}

@test
func test_parse_keeps_its_default_string_ceiling(env : &mut TestEnv) {
    // The default is a deliberate guard, not an oversight: a string past it is
    // refused rather than silently truncated.
    var doc = string_document(5000u)
    var parsed = json::parse(doc.to_view())
    if(parsed is std::Result.Ok) {
        env.error("json::parse accepted a 5000-byte string; the 4096 ceiling is not enforced")
    }
}

@test
func test_parse_with_limit_accepts_a_string_under_its_ceiling(env : &mut TestEnv) {
    // The upload case: a string far past 4096 bytes is accepted once the caller
    // raises the ceiling.
    var doc = string_document(5000u)
    var parsed = json::parse_with_limit(doc.to_view(), 8192u)
    if(parsed is std::Result.Err) {
        env.error("parse_with_limit refused a 5000-byte string with an 8192-byte ceiling")
        return
    }
    var Ok(value) = parsed else unsafe { unreachable }
    if(!(value is JsonValue.Object)) {
        env.error("parse_with_limit did not return the parsed object")
        return
    }
    var Object(members) = value else unsafe { unreachable }
    var field = members.get(&std::string("data"))
    if(field is std::Option.None) {
        env.error("parse_with_limit dropped the string field")
        return
    }
    var Some(entry) = field else unsafe { unreachable }
    if(!(entry is JsonValue.String)) {
        env.error("parse_with_limit did not return the string field as a string")
        return
    }
    var String(text) = entry else unsafe { unreachable }
    if(text.size() != 5000u) {
        env.error("parse_with_limit truncated the string field")
    }
}

@test
func test_parse_with_limit_still_refuses_a_string_over_its_ceiling(env : &mut TestEnv) {
    // A stated ceiling is a ceiling, not a hint.
    var doc = string_document(9000u)
    var parsed = json::parse_with_limit(doc.to_view(), 8192u)
    if(parsed is std::Result.Ok) {
        env.error("parse_with_limit accepted a 9000-byte string with an 8192-byte ceiling")
    }
}

@test
func test_parse_with_limit_does_not_change_small_documents(env : &mut TestEnv) {
    // Raising the ceiling must not alter parsing itself.
    var parsed = json::parse_with_limit("{\"a\": 1, \"b\": [true, null]}", 8192u)
    if(parsed is std::Result.Err) {
        env.error("parse_with_limit rejected a valid small document")
        return
    }
    var Ok(value) = parsed else unsafe { unreachable }
    if(!(value is JsonValue.Object)) {
        env.error("parse_with_limit did not return an object")
        return
    }
    var Object(members) = value else unsafe { unreachable }
    if(members.size() != 2u) {
        env.error("parse_with_limit changed the member count")
    }
}
