@test
func basic_html_structure_works(env : &mut TestEnv) {

    var page = HtmlPage()
    #html {
        Normal Text
    }
    if(!page.toString().equals("Normal Text")) {
        env.error("couldn't handle normal text");
    }

}