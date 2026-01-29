@test
func test_edge_cases_empty_input(env : &mut TestEnv) {
    var input = std::string_view("");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Empty input should produce empty output
    if(output.size() != 0) {
        env.error("Empty input should produce empty output");
    }
}

@test
func test_edge_cases_only_whitespace(env : &mut TestEnv) {
    var input = std::string_view("   \n  \n   \n");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Whitespace only should produce empty output
    if(output.size() != 0) {
        env.error("Whitespace only should produce empty output");
    }
}

@test
func test_edge_cases_mixed_formatting(env : &mut TestEnv) {
    var input = std::string_view("**bold *italic* ~~strikethrough~~ text**");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-bold", "Nested formatting should work");
    expect_html_contains(env, output.to_view(), "md-italic", "Nested italic should work");
    expect_html_contains(env, output.to_view(), "md-del", "Nested strikethrough should work");
}

@test
func test_edge_cases_unclosed_formatting(env : &mut TestEnv) {
    var input = std::string_view("This has **unclosed bold and *unclosed italic");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should not crash and should produce some output
    if(output.size() == 0) {
        env.error("Unclosed formatting should not crash");
    }
}

@test
func test_edge_cases_special_characters(env : &mut TestEnv) {
    var input = std::string_view("Special chars: < > & \" ' \\ / ` ! @ # $ % ^ & * ( ) _ - + = { } [ ] | ; : ' . , ?");
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should properly escape HTML entities
    expect_html_contains(env, output.to_view(), "&lt;", "Less than should be escaped");
    expect_html_contains(env, output.to_view(), "&gt;", "Greater than should be escaped");
    expect_html_contains(env, output.to_view(), "&amp;", "Ampersand should be escaped");
    expect_html_contains(env, output.to_view(), "&quot;", "Quote should be escaped");
}

@test
func test_performance_large_document(env : &mut TestEnv) {
    // Test with a reasonably large document to check for performance issues
    var input = std::string();
    var i = 0u;
    while(i < 100) {
        input.append_view("# Header ");
        input.append_uinteger(i);
        input.append_view("\n\n");
        input.append_view("This is paragraph ");
        input.append_uinteger(i);
        input.append_view(" with **bold** and *italic* text.\n\n");
        
        var j = 0u;
        while(j < 5) {
            input.append_view("- List item ");
            input.append_uinteger(j);
            input.append_view("\n");
            j++;
        }
        input.append_view("\n");
        i++;
    }
    
    var arena = md::Arena();
    var lexer = md::Lexer(input.to_view());
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should produce substantial output
    if(output.size() < 1000) {
        env.error("Large document should produce substantial output");
    }
    
    // Should contain expected elements
    expect_html_contains(env, output.to_view(), "md-hg md-h1", "Should contain headers");
    expect_html_contains(env, output.to_view(), "md-p", "Should contain paragraphs");
    expect_html_contains(env, output.to_view(), "md-ul", "Should contain lists");
    expect_html_contains(env, output.to_view(), "md-bold", "Should contain bold text");
    expect_html_contains(env, output.to_view(), "md-italic", "Should contain italic text");
}

@test
func test_unicode_various_languages(env : &mut TestEnv) {
    // Test various Unicode characters beyond just emojis
    var input = std::string_view(
"""English: Hello World
Spanish: ¡Hola Mundo!
French: Bonjour le monde
German: Hallo Welt
Russian: Привет мир
Chinese: 你好世界
Japanese: こんにちは世界
Arabic: مرحبا بالعالم
Hebrew: שלום עולם
Math: ∑∏∫∆∇∂
Currency: $€£¥₹
Symbols: ©®™§¶†‡•…‰‹›''–—""");
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    // Should preserve Unicode characters
    expect_html_contains(env, output.to_view(), "¡Hola Mundo!", "Spanish should be preserved");
    expect_html_contains(env, output.to_view(), "Bonjour le monde", "French should be preserved");
    expect_html_contains(env, output.to_view(), "Привет мир", "Russian should be preserved");
    expect_html_contains(env, output.to_view(), "你好世界", "Chinese should be preserved");
    expect_html_contains(env, output.to_view(), "こんにちは世界", "Japanese should be preserved");
    expect_html_contains(env, output.to_view(), "مرحبا بالعالم", "Arabic should be preserved");
    expect_html_contains(env, output.to_view(), "שלום עולם", "Hebrew should be preserved");
    expect_html_contains(env, output.to_view(), "∑∏∫∆∇∂", "Math symbols should be preserved");
    expect_html_contains(env, output.to_view(), "$€£¥₹", "Currency symbols should be preserved");
}

@test
func test_nested_structures_complex(env : &mut TestEnv) {
    var input = std::string_view(
"""> # Quote with header
> This is a quote with **bold** and *italic* text.
>
> - List in quote
> - Another item
>
> ```code
> code in quote
> ```
>
> > Nested quote"""
    );
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    expect_html_contains(env, output.to_view(), "md-blockquote", "Should have blockquote");
    expect_html_contains(env, output.to_view(), "md-hg md-h1", "Should have header in quote");
    expect_html_contains(env, output.to_view(), "md-bold", "Should have bold in quote");
    expect_html_contains(env, output.to_view(), "md-italic", "Should have italic in quote");
    expect_html_contains(env, output.to_view(), "md-ul", "Should have list in quote");
    expect_html_contains(env, output.to_view(), "md-pre", "Should have code block in quote");
}
