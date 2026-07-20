@test
public func random_keyword_values_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-align : center;
        display : block;
        position : absolute;
        overflow : hidden;
        float : left;
        align-content : center;
        justify-content : space-between;
        flex-direction : column;
        flex-wrap : wrap;
        align-self : stretch;
    }
    css_equals(env, page.toStringCssOnly(), "text-align:center;display:block;position:absolute;overflow:hidden;float:left;align-content:center;justify-content:space-between;flex-direction:column;flex-wrap:wrap;align-self:stretch;");
}

@test
public func random_dynamic_keyword_values_work(env : &mut TestEnv) {
    var page = HtmlPage()
    var dynTextAlign = "center";
    var dynDisplay = "block";
    var dynPosition = "absolute";
    var dynOverflow = "hidden";
    var dynFloat = "left";
    var dynAlignContent = "center";
    var dynJustifyContent = "space-between";
    var dynFlexDirection = "column";
    var dynFlexWrap = "wrap";
    var dynAlignSelf = "stretch";
    #css {
        text-align : {dynTextAlign};
        display : {dynDisplay};
        position : {dynPosition};
        overflow : {dynOverflow};
        float : {dynFloat};
        align-content : {dynAlignContent};
        justify-content : {dynJustifyContent};
        flex-direction : {dynFlexDirection};
        flex-wrap : {dynFlexWrap};
        align-self : {dynAlignSelf};
    }
    css_equals(env, page.toStringCssOnly(), "text-align:center;display:block;position:absolute;overflow:hidden;float:left;align-content:center;justify-content:space-between;flex-direction:column;flex-wrap:wrap;align-self:stretch;");
}

@test
public func border_style_property_works(env : &mut TestEnv) {
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
public func animation_timing_function_property_works(env : &mut TestEnv) {
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
public func mask_border_mode_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        mask-border-mode:alpha;
        mask-border-mode:luminance;
    }
    css_equals(env, page.toStringCssOnly(), "mask-border-mode:alpha;mask-border-mode:luminance;");
}

@test
public func mask_border_repeat_property_works(env : &mut TestEnv) {
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
public func text_decoration_skip_ink_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-decoration-skip-ink:auto;
        text-decoration-skip-ink:none;
    }
    css_equals(env, page.toStringCssOnly(), "text-decoration-skip-ink:auto;text-decoration-skip-ink:none;");
}

@test
public func text_underline_position_property_works(env : &mut TestEnv) {
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
public func font_optical_sizing_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-optical-sizing:auto;
        font-optical-sizing:none;
    }
    css_equals(env, page.toStringCssOnly(), "font-optical-sizing:auto;font-optical-sizing:none;");
}

@test
public func transition_timing_function_property_works(env : &mut TestEnv) {
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

@test
public func vector_effect_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        vector-effect:none;
        vector-effect:non-scaling-stroke;
    }
    css_equals(env, page.toStringCssOnly(), "vector-effect:none;vector-effect:non-scaling-stroke;");
}

@test
public func forced_color_adjust_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        forced-color-adjust:auto;
        forced-color-adjust:none;
    }
    css_equals(env, page.toStringCssOnly(), "forced-color-adjust:auto;forced-color-adjust:none;");
}

@test
public func color_scheme_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color-scheme:light;
        color-scheme:dark;
    }
    css_equals(env, page.toStringCssOnly(), "color-scheme:light;color-scheme:dark;");
}

@test
public func print_color_adjust_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        print-color-adjust:economy;
        print-color-adjust:exact;
    }
    css_equals(env, page.toStringCssOnly(), "print-color-adjust:economy;print-color-adjust:exact;");
}

@test
public func overscroll_behavior_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        overscroll-behavior:auto;
        overscroll-behavior:contain;
        overscroll-behavior:none;
    }
    css_equals(env, page.toStringCssOnly(), "overscroll-behavior:auto;overscroll-behavior:contain;overscroll-behavior:none;");
}

@test
public func page_orientation_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        page-orientation:portrait;
        page-orientation:landscape;
    }
    css_equals(env, page.toStringCssOnly(), "page-orientation:portrait;page-orientation:landscape;");
}