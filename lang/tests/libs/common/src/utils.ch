public func view_equals(env : &mut TestEnv, str : &std::string_view, view : &std::string_view) {
    if(str.equals(view)) {
        return;
    }

    env.error("equals failure");

    var exp = std::string("expected:\"");
    exp.append_view(view)
    exp.append('"');
    env.info(exp.data())

    var got2 = std::string("got     :\"");
    got2.append_view(str)
    got2.append('"');
    env.info(got2.data())

    // print information to easily track where the in-equality comes from
    var i : size_t = 0;
    const str_len = str.size();
    const view_len = view.size();
    const min_len = if (str_len < view_len) str_len else view_len;
    while(i < min_len && str.get(i) == view.get(i)) {
        i++;
    }

    const context_size : size_t = 25;
    const start = if (i > context_size) i - context_size else 0;
    const str_end = if (i + context_size < str_len) i + context_size else str_len;
    const view_end = if (i + context_size < view_len) i + context_size else view_len;

    var expected = std::string("expected:\"");
    if (start > 0) { expected.append_view("..."); }
    expected.append_view(view.subview(start, view_end));
    if (view_end < view_len) { expected.append_view("..."); }
    expected.append('"');
    env.info(expected.data())

    var got = std::string("got     :\"");
    if (start > 0) { got.append_view("..."); }
    got.append_view(str.subview(start, str_end));
    if (str_end < str_len) { got.append_view("..."); }
    got.append('"');
    env.info(got.data())

    var diff = std::string("");
    var j : size_t = 0;
    const dots_len : size_t = if (start > 0u) 3u else 0u;
    const offset = 10u + dots_len + (i - start);
    while(j < offset) {
        diff.append(' ');
        j++;
    }
    diff.append('^');
    diff.append_view(" diff at index ");
    diff.append_integer(i as bigint);

    if (i < str_len && i < view_len) {
        diff.append_view(", got '");
        diff.append(str.get(i));
        diff.append_view("' expected '");
        diff.append(view.get(i));
        diff.append('\'');
    } else if (i < str_len) {
        diff.append_view(", got extra character '");
        diff.append(str.get(i));
        diff.append('\'');
    } else if (i < view_len) {
        diff.append_view(", missing character '");
        diff.append(view.get(i));
        diff.append('\'');
    }

    env.info(diff.data())

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