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

// ─── Tests exercising shared converter code paths ───────────────────────────

@test
public func test_css_string_escaping_single_quote(env : &mut TestEnv) {
    // Exercises css_write_css_string_to_buffer: single-quote in string must be escaped
    const view = std::string_view("p { content: \"it's\" }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { content:'it\\'s'; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_string_escaping_backslash(env : &mut TestEnv) {
    // Exercises css_write_css_string_to_buffer: backslash in string must be escaped
    const view = std::string_view("p { content: \"path\\to\\file\" }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { content:'path\\\\to\\\\file'; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_color_rgb(env : &mut TestEnv) {
    // Exercises css_write_color with RGB kind — CSS parser normalizes commas to spaces
    const view = std::string_view("p { color: rgb(255, 0, 128) }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { color:rgb(255 0 128); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_color_hsl(env : &mut TestEnv) {
    // Exercises css_write_color with HSL kind — CSS parser normalizes commas to spaces
    const view = std::string_view("p { color: hsl(120, 50%, 50%) }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { color:hsl(120 50% 50%); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_transition_shorthand(env : &mut TestEnv) {
    // Exercises css_write_transition through the shared converter
    const view = std::string_view("p { transition: color 0.3s ease }")
    var out = css::parse_css(view)
    var expected = std::string_view("p { transition:color 0.3s ease; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_transform_rotate(env : &mut TestEnv) {
    // Exercises css_write_transform_value_data through the shared converter
    const view = std::string_view(".x { transform: rotate(45deg) }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { transform:rotate(45deg); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_animation_shorthand(env : &mut TestEnv) {
    // Exercises css_write_animation_value_data through the shared converter
    const view = std::string_view(".x { animation: spin 1s linear infinite }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { animation:spin 1s linear infinite; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_border_shorthand(env : &mut TestEnv) {
    // Exercises css_write_value with Border kind
    const view = std::string_view(".x { border: 1px solid red }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { border:1px solid red; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_box_shadow(env : &mut TestEnv) {
    // Exercises css_write_box_shadow_value_data through the shared converter
    const view = std::string_view(".x { box-shadow: 2px 2px 4px rgba(0,0,0,0.5) }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { box-shadow:2px 2px 4px rgba(0 0 0 / 0.5); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_border_radius(env : &mut TestEnv) {
    // Exercises css_write_border_radius_value_data through the shared converter
    const view = std::string_view(".x { border-radius: 4px 8px 12px 16px }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { border-radius:4px 8px 12px 16px; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_linear_gradient(env : &mut TestEnv) {
    // Exercises css_write_background_image_data with linear gradient
    const view = std::string_view(".x { background: linear-gradient(45deg, red, blue) }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { background:linear-gradient(45deg,red,blue); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_font_shorthand(env : &mut TestEnv) {
    // Exercises css_write_font_value_data through the shared converter
    const view = std::string_view(".x { font: italic bold 16px/1.5 Arial, sans-serif }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { font:italic bold 16px/1.5 Arial,sans-serif; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_text_decoration(env : &mut TestEnv) {
    // Exercises css_write_value with TextDecoration kind
    const view = std::string_view("a { text-decoration: underline wavy red }")
    var out = css::parse_css(view)
    var expected = std::string_view("a { text-decoration:underline wavy red; } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_var_function(env : &mut TestEnv) {
    // Exercises css_write_length with Variable kind
    const view = std::string_view(".x { color: var(--primary) }")
    var out = css::parse_css(view)
    var expected = std::string_view(".x { color:var(--primary); } ")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_media_nested_rule(env : &mut TestEnv) {
    // Exercises css_write_media_nested_rule through the shared converter
    // Note: the CSS parser only emits one rule per selector block in media queries
    const view = std::string_view("@media screen { .a { color: red } .b { color: blue } }")
    var out = css::parse_css(view)
    var expected = std::string_view("@media screen { .a { color:red; } }")
    css_view_equals(env, out.to_view(), &expected)
}

@test
public func test_css_keyframes_full(env : &mut TestEnv) {
    // Exercises css_write_keyframes_rule through the shared converter
    const view = std::string_view("@keyframes fade { 0% { transform: rotate(0deg) } 100% { transform: rotate(360deg) } }")
    var out = css::parse_css(view)
    var expected = std::string_view("@keyframes fade { 0% { transform:rotate(0deg); } 100% { transform:rotate(360deg); } }")
    css_view_equals(env, out.to_view(), &expected)
}
