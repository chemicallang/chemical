#react Greeting(props) {
    return <span>Hello</span>
}

@test
public func react_element_in_html_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <Greeting />
    }
    string_equals(env, page.toStringHtmlOnly(), "<span>Hello</span>");
}