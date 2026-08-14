@test
public func test_css_comments_dropped(env : &mut TestEnv) {
    const view = std::string_view("/* comment */ body { color: red }")
    var out = css::parse_css(view)
    var expected = std::string_view("body { color:red; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_multiple_selectors(env : &mut TestEnv) {
    const view = std::string_view("h1, h2 { color: red }")
    var out = css::parse_css(view)
    var expected = std::string_view("h1, h2 { color:red; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_pseudo_element_quotes(env : &mut TestEnv) {
    const view = std::string_view("p::before { content: \"x\" }")
    var out = css::parse_css(view)
    var expected = std::string_view("p::before { content:'x'; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_important(env : &mut TestEnv) {
    const view = std::string_view("p { color: red !important }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { color:red !important; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_calc_value(env : &mut TestEnv) {
    const view = std::string_view(".c { width: calc(100% - 10px) }")
    var out = css::parse_css(view)
    var expected = std::string_view(".c { width:calc(100% - 10px); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_short_hex(env : &mut TestEnv) {
    const view = std::string_view("p { color: #fff }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { color:#fff; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_multiple_declarations(env : &mut TestEnv) {
    const view = std::string_view("p { margin: 0; padding: 0; }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { margin:0;padding:0; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_media_condition(env : &mut TestEnv) {
    const view = std::string_view("@media (min-width: 700px) { .x { display: none } }")
    var out = css::parse_css(view)
    var expected = std::string_view("@media (min-width: 700px) { .x { display:none; } }")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_pseudo_class(env : &mut TestEnv) {
    const view = std::string_view("a:hover { text-decoration: underline }")
    var out = css::parse_css(view)
    var expected = std::string_view("a:hover { text-decoration:underline; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_empty_rule_dropped(env : &mut TestEnv) {
    const view = std::string_view("div {}")
    var out = css::parse_css(view)
    var expected = std::string_view("")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_import_dropped(env : &mut TestEnv) {
    const view = std::string_view("@import url(\"a.css\");")
    var out = css::parse_css(view)
    var expected = std::string_view("")
    css_view_equals(env, out.to_view(), &expected)
}
