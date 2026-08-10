using namespace std;

func css_view_equals(env : &mut TestEnv, got : &std::string_view, expected : &std::string_view) {
    if(got.equals(expected)) {
        return;
    }
    env.error("css parse mismatch");
    var expected_s = std::string("expected:\"");
    expected_s.append_view(expected)
    expected_s.append('"');
    env.info(expected_s.data())

    var got_s = std::string("got     :\"");
    got_s.append_view(got)
    got_s.append('"');
    env.info(got_s.data())
}

@test
public func test_css_parse_basic_rule(env : &mut TestEnv) {
    const view = std::string_view("body { color: red }")
    var out = css::parse_css(view)
    var expected = std::string_view("body { color:red; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_parse_multiple_rules(env : &mut TestEnv) {
    const view = std::string_view("h1:hover { color: blue } .cls { margin: 10px }")
    var out = css::parse_css(view)
    var expected = std::string_view("h1:hover { color:blue; } .cls { margin:10px; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_parse_media_query(env : &mut TestEnv) {
    const view = std::string_view("@media screen and (max-width: 600px) { body { font-size: 12px } }")
    var out = css::parse_css(view)
    var expected = std::string_view("@media screen and (max-width: 600px) { body { font-size:12px; } }")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_parse_keyframes(env : &mut TestEnv) {
    const view = std::string_view("@keyframes spin { from { transform: rotate(0deg) } to { transform: rotate(360deg) } }")
    var out = css::parse_css(view)
    var expected = std::string_view("@keyframes spin { from { transform:rotate(0deg); } to { transform:rotate(360deg); } }")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_parse_declaration_values(env : &mut TestEnv) {
    const view = std::string_view(".btn { background: #ff0000; border-radius: 4px 8px; padding: 10px 20px; font-family: Arial, sans-serif }")
    var out = css::parse_css(view)
    var expected = std::string_view(".btn { background: #ff0000;border-radius:4px 8px;padding:10px 20px;font-family:Arial,sans-serif; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_tokenize(env : &mut TestEnv) {
    const view = std::string_view(".cls { margin: 10px }")
    var tokens = css::tokenize_css(view)
    if(tokens.size() < 5) {
        env.error("expected at least 5 tokens")
    }
}
