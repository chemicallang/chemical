using namespace std;
using namespace md;

// ===== INTEGRATION TESTS =====
// These tests replicate the exact scenarios from the original problem

@test
func test_integration_original_problem_markdown(env : &mut TestEnv) {
    // Test the exact markdown from the original problem
    var input = std::string_view("""# Welcome to the Premium Showcase 🚀

Chemical Markdown isn't just about text; it's about **experience**. This showcase demonstrates the full power of our compiler and its ability to render beautiful, complex documents with zero runtime overhead.

## ✨ Core Philosophy

1.  **Lightning Fast**: Compiled at build-time for instant loading.
2.  **Universal Themes**: Switch styles without touching a single line of logic.
3.  **Extensible Syntax**: Support for advanced features like task lists and abbreviations.

___

### 🎭 Typography & Expression

We believe that great content deserves great fonts.
You can use **Bold**, *Italic*, or even ~~Strikethrough~~ to make your point.

Need more precision?
- **Marked text** for attention ==like this==
- **Inserted** content ++new stuff++
- **Chemical Formulas**: H~2~O, E=mc^2^
- **Abbreviations**: *[HTML]: HyperText Markup Language

> "Design is not just what it looks like and feels like. Design is how it works."
> — Steve Jobs

---

### 💻 Code & Technical Writing

```typescript
// Seamlessly integrated syntax highlighting
function helloChemical() {
  const message = "Hello, Modern World!";
  console.log(message);
  
  return {
    status: "Awesome",
    performance: 100
  };
}
```

You can also use `inline code` for quick references like `const x = 10`.

---

### 📊 Structured Data

| Category | Feature | Support | Performance |
| :--- | :--- | :--- | :--- |
| **Parsing** | Native Tables | Full | Ultra-fast |
| **Logic** | Dynamic Values | Built-in | Optimized |
| **Visuals** | Glassmorphism | Custom | GPU-Ready |

#### 📝 Task Lists & Deep Nesting

- [x] Implement core Markdown parser
- [x] Add theme support
- [ ] Implement live previewer
    - [x] Web interface
    - [ ] Desktop app (Electron?)
- [x] Optimize performance

---

### 📣 Component Intelligence (CBI)

::: info
**Information**: This entire page is generated at compile-time into a high-performance string concatenation block.
:::

::: tip
**Pro Tip**: Use the theme switcher on the left to see how different typography affects readability.
:::

::: warning
**Caution**: Make sure your emojis are saved in a UTF-8 environment to avoid mangling!
:::

::: error
**Critical**: Never settle for basic text rendering when you can have a showcase.
:::

### 🔗 References & Footnotes

You can easily add links to [Chemical Website](https://chemical-lang.org) or add footnotes for extra context[^1].

---

### 📅 Definition Lists

Chemical
:   A powerful systems programming language for the modern web.

Markdown
:   A lightweight markup language with plain-text formatting syntax.

[^1]: This is a footnote demonstrating the `md_cbi` extension support.""");
    
    env.info("Starting integration test with original problem markdown");
    
    var arena = md::Arena();
    env.info("Arena created");
    
    var lexer = md::Lexer(input);
    env.info("Lexer created");
    
    var tokens = lexer.lex();
    env.info("Lexing completed");
    
    var root = md::parse(&mut tokens, &mut arena);
    env.info("Parsing completed");
    
    var output = md::render_to_html(root);
    env.info("Rendering completed");
    
    env.info("Output length:");
    var len_str = std::string();
    len_str.append_uinteger(output.size());
    env.info(len_str.data());
    
    // Test key elements that were problematic in the original issue
    
    // 1. Emojis should be preserved
    expect_html_contains(env, output.to_view(), "🚀", "Rocket emoji should be preserved");
    expect_html_contains(env, output.to_view(), "✨", "Sparkle emoji should be preserved");
    expect_html_contains(env, output.to_view(), "🎭", "Theater emoji should be preserved");
    expect_html_contains(env, output.to_view(), "💻", "Computer emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📊", "Chart emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📝", "Memo emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📣", "Megaphone emoji should be preserved");
    expect_html_contains(env, output.to_view(), "🔗", "Link emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📅", "Calendar emoji should be preserved");
    
    // 2. Headers should have correct CSS classes
    expect_html_contains(env, output.to_view(), "md-hg md-h1", "H1 should have correct CSS classes");
    expect_html_contains(env, output.to_view(), "md-hg md-h2", "H2 should have correct CSS classes");
    expect_html_contains(env, output.to_view(), "md-hg md-h3", "H3 should have correct CSS classes");
    expect_html_contains(env, output.to_view(), "md-hg md-h4", "H4 should have correct CSS classes");
    
    // 3. Inline formatting should work
    expect_html_contains(env, output.to_view(), "md-bold", "Bold should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-italic", "Italic should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-del", "Strikethrough should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-mark", "Mark should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-ins", "Insert should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-sub", "Subscript should have correct CSS class");
    expect_html_contains(env, output.to_view(), "md-sup", "Superscript should have correct CSS class");
    
    // 4. Code blocks should work
    expect_html_contains(env, output.to_view(), "md-pre", "Code block should have md-pre class");
    expect_html_contains(env, output.to_view(), "md-code-block", "Code block should have md-code-block class");
    expect_html_contains(env, output.to_view(), "language-typescript", "Code block should have language class");
    expect_html_contains(env, output.to_view(), "md-code", "Inline code should have md-code class");
    
    // 5. Tables should be rendered correctly (this was a major issue)
    expect_html_contains(env, output.to_view(), "md-table", "Table should have md-table class");
    expect_html_contains(env, output.to_view(), "md-thead", "Table head should have md-thead class");
    expect_html_contains(env, output.to_view(), "md-tbody", "Table body should have md-tbody class");
    expect_html_contains(env, output.to_view(), "md-tr", "Table rows should have md-tr class");
    expect_html_contains(env, output.to_view(), "md-th", "Table headers should have md-th class");
    expect_html_contains(env, output.to_view(), "md-td", "Table cells should have md-td class");
    
    // 6. Task lists should work
    expect_html_contains(env, output.to_view(), "md-task-checkbox", "Task checkboxes should have correct CSS class");
    expect_html_contains(env, output.to_view(), "disabled checked", "Checked task should have checked attribute");
    expect_html_contains(env, output.to_view(), "disabled/>", "Unchecked task should not have checked attribute");
    
    // 7. Custom containers should work
    expect_html_contains(env, output.to_view(), "md-container md-info", "Info container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-tip", "Tip container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-warning", "Warning container should have correct classes");
    expect_html_contains(env, output.to_view(), "md-container md-error", "Error container should have correct classes");
    
    // 8. Links should work
    expect_html_contains(env, output.to_view(), "md-link", "Link should have md-link class");
    expect_html_contains(env, output.to_view(), "https://chemical-lang.org", "Link URL should be preserved");
    
    // 9. Footnotes should work
    expect_html_contains(env, output.to_view(), "md-footnote-ref", "Footnote reference should have correct class");
    expect_html_contains(env, output.to_view(), "md-footnote-def", "Footnote definition should have correct class");
    expect_html_contains(env, output.to_view(), "md-footnote-id", "Footnote ID should have correct class");
    
    // 10. Definition lists should work
    expect_html_contains(env, output.to_view(), "md-dl", "Definition list should have md-dl class");
    expect_html_contains(env, output.to_view(), "md-dt", "Definition term should have md-dt class");
    expect_html_contains(env, output.to_view(), "md-dd", "Definition data should have md-dd class");
    
    // 11. Blockquotes should work
    expect_html_contains(env, output.to_view(), "md-blockquote", "Blockquote should have md-blockquote class");
    
    // 12. Horizontal rule should work
    expect_html_contains(env, output.to_view(), "md-hr", "Horizontal rule should have md-hr class");
    
    // 13. Lists should work
    expect_html_contains(env, output.to_view(), "md-ol", "Ordered list should have md-ol class");
    expect_html_contains(env, output.to_view(), "md-ul", "Unordered list should have md-ul class");
    expect_html_contains(env, output.to_view(), "md-li", "List items should have md-li class");
}

