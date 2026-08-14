@test
func test_js_parse_while_loop(env : &mut TestEnv) {
    var input = std::string_view("while(x < 10){x += 1;}")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_do_while(env : &mut TestEnv) {
    var input = std::string_view("do{x += 1;} while(x < 10);")
    var out = js::parse_js(input)
    var expected = std::string_view("do {x += 1;} while(x < 10);")
    js_view_equals(env, out.to_view(), &expected)
}

@test
func test_js_parse_ternary(env : &mut TestEnv) {
    var input = std::string_view("var y = x > 0 ? 1 : 2;")
    var out = js::parse_js(input)
    var expected = std::string_view("var y = (x > 0 ? 1 : 2);")
    js_view_equals(env, out.to_view(), &expected)
}

@test
func test_js_parse_array_literal(env : &mut TestEnv) {
    var input = std::string_view("var a = [1, 2, 3];")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_string_concat(env : &mut TestEnv) {
    var input = std::string_view("var s = \"a\" + \"b\";")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_member_call(env : &mut TestEnv) {
    var input = std::string_view("obj.method(1, 2);")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_logical_ops(env : &mut TestEnv) {
    var input = std::string_view("var b = a && c || d;")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_increment(env : &mut TestEnv) {
    var input = std::string_view("x++;")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_function_call(env : &mut TestEnv) {
    var input = std::string_view("foo(1, 2);")
    test_js_roundtrip(env, &input)
}

@test
func test_js_parse_member_assign(env : &mut TestEnv) {
    var input = std::string_view("var o = {a: 1}; o.a = 2;")
    var out = js::parse_js(input)
    var expected = std::string_view("var o = { a: 1 };o.a = 2;")
    js_view_equals(env, out.to_view(), &expected)
}

@test
func test_js_parse_else_if_chain(env : &mut TestEnv) {
    var input = std::string_view("if(x){return 1;} else if(y){return 2;} else {return 3;}")
    test_js_roundtrip(env, &input)
}
