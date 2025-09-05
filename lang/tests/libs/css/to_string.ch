@test
func css_color_property_with_hex_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : #e3e3e3;
    }
    css_equals(env, page.toStringCssOnly(), "color:#e3e3e3;");
}