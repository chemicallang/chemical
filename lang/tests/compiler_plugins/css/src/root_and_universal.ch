@test
public func root_pseudo_class_with_custom_props_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        :root {
            --bg-color: #f8fafc;
            --text-color: #0f172a;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(":root { --bg-color:#f8fafc;--text-color:#0f172a; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func universal_selector_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        * {
            box-sizing: border-box;
            margin: 0;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view("* { box-sizing:border-box;margin:0; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func body_with_var_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        body {
            font-family: var(--wiqis-font);
            background: var(--wiqis-bg);
            color: var(--wiqis-text);
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view("body { font-family:var(--wiqis-font);background:var(--wiqis-bg);color:var(--wiqis-text); }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func class_with_custom_properties_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .wiqis-dark {
            --wiqis-bg: #0f172a;
            --wiqis-text: #f1f5f9;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".wiqis-dark { --wiqis-bg:#0f172a;--wiqis-text:#f1f5f9; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func tag_with_hover_pseudo_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        a:hover {
            text-decoration: underline;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view("a:hover { text-decoration:underline; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func descendant_combinator_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .wiqis-header h1 {
            font-size: 20px;
            font-weight: 700;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".wiqis-header h1 { font-size:20px;font-weight:700; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func rgba_in_shadow_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .shadow {
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.06);
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".shadow { box-shadow:0 1px 3px rgba(0 0 0 / 0.06); }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func minified_root_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        :root{--primary:#6366f1;--accent:#f59e0b;}
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    // The converter normalizes spacing, adding space around {
    expected.append_view(":root { --primary:#6366f1;--accent:#f59e0b; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func minified_universal_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        *{box-sizing:border-box;}
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view("* { box-sizing:border-box; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func combined_with_root_decl_and_selector_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        display: flex;
        .card {
            gap: 8px;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{display:flex;}");
    expected.append_view(".card { gap:8px; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func comma_separated_selectors_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .wiqis-results .score, .wiqis-results .score-detail {
            font-weight: 700;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".wiqis-results .score,.wiqis-results .score-detail { font-weight:700; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func z_index_integer_value_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .header {
            z-index: 100;
            position: sticky;
        }
    }
    css_equals(env, page.toStringCssOnly(), "z-index:100;position:sticky;");
}

@test
public func transition_all_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .btn {
            transition: all 0.15s;
        }
    }
    css_equals(env, page.toStringCssOnly(), "transition:all 0.15s;");
}

@test
public func accent_color_with_var_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .option input {
            accent-color: var(--wiqis-primary);
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".option input { accent-color:var(--wiqis-primary); }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func anchor_tag_styling_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        a {
            color: var(--wiqis-primary);
            text-decoration: none;
        }
    }
    css_equals(env, page.toStringCssOnly(), "color:var(--wiqis-primary);text-decoration:none;");
}

@test
public func button_outline_hover_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .btn-outline:hover {
            background: var(--surface-2);
            border-color: var(--border);
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(".btn-outline:hover { background:var(--surface-2);border-color:var(--border); }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func custom_property_with_quoted_font_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        :root {
            --wiqis-font: "Inter", -apple-system, sans-serif;
        }
    }
    var got = page.toStringCssOnly();
    var expected = std::string();
    var classView = std::string_view(got.data(), 8)
    expected.append_view(&classView)
    expected.append_view("{}");
    expected.append_view(":root { --wiqis-font:\\\"Inter\\\",-apple-system,sans-serif; }");
    compl_css_equals(env, &got, expected.to_view());
}

@test
public func header_multi_decl_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .wiqis-header {
            background: var(--wiqis-surface);
            border-bottom: 1px solid var(--wiqis-border);
            padding: 16px 24px;
            display: flex;
            align-items: center;
            justify-content: space-between;
        }
    }
    css_equals(env, page.toStringCssOnly(), "background:var(--wiqis-surface);border-bottom:1px solid var(--wiqis-border);padding:16px 24px;display:flex;align-items:center;justify-content:space-between;");
}
