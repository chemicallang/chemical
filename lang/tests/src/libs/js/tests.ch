using namespace std;
using namespace js;

func js_view_equals(env : &mut TestEnv, got : &std::string_view, expected : &std::string_view) {
    if(got.equals(expected)) {
        return;
    }
    env.error("js parse mismatch");
    var expected_s = std::string("expected:\"");
    expected_s.append_view(expected)
    expected_s.append('"');
    env.info(expected_s.data())

    var got_s = std::string("got     :\"");
    got_s.append_view(got)
    got_s.append('"');
    env.info(got_s.data())
}

func test_js_roundtrip(env : &mut TestEnv, input : &std::string_view) {
    var view = std::string_view(input.data(), input.size())
    var out = js::parse_js(view)
    js_view_equals(env, out.to_view(), &view)
}

@test
func test_js_parse_var_decl(env : &mut TestEnv) {
    var input = std::string_view("var x = 1 + 2;")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_function_decl(env : &mut TestEnv) {
    var input = std::string_view("function add(a, b){return a + b;}")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_if_else(env : &mut TestEnv) {
    var input = std::string_view("if(x > 5){x = 1;} else {x = 2;}")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_for_loop(env : &mut TestEnv) {
    var input = std::string_view("for(var i = 0; i < 10; i++){total += i;}")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_arrow_func(env : &mut TestEnv) {
    var input = std::string_view("const f = (a, b) => a * b;")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_object_literal(env : &mut TestEnv) {
    var input = std::string_view("var obj = { name: \"test\", value: 42 };")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_comment(env : &mut TestEnv) {
    var input = std::string_view("// hello\nvar x = 1;")
    var out = js::parse_js(input)
    // comments are not stored in the AST, so they are dropped by the converter
    var expected = std::string_view("var x = 1;")
    js_view_equals(env, out.to_view(), &expected)
}

@test
func test_js_tokenize(env : &mut TestEnv) {
    var input = std::string_view("var x = 1;")
    var tokens = js::tokenize_js(input)
    if(tokens.size() == 0) {
        env.error("tokenize_js returned no tokens")
        return
    }
}

@test
func test_js_parse_empty(env : &mut TestEnv) {
    var input = std::string_view("")
    var out = js::parse_js(input)
    if(!out.empty()) {
        env.error("empty js should parse to empty string")
    }
}
