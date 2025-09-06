@test
func color_property_with_hex_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : #e3e3e3;
    }
    css_equals(env, page.toStringCssOnly(), "color:#e3e3e3;");
}

@test
func color_property_with_named_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
    }
    css_equals(env, page.toStringCssOnly(), "color:red;");
}

@test
func color_property_with_rgb_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : rgb(0, 0, 0);
    }
    css_equals(env, page.toStringCssOnly(), "color:rgb(0 0 0);");
}

@test
func border_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border : 1px solid red;
    }
    css_equals(env, page.toStringCssOnly(), "border:1px solid red;");
}

@test
func border_radius_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-radius : 4px;
    }
    css_equals(env, page.toStringCssOnly(), "border-radius:4px;");
}