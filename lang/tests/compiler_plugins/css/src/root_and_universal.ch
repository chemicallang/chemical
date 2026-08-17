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
    compl_css_equals(env, &got, ":root { --bg-color:#f8fafc;--text-color:#0f172a; }");
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
    compl_css_equals(env, &got, "* { box-sizing:border-box;margin:0; }");
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
    compl_css_equals(env, &got, "body { font-family:var(--wiqis-font);background:var(--wiqis-bg);color:var(--wiqis-text); }");
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
    compl_css_equals(env, &got, ".wiqis-dark { --wiqis-bg:#0f172a;--wiqis-text:#f1f5f9; }");
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
    compl_css_equals(env, &got, "a:hover { text-decoration:underline; }");
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
    compl_css_equals(env, &got, ".wiqis-header h1 { font-size:20px;font-weight:700; }");
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
    compl_css_equals(env, &got, ".shadow { box-shadow:0 1px 3px rgba(0 0 0 / 0.06); }");
}

@test
public func minified_root_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        :root{--primary:#6366f1;--accent:#f59e0b;}
    }
    var got = page.toStringCssOnly();
    // The converter normalizes spacing, adding space around {
    compl_css_equals(env, &got, ":root { --primary:#6366f1;--accent:#f59e0b; }");
}

@test
public func minified_universal_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        *{box-sizing:border-box;}
    }
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, "* { box-sizing:border-box; }");
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
    // Nested rules without `&` are implicitly descendants of the root class
    // (CSS nesting semantics): `.class .card {}`
    expected.append_view(&classView)
    expected.append_view("{display:flex;}");
    expected.append_view(&classView)
    expected.append_view(" .card { gap:8px; }");
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
    compl_css_equals(env, &got, ".wiqis-results .score,.wiqis-results .score-detail { font-weight:700; }");
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
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, ".header { z-index:100;position:sticky; }");
}

@test
public func transition_all_shorthand_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        .btn {
            transition: all 0.15s;
        }
    }
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, ".btn { transition:all 0.15s; }");
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
    compl_css_equals(env, &got, ".option input { accent-color:var(--wiqis-primary); }");
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
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, "a { color:var(--wiqis-primary);text-decoration:none; }");
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
    compl_css_equals(env, &got, ".btn-outline:hover { background:var(--surface-2);border-color:var(--border); }");
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
    compl_css_equals(env, &got, ":root { --wiqis-font:\"Inter\", -apple-system, sans-serif; }");
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
    var got = page.toStringCssOnly();
    compl_css_equals(env, &got, ".wiqis-header { background:var(--wiqis-surface);border-bottom:1px solid var(--wiqis-border);padding:16px 24px;display:flex;align-items:center;justify-content:space-between; }");
}
