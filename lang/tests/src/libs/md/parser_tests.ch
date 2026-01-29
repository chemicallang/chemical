using namespace std;
using namespace md;

// ===== PARSER TESTS =====

@test
func test_parser_simple_paragraph(env : &mut TestEnv) {
    var input = std::string_view("Hello world");
    var expected = std::string_view("<p class=\"md-p\">Hello world</p>\n");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_headers(env : &mut TestEnv) {
    var input = std::string_view("# Header 1\n## Header 2\n### Header 3");
    var expected = std::string_view("""<h1 class="md-hg md-h1">Header 1</h1>
<h2 class="md-hg md-h2">Header 2</h2>
<h3 class="md-hg md-h3">Header 3</h3>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_bold_italic(env : &mut TestEnv) {
    var input = std::string_view("This is **bold** and *italic* text");
    var expected = std::string_view("""<p class="md-p">This is <strong class="md-bold">bold</strong> and <em class="md-italic">italic</em> text</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_code_block(env : &mut TestEnv) {
    var input = std::string_view("```typescript\nconsole.log('hello');\n```");
    var expected = std::string_view("""<pre class="md-pre"><code class="md-code-block language-typescript">console.log('hello');
</code></pre>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_inline_code(env : &mut TestEnv) {
    var input = std::string_view("Use `const x = 10` for variables");
    var expected = std::string_view("""<p class="md-p">Use <code class="md-code">const x = 10</code> for variables</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_links(env : &mut TestEnv) {
    var input = std::string_view("[link](https://example.com)");
    var expected = std::string_view("""<p class="md-p"><a class="md-link" href="https://example.com">link</a></p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_images(env : &mut TestEnv) {
    var input = std::string_view("![alt text](https://example.com/image.png)");
    var expected = std::string_view("""<p class="md-p"><img class="md-img" src="https://example.com/image.png" alt="alt text"/></p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_unordered_list(env : &mut TestEnv) {
    var input = std::string_view("- Item 1\n- Item 2\n- Item 3");
    var expected = std::string_view("""<ul class="md-ul">
<li class="md-li">Item 1</li>
<li class="md-li">Item 2</li>
<li class="md-li">Item 3</li>
</ul>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_ordered_list(env : &mut TestEnv) {
    var input = std::string_view("1. First\n2. Second\n3. Third");
    var expected = std::string_view("""<ol class="md-ol">
<li class="md-li">First</li>
<li class="md-li">Second</li>
<li class="md-li">Third</li>
</ol>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_task_lists(env : &mut TestEnv) {
    var input = std::string_view("- [x] Done\n- [ ] Pending");
    var expected = std::string_view("""<ul class="md-ul">
<li class="md-li"><input class="md-task-checkbox" type="checkbox" disabled checked/>Done</li>
<li class="md-li"><input class="md-task-checkbox" type="checkbox" disabled/>Pending</li>
</ul>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_blockquote(env : &mut TestEnv) {
    var input = std::string_view("> This is a quote");
    var expected = std::string_view("""<blockquote class="md-blockquote"><p class="md-p">This is a quote</p>
</blockquote>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_horizontal_rule(env : &mut TestEnv) {
    var input = std::string_view("---");
    var expected = std::string_view("""<hr class="md-hr"/>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_strikethrough(env : &mut TestEnv) {
    var input = std::string_view("~~deleted~~ text");
    var expected = std::string_view("""<p class="md-p"><del class="md-del">deleted</del> text</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_mark(env : &mut TestEnv) {
    var input = std::string_view("==marked== text");
    var expected = std::string_view("""<p class="md-p"><mark class="md-mark">marked</mark> text</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_insert(env : &mut TestEnv) {
    var input = std::string_view("++inserted++ text");
    var expected = std::string_view("""<p class="md-p"><ins class="md-ins">inserted</ins> text</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_subscript(env : &mut TestEnv) {
    var input = std::string_view("H~2~O");
    var expected = std::string_view("""<p class="md-p">H<sub class="md-sub">2</sub>O</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_superscript(env : &mut TestEnv) {
    var input = std::string_view("E=mc^2^");
    var expected = std::string_view("""<p class="md-p">E=mc<sup class="md-sup">2</sup></p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_custom_containers(env : &mut TestEnv) {
    var input = std::string_view("::: info\nThis is info\n:::");
    var expected = std::string_view("""<div class="md-container md-info"><p class="md-p">This is info</p>
</div>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_custom_containers_multiple(env : &mut TestEnv) {
    var input = std::string_view("""::: info
Info content
:::
::: warning
Warning content
:::""");
    var expected = std::string_view("""<div class="md-container md-info"><p class="md-p">Info content</p>
</div>
<div class="md-container md-warning"><p class="md-p">Warning content</p>
</div>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_abbreviations(env : &mut TestEnv) {
    var input = std::string_view("[HTML]: HyperText Markup Language\nHTML is used");
    var expected = std::string_view("""<abbr title=" HyperText Markup Language
">HTML</abbr><p class="md-p">HTML is used</p>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_footnotes(env : &mut TestEnv) {
    var input = std::string_view("Text with footnote[^1]\n\n[^1]: Footnote definition");
    var expected = std::string_view("""<p class="md-p">Text with footnote<sup class="md-footnote-ref" id="fnref:1"><a href="#fn:1">1</a></sup></p>
<div class="md-footnote-def" id="fn:1"><span class="md-footnote-id">1: </span><p class="md-p">Footnote definition</p>
</div>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_definition_lists(env : &mut TestEnv) {
    var input = std::string_view("Term\n: Definition");
    var expected = std::string_view("""<dl class="md-dl"><dt class="md-dt">Term</dt>
<dd class="md-dd">Definition</dd>
</dl>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_pipe_table(env : &mut TestEnv) {
    var input = std::string_view("| Col1 | Col2 |\n|------|------|\n| A | B |");
    var expected = std::string_view("""<table class="md-table">
<thead class="md-thead"><tr class="md-tr">
<th class="md-th">Col1</th>
<th class="md-th">Col2</th>
</tr></thead>
<tbody class="md-tbody"><tr class="md-tr">
<td class="md-td">A</td>
<td class="md-td">B</td>
</tr>
</tbody>
</table>
""");
    test_markdown_roundtrip(env, input, expected);
}

@test
func test_parser_no_false_positive_tables(env : &mut TestEnv) {
    var input = std::string_view("This is not a table\nIt's just regular text");
    var expected = std::string_view("""<p class="md-p">This is not a table</p>
<p class="md-p">It's just regular text</p>
""");
    test_markdown_roundtrip(env, input, expected);
}
