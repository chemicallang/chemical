// ===== RENDERER TESTS =====

@test
func test_renderer_css_classes_headers(env : &mut TestEnv) {
    var input = std::string_view("# H1\n## H2\n### H3");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-hg md-h1", "H1 should have correct CSS classes");
    expect_html_contains(env, output.to_view(), "md-hg md-h2", "H2 should have correct CSS classes");
    expect_html_contains(env, output.to_view(), "md-hg md-h3", "H3 should have correct CSS classes");
}

@test
func test_renderer_css_classes_paragraphs(env : &mut TestEnv) {
    var input = std::string_view("This is a paragraph");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-p", "Paragraph should have md-p class");
}

@test
func test_renderer_css_classes_inline_formatting(env : &mut TestEnv) {
    var input = std::string_view("**bold** *italic* ~~strike~~ ==mark== ++insert++ H~2~O E=mc^2^");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-bold", "Bold should have md-bold class");
    expect_html_contains(env, output.to_view(), "md-italic", "Italic should have md-italic class");
    expect_html_contains(env, output.to_view(), "md-del", "Strikethrough should have md-del class");
    expect_html_contains(env, output.to_view(), "md-mark", "Mark should have md-mark class");
    expect_html_contains(env, output.to_view(), "md-ins", "Insert should have md-ins class");
    expect_html_contains(env, output.to_view(), "md-sub", "Subscript should have md-sub class");
    expect_html_contains(env, output.to_view(), "md-sup", "Superscript should have md-sup class");
}

@test
func test_renderer_css_classes_code(env : &mut TestEnv) {
    var input = std::string_view("```typescript\nconsole.log('hello');\n```\n\nInline `code` here");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-pre", "Code block should have md-pre class");
    expect_html_contains(env, output.to_view(), "md-code-block", "Code block should have md-code-block class");
    expect_html_contains(env, output.to_view(), "language-typescript", "Code block should have language class");
    expect_html_contains(env, output.to_view(), "md-code", "Inline code should have md-code class");
}

@test
func test_renderer_css_classes_lists(env : &mut TestEnv) {
    var input = std::string_view("- Unordered\n1. Ordered\n- [x] Task");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-ul", "Unordered list should have md-ul class");
    expect_html_contains(env, output.to_view(), "md-ol", "Ordered list should have md-ol class");
    expect_html_contains(env, output.to_view(), "md-li", "List items should have md-li class");
    expect_html_contains(env, output.to_view(), "md-task-checkbox", "Task checkbox should have md-task-checkbox class");
}

@test
func test_renderer_css_classes_containers(env : &mut TestEnv) {
    var input = std::string_view("""::: info
Info
:::
::: tip
Tip
:::
::: warning
Warning
:::
::: error
Error
:::""");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-container md-info", "Info container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-tip", "Tip container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-warning", "Warning container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-error", "Error container should have correct classes");
}

@test
func test_renderer_css_classes_other_elements(env : &mut TestEnv) {
    var input = std::string_view("""> Quote

---

[link](url)

![alt](img.png)

| A | B |
|---|---|
| C | D |

Term
: Def

Text[^1]
[^1]: Note""");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-blockquote", "Blockquote should have md-blockquote class");
    expect_html_contains(env, output.to_view(), "md-hr", "HR should have md-hr class");
    expect_html_contains(env, output.to_view(), "md-link", "Link should have md-link class");
    expect_html_contains(env, output.to_view(), "md-img", "Image should have md-img class");
    expect_html_contains(env, output.to_view(), "md-table", "Table should have md-table class");
    expect_html_contains(env, output.to_view(), "md-thead", "Table head should have md-thead class");
    expect_html_contains(env, output.to_view(), "md-tbody", "Table body should have md-tbody class");
    expect_html_contains(env, output.to_view(), "md-tr", "Table row should have md-tr class");
    expect_html_contains(env, output.to_view(), "md-th", "Table header should have md-th class");
    expect_html_contains(env, output.to_view(), "md-td", "Table cell should have md-td class");
    expect_html_contains(env, output.to_view(), "md-dl", "Definition list should have md-dl class");
    expect_html_contains(env, output.to_view(), "md-dt", "Definition term should have md-dt class");
    expect_html_contains(env, output.to_view(), "md-dd", "Definition data should have md-dd class");
    expect_html_contains(env, output.to_view(), "md-footnote-ref", "Footnote ref should have md-footnote-ref class");
    expect_html_contains(env, output.to_view(), "md-footnote-def", "Footnote def should have md-footnote-def class");
    expect_html_contains(env, output.to_view(), "md-footnote-id", "Footnote id should have md-footnote-id class");
}

@test
func test_renderer_html_escaping(env : &mut TestEnv) {
    var input = std::string_view("Text with <script>alert('xss')</script> & \"quotes\"");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "&lt;script&gt;", "Script tags should be escaped");
    expect_html_contains(env, output.to_view(), "&amp;", "Ampersand should be escaped");
    expect_html_contains(env, output.to_view(), "&quot;", "Quotes should be escaped");
}

@test
func test_renderer_utf8_emoji_handling(env : &mut TestEnv) {
    var input = std::string_view("Hello 🚀 World ✨ Test 🎭");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Check that emojis are preserved (not mangled)
    expect_html_contains(env, output.to_view(), "🚀", "Rocket emoji should be preserved");
    expect_html_contains(env, output.to_view(), "✨", "Sparkle emoji should be preserved");
    expect_html_contains(env, output.to_view(), "🎭", "Theater emoji should be preserved");
}

@test
func test_renderer_nested_formatting(env : &mut TestEnv) {
    var input = std::string_view("This is **bold and *italic* text**");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "<strong class=\"md-bold\">bold and <em class=\"md-italic\">italic</em> text</strong>", "Nested formatting should work correctly");
}

@test
func test_renderer_complex_document(env : &mut TestEnv) {
    var input = std::string_view("""# Document Title

This is a **paragraph** with *formatting*.

## Code Example

```javascript
function test() {
    return true;
}
```

## Lists

- [x] Completed task
- [ ] Pending task

::: info
This is an info box
:::""");
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Verify all major elements are present with correct classes
    expect_html_contains(env, output.to_view(), "md-hg md-h1", "H1 should be present");
    expect_html_contains(env, output.to_view(), "md-hg md-h2", "H2 should be present");
    expect_html_contains(env, output.to_view(), "md-p", "Paragraphs should be present");
    expect_html_contains(env, output.to_view(), "md-bold", "Bold should be present");
    expect_html_contains(env, output.to_view(), "md-italic", "Italic should be present");
    expect_html_contains(env, output.to_view(), "md-pre", "Code block should be present");
    expect_html_contains(env, output.to_view(), "language-javascript", "Language class should be present");
    expect_html_contains(env, output.to_view(), "md-ul", "List should be present");
    expect_html_contains(env, output.to_view(), "md-task-checkbox", "Task checkbox should be present");
    expect_html_contains(env, output.to_view(), "md-container md-info", "Info container should be present");
}
