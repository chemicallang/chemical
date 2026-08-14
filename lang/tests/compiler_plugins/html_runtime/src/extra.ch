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
