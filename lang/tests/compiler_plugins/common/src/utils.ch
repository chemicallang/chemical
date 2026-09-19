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

// Compare the declarations of a `#css` block, ignoring the wrapper the converter
// puts around them: `.hXXXXXX{ ... }` for a class-scoped block (`#css` in value
// position) and `:root{ ... }` for a global block (`#css` as a statement).
public func css_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    var start_offset : size_t = 0
    var end_offset : size_t = 0
    if(str.size() > 9 && str.data()[0] == '.' && str.data()[8] == '{') {
        start_offset = 9
        end_offset = 1
    } else if(str.size() > 6 && str.data()[0] == ':' && str.data()[5] == '{') {
        start_offset = 6
        end_offset = 1
    }

    const start = str.data() + start_offset
    const end = str.data() + str.size() - end_offset;

    view_equals(env, std::string_view(start, end - start), view)

}

// Compare a stylesheet that begins with an at-rule (keyframes). A block that
// only contains at-rules used to emit an empty root scope (`.rXXXXXX_{}`) in
// front of them; a global block (statement position) no longer does, so skip
// that prefix only when it is present.
public func css_at_rule_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    var start_offset : size_t = 0
    if(str.size() > 10 && str.data()[0] == '.' && str.data()[9] == '}') {
        start_offset = 10
    }

    const start = str.data() + start_offset
    const end = str.data() + str.size();

    view_equals(env, std::string_view(start, end - start), view)

}

public func compl_css_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    if(str.size() < 10) {
        env.error("css less than expected length");
    }

    view_equals(env, str.to_view(), view)

}