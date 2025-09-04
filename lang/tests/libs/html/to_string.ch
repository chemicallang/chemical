public func html_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

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
func text_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        Normal Text}
    html_equals(env, page.toStringHtmlOnly(), "Normal Text");
}

@test
func element_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>Normal Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>Normal Text</div>");
}