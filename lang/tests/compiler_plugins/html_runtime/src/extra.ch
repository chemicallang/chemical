@test
func test_html_anchor_with_attrs(env : &mut TestEnv) {
    var input = std::string_view("<a href=\"https://x.com\" target=\"_blank\">link</a>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_nested_spans(env : &mut TestEnv) {
    var input = std::string_view("<div><span>a</span><span>b</span></div>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_input_void_no_slash(env : &mut TestEnv) {
    var input = std::string_view("<input type=\"text\" name=\"q\">")
    var out = html::parse_html(input)
    var expected = std::string_view("<input type=\"text\" name=\"q\"/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_hr_void(env : &mut TestEnv) {
    var input = std::string_view("<hr>")
    var out = html::parse_html(input)
    var expected = std::string_view("<hr/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_multiple_roots(env : &mut TestEnv) {
    var input = std::string_view("<p>a</p><p>b</p>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_plain_text(env : &mut TestEnv) {
    var input = std::string_view("plain text")
    test_html_roundtrip(env, &input)
}

@test
func test_html_boolean_attribute(env : &mut TestEnv) {
    var input = std::string_view("<input disabled>")
    var out = html::parse_html(input)
    var expected = std::string_view("<input disabled/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_entity_preserved(env : &mut TestEnv) {
    var input = std::string_view("<span>a &amp; b</span>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_table(env : &mut TestEnv) {
    var input = std::string_view("<table><tr><td>1</td></tr></table>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_br_inline_void(env : &mut TestEnv) {
    var input = std::string_view("<div>a<br>b</div>")
    var out = html::parse_html(input)
    var expected = std::string_view("<div>a<br/>b</div>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_style_attribute(env : &mut TestEnv) {
    var input = std::string_view("<p style=\"color: red\">x</p>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_data_attribute(env : &mut TestEnv) {
    var input = std::string_view("<div data-id=\"42\">x</div>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_select_options(env : &mut TestEnv) {
    var input = std::string_view("<select><option>a</option><option>b</option></select>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_entity_lt_gt(env : &mut TestEnv) {
    var input = std::string_view("<span>&lt;tag&gt;</span>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_multiline_text(env : &mut TestEnv) {
    var input = std::string_view("<p>line1\nline2</p>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_raw_amp_in_attr(env : &mut TestEnv) {
    var input = std::string_view("<a href=\"a?b=c&d=e\">x</a>")
    test_html_roundtrip(env, &input)
}

// ─── Tests exercising shared converter code paths ───────────────────────────

@test
func test_html_entity_all_major(env : &mut TestEnv) {
    // Exercises html_escape_append for all major HTML entities
    var input = std::string_view("<span>&amp; &lt; &gt; &quot;</span>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_text_with_angle_brackets(env : &mut TestEnv) {
    // Exercises html_escape_append: entities in text content roundtrip correctly
    var input = std::string_view("<p>if (a &lt; b &amp;&amp; c &gt; d)</p>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_deeply_nested(env : &mut TestEnv) {
    // Exercises the converter's recursive child traversal
    var input = std::string_view("<div><ul><li><a href=\"x\">link</a></li></ul></div>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_multiple_attributes(env : &mut TestEnv) {
    // Exercises the converter's attribute iteration
    var input = std::string_view("<img src=\"a.png\" alt=\"pic\" width=\"100\" height=\"200\" />")
    var out = html::parse_html(input)
    var expected = std::string_view("<img src=\"a.png\" alt=\"pic\" width=\"100\" height=\"200\"/>")
    html_view_equals(env, out.to_view(), &expected)
}

@test
func test_html_comment_in_div(env : &mut TestEnv) {
    // Exercises the converter's comment child handling
    var input = std::string_view("<div><!-- inner --><span>x</span></div>")
    test_html_roundtrip(env, &input)
}

@test
func test_html_self_closing_nested(env : &mut TestEnv) {
    // Exercises the converter's self-closing element handling in nested contexts
    var input = std::string_view("<form><input type=\"text\"/><br/><hr/></form>")
    var out = html::parse_html(input)
    var expected = std::string_view("<form><input type=\"text\"/><br/><hr/></form>")
    html_view_equals(env, out.to_view(), &expected)
}
