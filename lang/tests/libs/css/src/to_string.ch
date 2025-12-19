@test
public func global_values_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color:inherit;
        color:initial;
        color:unset;
    }
    css_equals(env, page.toStringCssOnly(), "color:inherit;color:initial;color:unset;");
}

@test
public func chemical_dynamic_values_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color:{"red"};
    }
    css_equals(env, page.toStringCssOnly(), "color:red;");
}

@test
public func chemical_dynamic_values_work2(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = true
    var color = if(condition) "green" else "blue"
    #css {
        color:{color};
    }
    css_equals(env, page.toStringCssOnly(), "color:green;");
}

@test
public func chemical_dynamic_values_work3(env : &mut TestEnv) {
    var page = HtmlPage()
    var condition = false
    var color = if(condition) "green" else "blue"
    #css {
        color:{color};
    }
    css_equals(env, page.toStringCssOnly(), "color:blue;");
}

@test
public func chemical_dynamic_values_work5(env : &mut TestEnv) {
    var page = HtmlPage()
    var color = "green";
    var length = "2px"
    #css {
        border : {length} solid {color};
    }
    css_equals(env, page.toStringCssOnly(), "border:2px solid green;");
}

@test
public func css_variables_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color:var(--custom-color);
    }
    css_equals(env, page.toStringCssOnly(), "color:var(--custom-color);");
}

@test
public func css_variables_work2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        width:var(--custom-width);
    }
    css_equals(env, page.toStringCssOnly(), "width:var(--custom-width);");
}

@test
public func css_variables_work3(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border:var(--custom-width) solid green;
    }
    css_equals(env, page.toStringCssOnly(), "border:var(--custom-width) solid green;");
}

@test
public func color_property_with_hex_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : #e3e3e3;
    }
    css_equals(env, page.toStringCssOnly(), "color:#e3e3e3;");
}

@test
public func color_property_with_named_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
    }
    css_equals(env, page.toStringCssOnly(), "color:red;");
}

@test
public func color_property_with_rgb_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : rgb(0, 0, 0);
    }
    css_equals(env, page.toStringCssOnly(), "color:rgb(0 0 0);");
}

@test
public func border_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border : 1px solid red;
    }
    css_equals(env, page.toStringCssOnly(), "border:1px solid red;");
}

@test
public func border_radius_property_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-radius : 4px;
    }
    css_equals(env, page.toStringCssOnly(), "border-radius:4px;");
}

@test
public func margin_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        margin: 10px 20px 30px 40px;
    }
    css_equals(env, page.toStringCssOnly(), "margin:10px 20px 30px 40px;");
}

@test
public func padding_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        padding: 1rem 2rem;
    }
    css_equals(env, page.toStringCssOnly(), "padding:1rem 2rem;");
}

@test
public func font_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font: italic small-caps bold 16px/1.5 "Open Sans", Arial, sans-serif;
    }
    css_equals(env, page.toStringCssOnly(), "font:italic small-caps bold 16px/1.5 \"Open Sans\",Arial,sans-serif;");
}

@test
public func font_family_with_fallbacks_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-family: "Times New Roman", Georgia, serif;
    }
    css_equals(env, page.toStringCssOnly(), "font-family:\"Times New Roman\",Georgia,serif;");
}

@test
public func background_image_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-image: url("img.png");
    }
    css_equals(env, page.toStringCssOnly(), "background-image:url(\"img.png\");");
}

@test
public func background_image_with_linear_gradient_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-image: linear-gradient(black, white);
    }
    css_equals(env, page.toStringCssOnly(), "background-image:linear-gradient(black,white);");
}

@test
public func background_image_with_linear_gradient_works2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-image: linear-gradient(red 0%, orange 10%, orange 30%, yellow 50%, yellow 70%, green 90%, green 100%);
    }
    css_equals(env, page.toStringCssOnly(), "background-image:linear-gradient(red 0%,orange 10%,orange 30%,yellow 50%,yellow 70%,green 90%,green 100%);");
}

@test
public func background_image_with_linear_gradient_works3(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-image: linear-gradient(45deg, red 0 50%, blue 50% 100%);
    }
    css_equals(env, page.toStringCssOnly(), "background-image:linear-gradient(45deg,red 0 50%,blue 50% 100%);");
}

@test
public func background_image_with_linear_gradient_works4(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-image: linear-gradient(to left top, blue, red);
    }
    css_equals(env, page.toStringCssOnly(), "background-image:linear-gradient(to left top,blue,red);");
}

