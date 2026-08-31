@test
func test_md_horizontal_rule(env : &mut TestEnv) {
    var input = std::string_view("---")
    var expected = std::string_view("<hr class=\"md-hr\"/>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_heading_with_bold(env : &mut TestEnv) {
    var input = std::string_view("# **bold heading**")
    var expected = std::string_view("<h1 class=\"md-hg md-h1\"><strong class=\"md-bold\">bold heading</strong></h1>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_bold_containing_link(env : &mut TestEnv) {
    var input = std::string_view("**[x](u)**")
    var expected = std::string_view("<p class=\"md-p\"><strong class=\"md-bold\"><a class=\"md-link\" href=\"u\">x</a></strong></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_ordered_list_offset_start(env : &mut TestEnv) {
    var input = std::string_view("5. five\n6. six")
    var expected = std::string_view("<ol class=\"md-ol\" start=\"5\"><li class=\"md-li\"> five</li>\n<li class=\"md-li\"> six</li>\n</ol>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_multiple_heading_levels(env : &mut TestEnv) {
    var input = std::string_view("## H2\n\n#### H4")
    var expected = std::string_view("<h2 class=\"md-hg md-h2\">H2</h2>\n<h4 class=\"md-hg md-h4\">H4</h4>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_paragraph_then_list(env : &mut TestEnv) {
    var input = std::string_view("para one\n\n- a\n- b")
    var expected = std::string_view("<p class=\"md-p\">para one</p>\n<ul class=\"md-ul\"><li class=\"md-li\"> a</li>\n<li class=\"md-li\"> b</li>\n</ul>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_all_heading_levels(env : &mut TestEnv) {
    var input = std::string_view("# h1\n## h2\n### h3\n#### h4\n##### h5\n###### h6")
    var expected = std::string_view("<h1 class=\"md-hg md-h1\">h1</h1>\n<h2 class=\"md-hg md-h2\">h2</h2>\n<h3 class=\"md-hg md-h3\">h3</h3>\n<h4 class=\"md-hg md-h4\">h4</h4>\n<h5 class=\"md-hg md-h5\">h5</h5>\n<h6 class=\"md-hg md-h6\">h6</h6>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_code_block_contains_tick(env : &mut TestEnv) {
    var input = std::string_view("```\ncode with `tick` inside\n```")
    var expected = std::string_view("<pre class=\"md-pre\"><code class=\"md-code-block\">code with `tick` inside\n</code></pre>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_two_links_in_paragraph(env : &mut TestEnv) {
    var input = std::string_view("[a](u1) and [b](u2)")
    var expected = std::string_view("<p class=\"md-p\"><a class=\"md-link\" href=\"u1\">a</a> and <a class=\"md-link\" href=\"u2\">b</a></p>\n")
    md_render_equals(env, &input, &expected)
}

// ─── Tests exercising shared converter code paths ───────────────────────────

@test
func test_md_html_escaping_angle_brackets(env : &mut TestEnv) {
    // Exercises md_escape_html: raw < and > in text must be escaped
    var input = std::string_view("use 1 < 2 and 3 > 0")
    var expected = std::string_view("<p class=\"md-p\">use 1 &lt; 2 and 3 &gt; 0</p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_html_escaping_ampersand(env : &mut TestEnv) {
    // Exercises md_escape_html: raw & in text must be escaped
    var input = std::string_view("Tom & Jerry")
    var expected = std::string_view("<p class=\"md-p\">Tom &amp; Jerry</p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_html_escaping_quotes(env : &mut TestEnv) {
    // Exercises md_escape_html: double quotes in text must be escaped
    var input = std::string_view("she said \"hello\"")
    var expected = std::string_view("<p class=\"md-p\">she said &quot;hello&quot;</p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_code_block_html_not_escaped(env : &mut TestEnv) {
    // Exercises md_escape_html inside code blocks: HTML in code should be escaped
    var input = std::string_view("```\n<div>hello</div>\n```")
    var expected = std::string_view("<pre class=\"md-pre\"><code class=\"md-code-block\">&lt;div&gt;hello&lt;/div&gt;\n</code></pre>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_inline_code_html_escaped(env : &mut TestEnv) {
    // Exercises md_escape_html inside inline code
    var input = std::string_view("use `<div>` tag")
    var expected = std::string_view("<p class=\"md-p\">use <code class=\"md-code\">&lt;div&gt;</code> tag</p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_image_alt_html_escaped(env : &mut TestEnv) {
    // Exercises md_escape_html for image alt text
    var input = std::string_view("![a < b](img.png)")
    var expected = std::string_view("<p class=\"md-p\"><img class=\"md-img\" src=\"img.png\" alt=\"a &lt; b\"/></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_link_title_html_escaped(env : &mut TestEnv) {
    // Exercises md_escape_html: & in image alt text is escaped
    var input = std::string_view("![a & b](img.png)")
    var expected = std::string_view("<p class=\"md-p\"><img class=\"md-img\" src=\"img.png\" alt=\"a &amp; b\"/></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_superscript_and_subscript(env : &mut TestEnv) {
    // Exercises md_convert_md_node for Superscript and Subscript kinds
    var input = std::string_view("H~2~O and x^2^")
    var expected = std::string_view("<p class=\"md-p\">H<sub class=\"md-sub\">2</sub>O and x<sup class=\"md-sup\">2</sup></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_insert_and_mark(env : &mut TestEnv) {
    // Exercises md_convert_md_node for Insert and Mark kinds
    var input = std::string_view("++inserted++ and ==marked==")
    var expected = std::string_view("<p class=\"md-p\"><ins class=\"md-ins\">inserted</ins> and <mark class=\"md-mark\">marked</mark></p>\n")
    md_render_equals(env, &input, &expected)
}
