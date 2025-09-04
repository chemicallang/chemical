public func html_equals(env : &mut TestEnv, str : &std::string, view : &std::string_view) {

    if(str.equals_view(view)) {
        return;
    }

    env.error("equals failure");

    var expected = std::string("expected:\"");
    expected.append_view(view)
    expected.append('"');
    env.info(expected.data())

    var got = std::string("got:\"");
    got.append_string(str)
    got.append('"');
    env.info(got.data())

}

@test
func text_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        Normal Text}
    html_equals(env, page.toStringHtmlOnly(), "Normal Text");
}

@test
func multiple_elements_in_root_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        Normal Text<div>is</div>Here}
    html_equals(env, page.toStringHtmlOnly(), "Normal Text<div>is</div>Here");
}

@test
func element_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>Normal Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>Normal Text</div>");
}

@test
func nested_element_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div>Normal Text</div>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><div>Normal Text</div></div>");
}

@test
func attribute_on_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div id="something">Normal Text</div>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><div id=\"something\">Normal Text</div></div>");
}

@test
func multiple_children_in_root_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div>First Child</div>
            <div>Second Child</div>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><div>First Child</div><div>Second Child</div></div>");
}

@test
func self_closing_tag_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text" />
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\"/></div>");
}

@test
func optional_fwd_slash_in_self_closing(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text">
            <br>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\"/><br/></div>");
}

@test
func attribute_without_value_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text" disabled/>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\" disabled/></div>");
}

@test
func can_handle_comments(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <!-- This is a comment -->
            <div>Normal Text</div>
        </div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div><div>Normal Text</div></div>");
}

@test
func chemical_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var text = "Normal"
    #html {
        <div>{text}Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>NormalText</div>");
}

@test
func chemical_value_in_attribute_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var idValue = "something"
    #html {
        <div id={idValue}>Normal Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div id=\"something\">Normal Text</div>");
}

@test
func chem_string_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{"Normal"}Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>NormalText</div>");
}

func ret_str_for_html(cond : bool) : *char {
    if(cond) {
        return "abc"
    } else {
        return "xyz"
    }
}

@test
func chem_string_value_call_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>ABC={ret_str_for_html(true)},XYZ={ret_str_for_html(false)}</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>ABC=abc,XYZ=xyz</div>");
}

@test
func chem_char_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>Normal{' '}Text</div>
    }
    html_equals(env, page.toStringHtmlOnly(), "<div>Normal Text</div>");
}

// func ret_char_for_html(cond : bool) : char {
//     if(cond) {
//         return 'a'
//     } else {
//         return 'b'
//     }
// }
//
// @test
// func chem_char_value_call_in_text_works(env : &mut TestEnv) {
//     var page = HtmlPage()
//     #html {
//         <div>A={ret_char_for_html(true)},B={ret_char_for_html(false)}</div>
//     }
//     html_equals(env, page.toStringHtmlOnly(), "<div>A=a,B=b</div>");
// }