@test
public func transition_shorthand_multiple_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transition: opacity 0.3s ease-in-out 0s, transform 200ms linear;
    }
    css_equals(env, page.toStringCssOnly(), "transition:opacity 0.3s ease-in-out 0s,transform linear 200ms;");
}

@test
public func transition_multiple_properties_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transition: opacity 250ms ease-in, transform 400ms cubic-bezier(0.2,0.8,0.2,1);
    }
    css_equals(env, page.toStringCssOnly(), "transition:opacity 250ms ease-in,transform cubic-bezier(0.2,0.8,0.2,1) 400ms;");
}

@test
public func box_shadow_multiple_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        box-shadow: 0 2px 4px #000000, 0 1px 3px #111111;
    }
    css_equals(env, page.toStringCssOnly(), "box-shadow:0 2px 4px #000000,0 1px 3px #111111;");
}

@test
public func box_shadow_complex_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        box-shadow: inset 0 2px 4px #000, 0 4px 8px rgba(0,0,0,0.2);
    }
    css_equals(env, page.toStringCssOnly(), "box-shadow:inset 0 2px 4px #000,0 4px 8px rgba(0 0 0 / 0.2);");
}

@test
public func complex_shadow_spread_and_inset_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        box-shadow: 0 2px 6px -1px rgba(0,0,0,0.3), inset 0 -3px 0 0 rgba(255,255,255,0.1);
    }
    css_equals(env, page.toStringCssOnly(), "box-shadow:0 2px 6px -1px rgba(0 0 0 / 0.3),inset 0 -3px 0 0 rgba(255 255 255 / 0.1);");
}

@test
public func text_shadow_multiple_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-shadow: 1px 1px 2px #000, 0 0 1px #333;
    }
    css_equals(env, page.toStringCssOnly(), "text-shadow:1px 1px 2px #000,0 0 1px #333;");
}

@test
public func text_shadow_multi_layer_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-shadow: 0 1px 0 #fff, 0 2px 3px rgba(0,0,0,0.4);
    }
    css_equals(env, page.toStringCssOnly(), "text-shadow:0 1px 0 #fff,0 2px 3px rgba(0 0 0 / 0.4);");
}

@test
public func padding_four_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        padding: 4px 8px 12px 16px;
    }
    css_equals(env, page.toStringCssOnly(), "padding:4px 8px 12px 16px;");
}

@test
public func padding_mixed_units_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        padding: 1rem 2vw 3vh 4ch;
    }
    css_equals(env, page.toStringCssOnly(), "padding:1rem 2vw 3vh 4ch;");
}

@test
public func margin_all_sides_explicit_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        margin: 5px 10px 15px 20px;
    }
    css_equals(env, page.toStringCssOnly(), "margin:5px 10px 15px 20px;");
}

@test
public func font_family_multiple_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-family: "Helvetica Neue", Helvetica, Arial, sans-serif;
    }
    css_equals(env, page.toStringCssOnly(), "font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;");
}

@test
public func border_shorthand_with_radius_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border: 2px dashed #0a0a0a;
        border-radius: 8px 4px;
    }
    css_equals(env, page.toStringCssOnly(), "border:2px dashed #0a0a0a;border-radius:8px 4px;");
}

@test
public func transform_multiple_functions_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transform: translate(10px, 20px) rotate(45deg) scale(1.2);
    }
    css_equals(env, page.toStringCssOnly(), "transform:translate(10px,20px) rotate(45deg) scale(1.2);");
}

@test
public func transform_many_functions_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transform: translate(10px 20px) rotate(30deg) scale(1.1, 0.9) skewX(10deg);
    }
    css_equals(env, page.toStringCssOnly(), "transform:translate(10px,20px) rotate(30deg) scale(1.1,0.9) skewX(10deg);");
}

@test
public func transition_timing_function_keywords_and_params_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transition-timing-function: steps(4, end);
    }
    css_equals(env, page.toStringCssOnly(), "transition-timing-function:steps(4,end);");
}

@test
public func gap_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        gap: 10px 20px;
    }
    css_equals(env, page.toStringCssOnly(), "gap:10px 20px;");
}

@test
public func row_and_column_gap_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        row-gap: 8px;
        column-gap: 12px;
    }
    css_equals(env, page.toStringCssOnly(), "row-gap:8px;column-gap:12px;");
}

@test
public func simplest_background_works_with_named_color(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: red;
    }
    css_equals(env, page.toStringCssOnly(), "background: red;");
}

@test
public func background_shorthand_with_url_and_size_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: url("img.png") no-repeat center/cover;
    }
    css_equals(env, page.toStringCssOnly(), "background:url(\"img.png\") center/cover no-repeat;");
}

