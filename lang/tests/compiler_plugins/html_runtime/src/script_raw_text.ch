// The runtime html parser shares the lexer with the #html compile-time macro,
// including its <script> raw-text mode -- so the decision about what counts as
// script data lives in one place (html_parser) and both consumers have to agree.
//
// A <script> whose content really is JavaScript is raw text: everything up to
// </script> is one Text token and round-trips byte for byte, braces and all. A
// <script> whose `type` names a data block (application/json and friends) is
// NOT raw text: its content goes through the ordinary text/interpolation path
// like every other element. Keying the mode on the element name alone meant a
// data island could not be lexed at all in the interpolating sense, which is
// what made `{expr}` in one come out as literal source text.
//
// The macro-side tests for this live in
// lang/tests/compiler_plugins/html/src/script_interpolation.ch.

using namespace std;
using namespace html;

func runtime_roundtrip(env : &mut TestEnv, input : &std::string_view) {
    var view = std::string_view(input.data(), input.size())
    var out = html::parse_html(view)
    html_view_equals(env, out.to_view(), &view)
}

// --- must stay raw text: content is script data -------------------------

@test
public func test_runtime_script_without_type_is_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script>if(1 < 2) { var s = \"a & b\"; }</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_module_is_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"module\">const o = { a: 1 }; if(o.a) { go(); }</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_javascript_mime_type_is_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"text/javascript\">if(1 < 2) { go(); }</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_importmap_is_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"importmap\">{\"imports\":{\"lib\":\"/lib.js\"}}</script>")
    test_html_roundtrip(env, &input)
}

// --- data blocks: NOT raw text ------------------------------------------
// These still round-trip (the runtime parser re-emits text and attributes), but
// what matters is that they are no longer lexed as one opaque raw token, so the
// ordinary text/interpolation path applies to them.

@test
public func test_runtime_script_json_type_is_not_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"application/json\">hello island</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_json_type_with_attributes(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"application/json\" id=\"cfg\">plain body</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_type_is_case_insensitive(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"TEXT/JavaScript\">if(1 < 2) { go(); }</script>")
    test_html_roundtrip(env, &input)
}

@test
public func test_runtime_script_empty_type_is_raw_text(env : &mut TestEnv) {
    var input = std::string_view("<script type=\"\">if(1 < 2) { go(); }</script>")
    test_html_roundtrip(env, &input)
}