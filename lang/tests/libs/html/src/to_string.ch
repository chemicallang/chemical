@test
public func text_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        Normal Text}
    string_equals(env, page.toStringHtmlOnly(), "Normal Text");
}

@test
public func multiple_elements_in_root_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        Normal Text<div>is</div>Here}
    string_equals(env, page.toStringHtmlOnly(), "Normal Text<div>is</div>Here");
}

@test
public func element_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>Normal Text</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>Normal Text</div>");
}

@test
public func nested_element_in_root_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div>Normal Text</div>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><div>Normal Text</div></div>");
}

@test
public func attribute_on_element_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div id="something">Normal Text</div>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><div id=\"something\">Normal Text</div></div>");
}

@test
public func multiple_children_in_root_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <div>First Child</div>
            <div>Second Child</div>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><div>First Child</div><div>Second Child</div></div>");
}

@test
public func self_closing_tag_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text" />
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\"/></div>");
}

@test
public func optional_fwd_slash_in_self_closing(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text">
            <br>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\"/><br/></div>");
}

@test
public func attribute_without_value_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <input type="text" disabled/>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><input type=\"text\" disabled/></div>");
}

@test
public func can_handle_comments(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>
            <!-- This is a comment -->
            <div>Normal Text</div>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div><div>Normal Text</div></div>");
}

@test
public func chemical_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var text = "Normal"
    #html {
        <div>{text}Text</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>NormalText</div>");
}

@test
public func chemical_value_in_attribute_works(env : &mut TestEnv) {
    var page = HtmlPage()
    var idValue = "something"
    #html {
        <div id={idValue}>Normal Text</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div id=\"something\">Normal Text</div>");
}

@test
public func chem_string_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{"Normal"}Text</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>NormalText</div>");
}

func ret_str_for_html(cond : bool) : *char {
    if(cond) {
        return "abc"
    } else {
        return "xyz"
    }
}

@test
public func chem_string_value_call_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>ABC={ret_str_for_html(true)},XYZ={ret_str_for_html(false)}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>ABC=abc,XYZ=xyz</div>");
}

@test
public func chem_char_value_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>Normal{' '}Text</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>Normal Text</div>");
}

func ret_char_for_html(cond : bool) : char {
    if(cond) {
        return 'a'
    } else {
        return 'b'
    }
}

@test
public func chem_char_value_call_in_text_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>A={ret_char_for_html(true)},B={ret_char_for_html(false)}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>A=a,B=b</div>");
}

func my_component(page : &mut HtmlPage) {
    #html {
        <div>From Component</div>
    }
}

@test
public func call_void_returning_components_in_html(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div id="p">
            {my_component(page)}
            <div id="c">C</div>
        </div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div id=\"p\"><div>From Component</div><div id=\"c\">C</div></div>");
}

@test
public func integer_values_in_html_automatically_converted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{128}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>128</div>");
}

@test
public func integer_values_in_html_automatically_converted2(env : &mut TestEnv) {
    var page = HtmlPage()
    var value : int = 128
    #html {
        <div>{value}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>128</div>");
}

@test
public func uinteger_values_in_html_automatically_converted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{128u}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>128</div>");
}

@test
public func uinteger_values_in_html_automatically_converted2(env : &mut TestEnv) {
    var page = HtmlPage()
    var value : uint = 128
    #html {
        <div>{value}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>128</div>");
}

@test
public func floating_values_in_html_automatically_converted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{876.123f}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>876.123</div>");
}

@test
public func floating_values_in_html_automatically_converted2(env : &mut TestEnv) {
    var page = HtmlPage()
    var value = 876.123f
    #html {
        <div>{value}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>876.123</div>");
}

@test
public func double_values_in_html_automatically_converted(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <div>{876.123}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>876.123</div>");
}

@test
public func double_values_in_html_automatically_converted2(env : &mut TestEnv) {
    var page = HtmlPage()
    var value = 876.123
    #html {
        <div>{value}</div>
    }
    string_equals(env, page.toStringHtmlOnly(), "<div>876.123</div>");
}