@test
public func background_linear_gradient_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: linear-gradient(45deg, #ff0000 0%, #0000ff 100%);
    }
    css_equals(env, page.toStringCssOnly(), "background:linear-gradient(45deg,#ff0000 0%,#0000ff 100%);");
}

@test
public func background_with_position_and_size_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: url("hero.jpg") no-repeat 20% 30%/contain;
    }
    css_equals(env, page.toStringCssOnly(), "background:url(\"hero.jpg\") 20% 30%/contain no-repeat;");
}

@test
public func multi_backgrounds_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: url("a.png") left top no-repeat, linear-gradient(180deg,#fff 0%,#eee 100%);
    }
    css_equals(env, page.toStringCssOnly(), "background:url(\"a.png\") left top no-repeat, linear-gradient(180deg,#fff 0%,#eee 100%);");
}

@test
public func background_multiple_layers_with_positions_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: url("a.png") left top no-repeat, url("b.png") right bottom repeat;
    }
    css_equals(env, page.toStringCssOnly(), "background:url(\"a.png\") left top no-repeat, url(\"b.png\") right bottom repeat;");
}

@test
public func background_gradient_and_image_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: linear-gradient(90deg,#fff 0%,#ddd 100%), url("bg.jpg") center/cover;
    }
    css_equals(env, page.toStringCssOnly(), "background:linear-gradient(90deg,#fff 0%,#ddd 100%), url(\"bg.jpg\") center/cover;");
}

@test
public func multi_value_background_properties_with_gradients_and_positions_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: linear-gradient(45deg,#f00 0%,#0f0 50%,#00f 100%) center/80% no-repeat, url("texture.png") repeat;
    }
    css_equals(env, page.toStringCssOnly(), "background:linear-gradient(45deg,#f00 0%,#0f0 50%,#00f 100%) center/80% no-repeat, url(\"texture.png\") repeat;");
}

@test
public func gradient_position_sizes_and_percentages_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: linear-gradient(135deg, rgba(255,0,0,0.8) 10%, rgba(0,0,255,0.6) 90%) center/60% no-repeat;
    }
    css_equals(env, page.toStringCssOnly(), "background:linear-gradient(135deg,rgba(255 0 0 / 0.8) 10%,rgba(0 0 255 / 0.6) 90%) center/60% no-repeat;");
}

@test
public func gradient_repeating_and_conic_variants_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background: repeating-linear-gradient(0deg,#000 0 5px,#fff 5px 10px), conic-gradient(from 45deg, #f00, #0f0 50%, #00f);
    }
    css_equals(env, page.toStringCssOnly(), "background:repeating-linear-gradient(0deg,#000 0 5px,#fff 5px 10px), conic-gradient(from 45deg,#f00,#0f0 50%,#00f);");
}

