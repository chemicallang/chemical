public func html_equals(env : &mut TestEnv, page : &mut HtmlPage, view : &std::string_view) {

    var str = page.toStringHtmlOnly()
    if(str.equals_view(view)) {
        return;
    }

    env.error("equals failure");

    var expected = std::string("expected:");
    expected.append_view(view)
    env.info(expected.data())

    var got = std::string("got:");
    got.append_string(str)
    env.info(got.data())

}

@test
func basic_html_structure_works(env : &mut TestEnv) {

    var page = HtmlPage()
    #html {
        <div>Normal Text</div>
    }
    html_equals(env, page, "<div>Normal Text</div>");

}