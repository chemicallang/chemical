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
func test_object_in_array_succeeds(env : &mut TestEnv) {
    var input = std::string_view("[{ \"name\" : \"Criminal Joe\" }, 999, true]");
    var output = std::string_view("[{\"name\":\"Criminal Joe\"},999,true]");
    test_parsed_json_equals(env, input, output)
}

@test
func test_deeply_nested_structures(env : &mut TestEnv) {
    var input = std::string_view("{\"a\":{\"b\":{\"c\":[1,{\"d\":true}]}}}");
    var output = std::string_view("{\"a\":{\"b\":{\"c\":[1,{\"d\":true}]}}}");
    test_parsed_json_equals(env, input, output)
}

@test
func test_escaped_strings(env : &mut TestEnv) {
    var input = std::string_view("{\"escapes\": \"\\n\\t\\\"\\\\\\/\\b\\f\\r\"}");
    var output = std::string_view("{\"escapes\":\"\\n\\t\\\"\\\\/\\b\\f\\r\"}");
    test_parsed_json_equals(env, input, output)
}

@test
func test_unicode_escapes(env : &mut TestEnv) {
    var input = std::string_view("{\"unicode\": \"\\u0041\\u0042\\u0043\"}");
    var output = std::string_view("{\"unicode\":\"ABC\"}");
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