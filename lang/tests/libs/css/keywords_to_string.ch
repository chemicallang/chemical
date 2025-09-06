@test
func border_style_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-style : none;
        border-style : hidden;
        border-style : dotted;
        border-style : dashed;
        border-style : solid;
        border-style : double;
        border-style : groove;
        border-style : ridge;
        border-style : inset;
        border-style : outset;
    }
    css_equals(env, page.toStringCssOnly(), "border-style:none;border-style:hidden;border-style:dotted;border-style:dashed;border-style:solid;border-style:double;border-style:groove;border-style:ridge;border-style:inset;border-style:outset;");
}

@test
func animation_timing_function_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        animation-timing-function : ease;
        animation-timing-function : linear;
        animation-timing-function : ease-in;
        animation-timing-function : ease-out;
        animation-timing-function : ease-in-out;
        animation-timing-function : step-start;
        animation-timing-function : step-end;
    }
    css_equals(env, page.toStringCssOnly(), "animation-timing-function:ease;animation-timing-function:linear;animation-timing-function:ease-in;animation-timing-function:ease-out;animation-timing-function:ease-in-out;animation-timing-function:step-start;animation-timing-function:step-end;");
}

@test
func mask_border_mode_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        mask-border-mode:alpha;
        mask-border-mode:luminance;
    }
    css_equals(env, page.toStringCssOnly(), "mask-border-mode:alpha;mask-border-mode:luminance;");
}

@test
func mask_border_repeat_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        mask-border-repeat:stretch;
        mask-border-repeat:repeat;
        mask-border-repeat:round;
        mask-border-repeat:space;
    }
    css_equals(env, page.toStringCssOnly(), "mask-border-repeat:stretch;mask-border-repeat:repeat;mask-border-repeat:round;mask-border-repeat:space;");
}

@test
func text_decoration_skip_ink_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-decoration-skip-ink:auto;
        text-decoration-skip-ink:none;
    }
    css_equals(env, page.toStringCssOnly(), "text-decoration-skip-ink:auto;text-decoration-skip-ink:none;");
}

@test
func text_underline_position_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-underline-position:auto;
        text-underline-position:under;
        text-underline-position:left;
        text-underline-position:right;
        text-underline-position:from-font;
    }
    css_equals(env, page.toStringCssOnly(), "text-underline-position:auto;text-underline-position:under;text-underline-position:left;text-underline-position:right;text-underline-position:from-font;");
}

@test
func font_optical_sizing_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-optical-sizing:auto;
        font-optical-sizing:none;
    }
    css_equals(env, page.toStringCssOnly(), "font-optical-sizing:auto;font-optical-sizing:none;");
}

@test
func transition_timing_function_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transition-timing-function:ease;
        transition-timing-function:linear;
        transition-timing-function:ease-in;
        transition-timing-function:ease-out;
        transition-timing-function:ease-in-out;
        transition-timing-function:step-start;
        transition-timing-function:step-end;
    }
    css_equals(env, page.toStringCssOnly(), "transition-timing-function:ease;transition-timing-function:linear;transition-timing-function:ease-in;transition-timing-function:ease-out;transition-timing-function:ease-in-out;transition-timing-function:step-start;transition-timing-function:step-end;");
}