@test
public func media_queries1(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        @media screen {
            background-color : red;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}@media screen  { ");
    expected.append_view(classView)
    expected.append_view(" { background-color:red; } }");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func media_queries_complex(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        @media screen and (min-width: 480px) {
            color: blue;
        }
        @media only screen and (max-width: 600px) {
            color: green;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{");
    // Note: The parser currently adds a space after every token in the media query.
    // So "min-width: 480px" becomes "min-width : 480 px "
    expected.append_view("}@media screen and ( min-width : 480 px )  { ");
    expected.append_view(classView)
    expected.append_view(" { color:blue; } }");
    expected.append_view("@media only screen and ( max-width : 600 px )  { ");
    expected.append_view(classView)
    expected.append_view(" { color:green; } }");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        &.blue {
            color: blue;
        }
        &#yellow{
            color: yellow;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view(classView)
    expected.append_view(".blue { color:blue;}");
    expected.append_view(classView)
    expected.append_view("#yellow { color:yellow;}");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        & .blue {
            color: blue;
        }
        & #yellow {
            color: yellow;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view(classView)
    expected.append_view(" .blue { color:blue;}");
    expected.append_view(classView)
    expected.append_view(" #yellow { color:yellow;}");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test3(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        & > .blue {
            color: blue;
        }
        & > #yellow {
            color: yellow;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view(classView)
    expected.append_view(" > .blue { color:blue;}");
    expected.append_view(classView)
    expected.append_view(" > #yellow { color:yellow;}");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test4(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        & ~ .blue {
            color: blue;
        }
        & ~ #yellow {
            color: yellow;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view(classView)
    expected.append_view(" ~ .blue { color:blue;}");
    expected.append_view(classView)
    expected.append_view(" ~ #yellow { color:yellow;}");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test5(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        &:hover {
            color: red;
        }
        &::before {
            color: blue;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view(classView)
    expected.append_view(":hover { color:red;}");
    expected.append_view(classView)
    expected.append_view("::before { color:blue;}");
    compl_css_equals(env, got, expected.to_view());
}

@test
public func nested_queries_test6(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color : red;
        div {
            color: red;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(classView)
    expected.append_view("{color:red;}");
    expected.append_view("div { color:red;}");
    compl_css_equals(env, got, expected.to_view());
}

/**

@test
public func font_shorthand_full_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font: italic small-caps 600 18px/1.4 \"Open Sans\", Arial, sans-serif;
    }
    css_equals(env, page.toStringCssOnly(), "font:italic small-caps 600 18px/1.4 \"Open Sans\",Arial,sans-serif;");
}

@test
public func grid_template_columns_repeat_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        grid-template-columns: repeat(3, 1fr);
    }
    css_equals(env, page.toStringCssOnly(), "grid-template-columns:repeat(3 1fr);");
}

@test
public func flex_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        flex: 1 0 auto;
    }
    css_equals(env, page.toStringCssOnly(), "flex:1 0 auto;");
}

@test
public func outline_with_color_and_width_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        outline: 1px solid #ff0000;
    }
    css_equals(env, page.toStringCssOnly(), "outline:1px solid #ff0000;");
}

@test
public func width_with_calc_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        width: calc(100% - 20px);
    }
    css_equals(env, page.toStringCssOnly(), "width:calc(100% - 20px);");
}

@test
public func animation_shorthand_complex_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        animation: slide 3s cubic-bezier(0.4,0,0.2,1) 0.5s 2 reverse both running;
    }
    css_equals(env, page.toStringCssOnly(), "animation:slide 3s cubic-bezier(0.4,0,0.2,1) 0.5s 2 reverse both running;");
}

@test
public func filter_multiple_functions_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        filter: blur(5px) brightness(0.8) contrast(120%);
    }
    css_equals(env, page.toStringCssOnly(), "filter:blur(5px) brightness(0.8) contrast(120%);");
}

@test
public func clip_path_polygon_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        clip-path: polygon(0 0, 100% 0, 100% 100%, 0 100%);
    }
    css_equals(env, page.toStringCssOnly(), "clip-path:polygon(0 0,100% 0,100% 100%,0 100%);");
}

@test
public func list_style_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        list-style: square inside url("bullet.png");
    }
    css_equals(env, page.toStringCssOnly(), "list-style:square inside url(\"bullet.png\");");
}

@test
public func columns_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        columns: 200px 3;
    }
    css_equals(env, page.toStringCssOnly(), "columns:200px 3;");
}

@test
public func grid_template_rows_and_columns_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        grid-template-columns: 1fr 2fr 100px;
        grid-template-rows: 100px auto 1fr;
    }
    css_equals(env, page.toStringCssOnly(), "grid-template-columns:1fr 2fr 100px;grid-template-rows:100px auto 1fr;");
}

@test
public func min_max_width_height_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        min-width: 320px;
        max-width: 960px;
        min-height: 200px;
        max-height: calc(100vh - 100px);
    }
    css_equals(env, page.toStringCssOnly(), "min-width:320px;max-width:960px;min-height:200px;max-height:calc(100vh - 100px);");
}

@test
public func transform_origin_three_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transform-origin: 10px 20px 5px;
    }
    css_equals(env, page.toStringCssOnly(), "transform-origin:10px 20px 5px;");
}

@test
public func perspective_and_origin_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        perspective: 500px;
        perspective-origin: 50% 25%;
    }
    css_equals(env, page.toStringCssOnly(), "perspective:500px;perspective-origin:50% 25%;");
}

@test
public func object_position_multiple_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        object-position: 10% 90%;
    }
    css_equals(env, page.toStringCssOnly(), "object-position:10% 90%;");
}

@test
public func text_decoration_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-decoration: underline dotted #ff8800 2px;
    }
    css_equals(env, page.toStringCssOnly(), "text-decoration:underline dotted #ff8800 2px;");
}

@test
public func border_image_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-image: url("frame.png") 30 30 stretch round;
    }
    css_equals(env, page.toStringCssOnly(), "border-image:url(\"frame.png\") 30 30 stretch round;");
}

@test
public func unicode_range_and_font_face_like_value_simulation_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        // simulate a multi-part property value often seen in font-face / @supports contexts
        src: url("font.woff2") format("woff2"), url("font.woff") format("woff");
    }
    css_equals(env, page.toStringCssOnly(), "src:url(\"font.woff2\") format(\"woff2\"),url(\"font.woff\") format(\"woff\");");
}

@test
public func border_sides_different_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-top: 1px solid red;
        border-right: 2px dotted green;
        border-bottom: 3px double blue;
        border-left: 4px groove #ccc;
    }
    css_equals(env, page.toStringCssOnly(), "border-top:1px solid red;border-right:2px dotted green;border-bottom:3px double blue;border-left:4px groove #ccc;");
}

@test
public func background_position_and_size_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-position: 20% 30%;
        background-size: 50% auto;
    }
    css_equals(env, page.toStringCssOnly(), "background-position:20% 30%;background-size:50% auto;");
}

@test
public func background_attachment_and_clip_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-attachment: fixed;
        background-clip: content-box;
    }
    css_equals(env, page.toStringCssOnly(), "background-attachment:fixed;background-clip:content-box;");
}

@test
public func font_variant_and_feature_settings_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-variant: small-caps;
        font-feature-settings: \"liga\" 0, \"ss01\" 1;
    }
    css_equals(env, page.toStringCssOnly(), "font-variant:small-caps;font-feature-settings:\"liga\" 0,\"ss01\" 1;");
}

@test
public func font_face_like_src_list_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        src: url(\"font.woff2\") format(\"woff2\"), url(\"font.woff\") format(\"woff\");
    }
    css_equals(env, page.toStringCssOnly(), "src:url(\"font.woff2\") format(\"woff2\"),url(\"font.woff\") format(\"woff\");");
}

@test
public func opacity_and_filter_chain_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        opacity: 0.85;
        filter: blur(2px) saturate(1.2);
    }
    css_equals(env, page.toStringCssOnly(), "opacity:0.85;filter:blur(2px) saturate(1.2);");
}

@test
public func transform_origin_three_values_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transform-origin: 10px 20px 2px;
    }
    css_equals(env, page.toStringCssOnly(), "transform-origin:10px 20px 2px;");
}

@test
public func perspective_and_origin_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        perspective: 800px;
        perspective-origin: 40% 60%;
    }
    css_equals(env, page.toStringCssOnly(), "perspective:800px;perspective-origin:40% 60%;");
}

@test
public func animation_full_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        animation: fadeIn 1.2s ease-in-out 0s 1 normal forwards paused;
    }
    css_equals(env, page.toStringCssOnly(), "animation:fadeIn 1.2s ease-in-out 0s 1 normal forwards paused;");
}

@test
public func grid_template_with_named_areas_and_repeat_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        grid-template-columns: repeat(3, minmax(100px, 1fr));
        grid-template-rows: 100px auto;
    }
    css_equals(env, page.toStringCssOnly(), "grid-template-columns:repeat(3 minmax(100px,1fr));grid-template-rows:100px auto;");
}

@test
public func grid_gap_and_areas_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        gap: 12px 18px;
        grid-auto-rows: minmax(50px, auto);
    }
    css_equals(env, page.toStringCssOnly(), "gap:12px 18px;grid-auto-rows:minmax(50px,auto);");
}

@test
public func grid_column_row_span_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        grid-column: 1 / span 2;
        grid-row: 2 / 4;
    }
    css_equals(env, page.toStringCssOnly(), "grid-column:1 / span 2;grid-row:2 / 4;");
}

@test
public func flex_shorthand_with_basis_grow_shrink_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        flex: 2 1 150px;
    }
    css_equals(env, page.toStringCssOnly(), "flex:2 1 150px;");
}

@test
public func flex_flow_and_wrap_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        flex-flow: row wrap;
        align-content: space-between;
    }
    css_equals(env, page.toStringCssOnly(), "flex-flow:row wrap;align-content:space-between;");
}

@test
public func order_and_flex_basis_auto_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        order: 3;
        flex-basis: auto;
    }
    css_equals(env, page.toStringCssOnly(), "order:3;flex-basis:auto;");
}

@test
public func object_fit_and_position_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        object-fit: cover;
        object-position: 10% 90%;
    }
    css_equals(env, page.toStringCssOnly(), "object-fit:cover;object-position:10% 90%;");
}

@test
public func columns_shorthand_and_gap_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        columns: 200px 4;
        column-gap: 16px;
    }
    css_equals(env, page.toStringCssOnly(), "columns:200px 4;column-gap:16px;");
}

@test
public func column_rule_and_span_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        column-rule: 1px dashed #666;
        column-span: all;
    }
    css_equals(env, page.toStringCssOnly(), "column-rule:1px dashed #666;column-span:all;");
}

@test
public func list_style_full_shorthand_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        list-style: square outside url(\"bullet.png\");
    }
    css_equals(env, page.toStringCssOnly(), "list-style:square outside url(\"bullet.png\");");
}

@test
public func counter_style_and_symbols_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        counter-reset: section 1;
        counter-increment: item 2;
    }
    css_equals(env, page.toStringCssOnly(), "counter-reset:section 1;counter-increment:item 2;");
}

@test
public func table_layout_and_border_collapse_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        table-layout: fixed;
        border-collapse: collapse;
    }
    css_equals(env, page.toStringCssOnly(), "table-layout:fixed;border-collapse:collapse;");
}

@test
public func caption_side_and_empty_cells_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        caption-side: bottom;
        empty-cells: show;
    }
    css_equals(env, page.toStringCssOnly(), "caption-side:bottom;empty-cells:show;");
}

@test
public func list_marker_and_image_settings_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        list-style-image: url(\"marker.svg\");
    }
    css_equals(env, page.toStringCssOnly(), "list-style-image:url(\"marker.svg\");");
}

@test
public func outline_and_offset_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        outline: 3px solid rgba(255,0,0,0.5);
        outline-offset: 4px;
    }
    css_equals(env, page.toStringCssOnly(), "outline:3px solid rgba(255,0,0,0.5);outline-offset:4px;");
}

@test
public func resize_and_overflow_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        resize: both;
        overflow: auto;
        overflow-x: hidden;
        overflow-y: scroll;
    }
    css_equals(env, page.toStringCssOnly(), "resize:both;overflow:auto;overflow-x:hidden;overflow-y:scroll;");
}

@test
public func clip_path_circle_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        clip-path: circle(50% at 50% 50%);
    }
    css_equals(env, page.toStringCssOnly(), "clip-path:circle(50% at 50% 50%);");
}

@test
public func clip_path_polygon_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        clip-path: polygon(0 0,100% 0,100% 100%,0 100%);
    }
    css_equals(env, page.toStringCssOnly(), "clip-path:polygon(0 0,100% 0,100% 100%,0 100%);");
}

@test
public func mask_and_mask_composite_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        mask-image: linear-gradient(180deg, rgba(0,0,0,1), rgba(0,0,0,0));
        mask-composite: add;
    }
    css_equals(env, page.toStringCssOnly(), "mask-image:linear-gradient(180deg,rgba(0,0,0,1),rgba(0,0,0,0));mask-composite:add;");
}

@test
public func mix_blend_mode_and_background_blend_modes_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        mix-blend-mode: multiply;
        background-blend-mode: multiply, screen;
    }
    css_equals(env, page.toStringCssOnly(), "mix-blend-mode:multiply;background-blend-mode:multiply,screen;");
}

@test
public func object_fit_and_multivalue_object_position_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        object-fit: contain;
        object-position: 10% 20%;
    }
    css_equals(env, page.toStringCssOnly(), "object-fit:contain;object-position:10% 20%;");
}

@test
public func speak_and_voice_family_simulation_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        speak: normal;
        voice-family: \"serif\";
    }
    css_equals(env, page.toStringCssOnly(), "speak:normal;voice-family:\"serif\";");
}

@test
public func filter_complex_chain_works_2(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        filter: grayscale(100%) sepia(30%) hue-rotate(90deg) drop-shadow(2px 4px 6px #222);
    }
    css_equals(env, page.toStringCssOnly(), "filter:grayscale(100%) sepia(30%) hue-rotate(90deg) drop-shadow(2px 4px 6px #222);");
}

@test
public func image_rendering_and_sizing_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        image-rendering: crisp-edges;
        image-orientation: 90deg;
    }
    css_equals(env, page.toStringCssOnly(), "image-rendering:crisp-edges;image-orientation:90deg;");
}

@test
public func object_viewbox_and_position_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        vector-effect: non-scaling-stroke;
    }
    css_equals(env, page.toStringCssOnly(), "vector-effect:non-scaling-stroke;");
}

@test
public func caret_color_and_selection_color_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        caret-color: #ff0;
        ::selection { background: #333; color: #fff; }
    }
    css_equals(env, page.toStringCssOnly(), "caret-color:#ff0;::selection{background:#333;color:#fff;}");
}

@test
public func placeholder_and_form_control_simulation_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        input::placeholder { color: #999; opacity: 0.8; }
        input { appearance: none; border-radius: 6px; }
    }
    css_equals(env, page.toStringCssOnly(), "input::placeholder{color:#999;opacity:0.8;}input{appearance:none;border-radius:6px;}");
}

@test
public func appearance_and_user_select_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        -webkit-appearance: none;
        user-select: none;
    }
    css_equals(env, page.toStringCssOnly(), "-webkit-appearance:none;user-select:none;");
}

@test
public func text_decoration_shorthand_and_offsets_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-decoration: underline dashed #ff8800 2px;
        text-decoration-thickness: 2px;
        text-underline-offset: 4px;
    }
    css_equals(env, page.toStringCssOnly(), "text-decoration:underline dashed #ff8800 2px;text-decoration-thickness:2px;text-underline-offset:4px;");
}

@test
public func letter_spacing_and_word_spacing_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        letter-spacing: 0.02em;
        word-spacing: 0.5rem;
    }
    css_equals(env, page.toStringCssOnly(), "letter-spacing:0.02em;word-spacing:0.5rem;");
}

@test
public func line_height_and_text_indent_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        line-height: 1.6;
        text-indent: 2em;
    }
    css_equals(env, page.toStringCssOnly(), "line-height:1.6;text-indent:2em;");
}

@test
public func white_space_and_word_break_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        white-space: pre-wrap;
        word-break: break-word;
    }
    css_equals(env, page.toStringCssOnly(), "white-space:pre-wrap;word-break:break-word;");
}

@test
public func hyphens_and_text_wrap_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        hyphens: auto;
        overflow-wrap: break-word;
    }
    css_equals(env, page.toStringCssOnly(), "hyphens:auto;overflow-wrap:break-word;");
}

@test
public func writing_mode_and_text_orientation_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        writing-mode: vertical-rl;
        text-orientation: upright;
    }
    css_equals(env, page.toStringCssOnly(), "writing-mode:vertical-rl;text-orientation:upright;");
}

@test
public func text_emphasis_and_mark_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-emphasis: filled dot #f00;
        mark { background: yellow; color: black; }
    }
    css_equals(env, page.toStringCssOnly(), "text-emphasis:filled dot #f00;mark{background:yellow;color:black;}");
}

@test
public func column_count_and_fill_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        column-count: 3;
        column-fill: balance;
    }
    css_equals(env, page.toStringCssOnly(), "column-count:3;column-fill:balance;");
}

@test
public func scroll_snap_points_and_align_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        scroll-snap-type: x mandatory;
        scroll-snap-align: center start;
    }
    css_equals(env, page.toStringCssOnly(), "scroll-snap-type:x mandatory;scroll-snap-align:center start;");
}

@test
public func resize_observer_and_svg_viewbox_like_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        view-transition-name: hero;
        shape-outside: inset(10px 20px 30px 40px round 10px);
    }
    css_equals(env, page.toStringCssOnly(), "view-transition-name:hero;shape-outside:inset(10px 20px 30px 40px round 10px);");
}

@test
public func background_blend_modes_multilayer_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        background-blend-mode: normal, multiply;
    }
    css_equals(env, page.toStringCssOnly(), "background-blend-mode:normal,multiply;");
}

@test
public func paint_order_and_mix_blend_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        paint-order: stroke fill markers;
        mix-blend-mode: screen;
    }
    css_equals(env, page.toStringCssOnly(), "paint-order:stroke fill markers;mix-blend-mode:screen;");
}

@test
public func filter_function_with_comma_and_space_variants_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        filter: drop-shadow(2px 4px 6px #222), blur(3px);
    }
    css_equals(env, page.toStringCssOnly(), "filter:drop-shadow(2px 4px 6px #222),blur(3px);");
}

@test
public func vendor_prefixed_properties_and_multiple_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        -webkit-text-stroke: 1px #000;
        -moz-column-gap: 12px;
    }
    css_equals(env, page.toStringCssOnly(), "-webkit-text-stroke:1px #000;-moz-column-gap:12px;");
}

@test
public func media_query_simulation_in_css_block_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        @media (min-width: 600px) { .col { width: calc(50% - 10px); } }
    }
    css_equals(env, page.toStringCssOnly(), "@media (min-width: 600px){.col{width:calc(50% - 10px);}}");
}

@test
public func logical_properties_and_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        margin-inline: 1rem 2rem;
        padding-block: 8px 16px;
    }
    css_equals(env, page.toStringCssOnly(), "margin-inline:1rem 2rem;padding-block:8px 16px;");
}

@test
public func inset_shorthand_and_absolute_positioning_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        position: absolute;
        inset: 10px 20px 30px 40px;
    }
    css_equals(env, page.toStringCssOnly(), "position:absolute;inset:10px 20px 30px 40px;");
}

@test
public func clip_and_rect_and_old_syntax_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        clip: rect(0px, 100px, 200px, 0px);
    }
    css_equals(env, page.toStringCssOnly(), "clip:rect(0px,100px,200px,0px);");
}

@test
public func calc_with_nested_operations_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        width: calc(100% - (2 * 1rem) - 20px);
    }
    css_equals(env, page.toStringCssOnly(), "width:calc(100% - (2 * 1rem) - 20px);");
}

@test
public func multi_value_custom_property_and_fallback_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        --card-padding: 1rem 2rem;
        padding: var(--card-padding, 8px 12px);
    }
    css_equals(env, page.toStringCssOnly(), "--card-padding:1rem 2rem;padding:var(--card-padding, 8px 12px);");
}

@test
public func svg_stroke_and_fill_properties_with_multiple_values_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        stroke-dasharray: 5 3 2;
        stroke-linejoin: round;
    }
    css_equals(env, page.toStringCssOnly(), "stroke-dasharray:5 3 2;stroke-linejoin:round;");
}

@test
public func text_overflow_and_multi_part_clamping_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        text-overflow: ellipsis;
        -webkit-line-clamp: 3;
        display: -webkit-box;
        -webkit-box-orient: vertical;
    }
    css_equals(env, page.toStringCssOnly(), "text-overflow:ellipsis;-webkit-line-clamp:3;display:-webkit-box;-webkit-box-orient:vertical;");
}

@test
public func complex_border_image_slice_and_repeat_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-image: url(\"slice.png\") 30 30 30 30 round stretch;
    }
    css_equals(env, page.toStringCssOnly(), "border-image:url(\"slice.png\") 30 30 30 30 round stretch;");
}

@test
public func color_function_hsl_and_hsla_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color: hsl(120 50% 50%);
        background-color: hsla(240, 100%, 50%, 0.5);
    }
    css_equals(env, page.toStringCssOnly(), "color:hsl(120 50% 50%);background-color:hsla(240,100%,50%,0.5);");
}

@test
public func rgb_space_normalization_and_alpha_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        color: rgb(255, 128, 64);
        background: rgba(0,128,255,0.3);
    }
    css_equals(env, page.toStringCssOnly(), "color:rgb(255 128 64);background:rgba(0,128,255,0.3);");
}

@test
public func complex_cursor_and_hotspot_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        cursor: url(\"cursor.png\") 4 12, pointer;
    }
    css_equals(env, page.toStringCssOnly(), "cursor:url(\"cursor.png\") 4 12,pointer;");
}

@test
public func multi_value_border_radius_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        border-radius: 10px 20px 30px 40px / 5px 6px 7px 8px;
    }
    css_equals(env, page.toStringCssOnly(), "border-radius:10px 20px 30px 40 / 5px 6px 7px 8px;");
}

@test
public func complex_calc_and_clamp_and_min_max_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        width: clamp(300px, calc(50% + 2rem), 800px);
        height: min(50vh, 600px);
    }
    css_equals(env, page.toStringCssOnly(), "width:clamp(300px, calc(50% + 2rem), 800px);height:min(50vh,600px);");
}

@test
public func animation_and_transition_combined_properties_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        transition: transform 150ms cubic-bezier(0.4,0,0.2,1);
        animation: spin 1s linear infinite;
    }
    css_equals(env, page.toStringCssOnly(), "transition:transform 150ms cubic-bezier(0.4,0,0.2,1);animation:spin 1s linear infinite;");
}

@test
public func multiple_font_variation_settings_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        font-variation-settings: \"wght\" 700, \"slnt\" -10;
    }
    css_equals(env, page.toStringCssOnly(), "font-variation-settings:\"wght\" 700,\"slnt\" -10;");
}

@test
public func backdrop_filter_and_blend_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        backdrop-filter: blur(6px) brightness(0.9);
    }
    css_equals(env, page.toStringCssOnly(), "backdrop-filter:blur(6px) brightness(0.9);");
}

@test
public func scrollbar_color_and_width_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        scrollbar-color: #888 #222;
        scrollbar-width: thin;
    }
    css_equals(env, page.toStringCssOnly(), "scrollbar-color:#888 #222;scrollbar-width:thin;");
}

@test
public func safe_area_inset_and_env_vars_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        padding-top: env(safe-area-inset-top, 20px);
    }
    css_equals(env, page.toStringCssOnly(), "padding-top:env(safe-area-inset-top, 20px);");
}
**/