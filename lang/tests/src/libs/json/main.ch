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