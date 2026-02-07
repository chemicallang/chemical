@test
func test_minimal_json_decodes_fine(env : &mut TestEnv) {
    var input = std::string_view("{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    var output = std::string_view("{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    test_parsed_json_equals(env, input, output)
}

@test
func test_array_value_decodes_fine(env : &mut TestEnv) {
    var input = std::string_view("[\"Hendrix Lua\", 29, false]");
    var output = std::string_view("[\"Hendrix Lua\",29,false]");
    test_parsed_json_equals(env, input, output)
}

@test
func test_escaped_strings(env : &mut TestEnv) {
    // Note: / remains / as it's not required to be escaped in JSON emitter,
    // although it's allowed in input. But since input has \\/ (\ and /)
    // they should be preserved as \ and / in the output too.
    var input = std::string_view("{\"escapes\": \"\\n\\t\\\"\\\\\\/\\b\\f\\r\"}");
    var output = std::string_view("{\"escapes\":\"\\n\\t\\\"\\\\/\\b\\f\\r\"}");
    test_parsed_json_equals(env, input, output)
}

@test
func test_unicode_surrogates(env : &mut TestEnv) {
    // Grinning Face emoji: U+1F600 -> \uD83D\uDE00
    var input = std::string_view("{\"emoji\": \"\\uD83D\\uDE00\"}");
    var output = std::string_view("{\"emoji\":\"\xf0\x9f\x98\x80\"}");
    test_parsed_json_equals(env, input, output)
}

@test
func test_complex_nesting(env : &mut TestEnv) {
    var input = std::string_view("[null,true,{\"a\":[1.0,2.0]},[[]],{}]");
    test_parsed_json_equals(env, input, input)
}

@test
func test_large_number(env : &mut TestEnv) {
    var input = std::string_view("123456789012345678901234567890");
    test_parsed_json_equals(env, input, input)
}

@test
func test_invalid_trailing_comma(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(256, 8192);
    var doc = std::string_view("{\"a\":1,}");
    var r = parser.parse(doc.data(), doc.size(), ph);
    if (r.ok) {
        env.error("Expected error for trailing comma");
    }
}

@test
func test_object_in_array_succeeds(env : &mut TestEnv) {
    var input = std::string_view("[{ \"name\" : \"Criminal Joe\" }, 999, true]");
    var output = std::string_view("[{\"name\":\"Criminal Joe\"},999,true]");
    test_parsed_json_equals(env, input, output)
}

@test
func test_number_formats(env : &mut TestEnv) {
    const tests = [
        "0",
        "-0",
        "123",
        "-123",
        "1.23",
        "-1.23",
        "1.23e4",
        "1.23E4",
        "1.23e+4",
        "1.23E-4",
        "-1.23e-4"
    ];
    const tests_view = std::span<*char>(tests)
    for (var i = 0; i < tests_view.size(); i++) {
        var input = std::string_view(tests[i]);
        test_parsed_json_equals(env, input, input);
    }
}

@test
func test_empty_containers(env : &mut TestEnv) {
    test_parsed_json_equals(env, std::string_view("{}"), std::string_view("{}"));
    test_parsed_json_equals(env, std::string_view("[]"), std::string_view("[]"));
}

@test
func test_whitespace_tolerance(env : &mut TestEnv) {
    var input = std::string_view("  {  \"key\"  :  [  1  ,  2  ]  }  ");
    var output = std::string_view("{\"key\":[1,2]}");
    test_parsed_json_equals(env, input, output);
}

@test
func test_invalid_trailing_data(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(128, 4096);
    var doc = std::string_view("{\"a\":1} trailing");
    var r = parser.parse(doc.data(), doc.size(), ph);
    if (r.ok) {
        env.error("Expected error for trailing data");
    }
}

@test
func test_invalid_missing_value(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(128, 4096);
    var doc = std::string_view("[1, , 2]");
    var r = parser.parse(doc.data(), doc.size(), ph);
    if (r.ok) {
        env.error("Expected error for missing value in array");
    }
}
