using namespace std;
using namespace html;

func html_view_equals(env : &mut TestEnv, got : &std::string_view, expected : &std::string_view) {
    if(got.equals(expected)) {
        return;
    }
    env.error("html parse mismatch");
    var expected_s = std::string("expected:\"");
    expected_s.append_view(expected)
    expected_s.append('"');
    env.info(expected_s.data())

    var got_s = std::string("got     :\"");
    got_s.append_view(got)
    got_s.append('"');
    env.info(got_s.data())
}

func test_html_roundtrip(env : &mut TestEnv, input : &std::string_view) {
    var view = std::string_view(input.data(), input.size())
    var out = html::parse_html(view)
    html_view_equals(env, out.to_view(), &view)
}

@test
func test_html_parse_simple_div(env : &mut TestEnv) {
    var input = std::string_view("<div class=\"test\">Hello <b>world</b></div>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_parse_paragraph(env : &mut TestEnv) {
    var input = std::string_view("<p>Simple text</p>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_parse_self_closing(env : &mut TestEnv) {
    var input = std::string_view("<img src=\"a.png\" alt=\"pic\" />")
    var out = html::parse_html(input)
    var expected = std::string_view("<img src=\"a.png\" alt=\"pic\"/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_parse_nested_list(env : &mut TestEnv) {
    var input = std::string_view("<ul><li>one</li><li>two</li></ul>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_parse_void_br(env : &mut TestEnv) {
    var input = std::string_view("<br>")
    var out = html::parse_html(input)
    var expected = std::string_view("<br/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_parse_comment(env : &mut TestEnv) {
    var input = std::string_view("<!-- a comment --><span>after</span>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_parse_single_quoted_attr(env : &mut TestEnv) {
    var input = std::string_view("<a href='https://x.com'>link</a>")
    var out = html::parse_html(input)
    var expected = std::string_view("<a href=\"https://x.com\">link</a>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_tokenize(env : &mut TestEnv) {
    var input = std::string_view("<div>hi</div>")
    var tokens = html::tokenize_html(input)
    if(tokens.size() == 0) {
        env.error("tokenize_html returned no tokens")
        return
    }
}

@test
func test_html_parse_root_api(env : &mut TestEnv) {
    var input = std::string_view("<span>x</span>")
    var allocator = ASTAllocator.make()
    var root = html::parse_html_root(input, &raw mut allocator)
    if(root == null) {
        env.error("parse_html_root returned null")
        allocator.deinit()
        return
    }
    var out = html::convert_html_root_to_string(root)
    html_view_equals(env, out.to_view(), &input)
    allocator.deinit()
}

@test
func test_html_parse_empty(env : &mut TestEnv) {
    var input = std::string_view("")
    var out = html::parse_html(input)
    if(!out.empty()) {
        env.error("empty html should parse to empty string")
    }
}
