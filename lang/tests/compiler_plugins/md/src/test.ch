@test
public func md_plain_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #md
Hello World
#endmd
    string_equals(env, page.toStringHtmlOnly(), "<p class=\"md-p\">Hello World</p>\n");
}

@test
public func md_paragraph_and_bold_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #md
Hello **bold** world
#endmd
    string_equals(env, page.toStringHtmlOnly(), "<p class=\"md-p\">Hello <strong class=\"md-bold\">bold</strong> world</p>\n");
}

@test
public func md_interpolation_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var name = "World"
    #md
Hello ${name}!
#endmd
    string_equals(env, page.toStringHtmlOnly(), "<p class=\"md-p\">Hello World!</p>\n");
}
