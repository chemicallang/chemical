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
