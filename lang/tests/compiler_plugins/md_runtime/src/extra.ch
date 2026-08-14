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
