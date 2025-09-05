@test
func css_color_property_with_hex_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : #e3e3e3;
    }
    css_equals(env, page.toStringCssOnly(), "color:#e3e3e3;");
}

@test
func css_color_property_with_named_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
    }
    css_equals(env, page.toStringCssOnly(), "color:red;");
}

@test
func css_color_property_with_rgb_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : rgb(0, 0, 0);
    }
    css_equals(env, page.toStringCssOnly(), "color:rgb(0 0 0);");
}