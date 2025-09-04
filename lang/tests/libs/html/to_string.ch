@test
func basic_html_structure_works(env : &mut TestEnv) {

    var page = HtmlPage()
    #html {
        <div>Normal Text</div>
    }
    if(!page.toString().equals(std::string("<div>Normal Text</div>"))) {
        env.error("couldn't handle normal text");
    }

}