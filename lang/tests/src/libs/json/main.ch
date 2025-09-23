@test
func test_minimal_json_decodes_fine(env : &mut TestEnv) {
    var input = std::string_view("\{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    var output = std::string_view("\{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    test_parsed_json_equals(env, input, output)
}