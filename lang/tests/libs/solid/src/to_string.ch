#solid Greeting(props) {
    return <span>Hello</span>
}

@test
public func solid_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<script>$_sm(document.currentScript, Greeting, {});</script>");
}

@test
public func solid_element_in_html_works_head_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHeadJsOnly(), "page head js");
}