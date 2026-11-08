public func view_equals(env : &mut TestEnv, str : &std::string_view, view : &std::string_view) {
    if(str.equals(view)) {
        return;
    }

    env.error("equals failure");

    var expected = std::string("expected:\"");
    expected.append_view(view)
    expected.append('"');
    test_env::ffi::log_cstr(env, LogType.Information, expected.data(), 0, 0)

    var got = std::string("got     :\"");
    got.append_view(str)
    got.append('"');
    test_env::ffi::log_cstr(env, LogType.Information, got.data(), 0, 0)
}

func test_parsed_json_equals(env : &mut TestEnv, doc : &std::string_view, other : &std::string_view) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(256, 8192);
    var r = json::raw::parse(&mut parser, doc.data(), doc.size(), &mut ph);
    if (!r.ok) {
        // TODO: support expressive strings in test environment
        env.error("Parse error")
        return;
    }
    var output = std::string()
    output.append_value(&ph.root)
    view_equals(env, output.to_view(), other)
}