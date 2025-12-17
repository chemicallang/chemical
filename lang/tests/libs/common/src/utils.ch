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

public func string_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {
    view_equals(env, str.to_view(), view)
}

public func css_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    if(str.size() < 10) {
        env.error("css less than expected length");
    }

    const start = str.data() + 9
    const end = str.data() + str.size() - 1;

    view_equals(env, std::string_view(start, end - start), view)

}

public func compl_css_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    if(str.size() < 10) {
        env.error("css less than expected length");
    }

    view_equals(env, str.to_view(), view)

}