@test
func test_integration_emoji_encoding_specific(env : &mut TestEnv) {
    // Test specific emoji sequences that were problematic
    var input = std::string_view("🚀 ✨ 🎭 💻 📊 📝 📣 🔗 📅");
    
    env.info("Starting emoji encoding test");
    
    var arena = md::Arena();
    env.info("Arena created");
    
    var lexer = md::Lexer(input);
    env.info("Lexer created");
    
    var tokens = lexer.lex();
    env.info("Lexing completed");
    
    var root = md::parse(&mut tokens, &mut arena);
    env.info("Parsing completed");
    
    var output = md::render_to_html(root);
    env.info("Rendering completed");
    
    env.info("Output length:");
    var len_str = std::string();
    len_str.append_uinteger(output.size());
    env.info(len_str.data());
    
    // All emojis should be preserved exactly
    expect_html_contains(env, output.to_view(), "🚀", "Rocket emoji should be preserved");
    expect_html_contains(env, output.to_view(), "✨", "Sparkle emoji should be preserved");
    expect_html_contains(env, output.to_view(), "🎭", "Theater emoji should be preserved");
    expect_html_contains(env, output.to_view(), "💻", "Computer emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📊", "Chart emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📝", "Memo emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📣", "Megaphone emoji should be preserved");
    expect_html_contains(env, output.to_view(), "🔗", "Link emoji should be preserved");
    expect_html_contains(env, output.to_view(), "📅", "Calendar emoji should be preserved");
    
    // Should NOT contain mangled emoji sequences
    if(output.contains("≡ƒ") || output.contains("∩┐╜")) {
        env.error("Output contains mangled emoji sequences");
    }
}

@test
func test_integration_table_false_positives(env : &mut TestEnv) {
    // Test that regular text is NOT parsed as tables
    var input = std::string_view(
"""This is not a table
It's just regular text
With multiple lines
That might look like columns
But aren't actually tables"""
    );
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should NOT contain table elements
    if(output.contains("md-table") || output.contains("<table")) {
        env.error("Regular text was incorrectly parsed as table");
    }
    
    // Should be rendered as paragraphs
    expect_html_contains(env, output.to_view(), "md-p", "Text should be rendered as paragraphs");
}

@test
func test_integration_actual_pipe_table(env : &mut TestEnv) {
    // Test that actual pipe tables ARE parsed correctly
    var input = std::string_view(
"""| Name | Age | City |
|------|-----|------|
| John | 25  | NYC  |
| Jane | 30  | LA   |"""
    );
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should contain table elements
    expect_html_contains(env, output.to_view(), "md-table", "Pipe table should be parsed as table");
    expect_html_contains(env, output.to_view(), "md-thead", "Table should have thead");
    expect_html_contains(env, output.to_view(), "md-tbody", "Table should have tbody");
    expect_html_contains(env, output.to_view(), "md-th", "Table should have th elements");
    expect_html_contains(env, output.to_view(), "md-td", "Table should have td elements");
    
    // Should contain the actual content
    expect_html_contains(env, output.to_view(), "John", "Table should contain John");
    expect_html_contains(env, output.to_view(), "Jane", "Table should contain Jane");
    expect_html_contains(env, output.to_view(), "NYC", "Table should contain NYC");
    expect_html_contains(env, output.to_view(), "LA", "Table should contain LA");
}
