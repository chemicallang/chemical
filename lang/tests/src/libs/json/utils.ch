public func view_equals(env : &mut TestEnv, str : &std::string_view, view : &std::string_view) {
    if(str.equals(view)) {
        return;
    }

    env.error("equals failure");

    var expected = std::string("expected:\"");
    expected.append_view(view)
    expected.append('"');
    env.info(expected.data())

    var got = std::string("got     :\"");
    got.append_view(str)
    got.append('"');
    env.info(got.data())
}

func test_parsed_json_equals(env : &mut TestEnv, doc : &std::string_view, other : &std::string_view) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(256, 8192);
    var r = parser.parse(doc.data(), doc.size(), ph);
    if (!r.ok) {
        // TODO: support expressive strings in test environment
        env.error("Parse error")
        return;
    }
    var output = std::string()
    var printer = JsonStringBuilder { ptr : output }
    printer.append_value(ph.root)
    view_equals(env, output.to_view(), other)
}