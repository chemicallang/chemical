using namespace std;
using namespace md;

// ===== LEXER TESTS =====

@test
func test_lexer_basic_text(env : &mut TestEnv) {
    var input = std::string_view("Hello world");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_headers(env : &mut TestEnv) {
    var input = std::string_view("# H1\n## H2\n### H3");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    // Should have: Hash, Text, Newline, Hash, Hash, Text, Newline, Hash, Hash, Hash, Text, EndOfFile
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Hash as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_bold_italic(env : &mut TestEnv) {
    var input = std::string_view("**bold** *italic* __bold__ _italic_");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Star as int);
    expected_types.push(md::MdTokenType.Star as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Star as int);
    expected_types.push(md::MdTokenType.Star as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Star as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Underscore as int);
    expected_types.push(md::MdTokenType.Underscore as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Underscore as int);
    expected_types.push(md::MdTokenType.Underscore as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Underscore as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_code_blocks(env : &mut TestEnv) {
    var input = std::string_view("```typescript\nconsole.log('hello');\n```");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.FencedCodeStart as int);
    expected_types.push(md::MdTokenType.CodeContent as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.FencedCodeEnd as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_links(env : &mut TestEnv) {
    var input = std::string_view("[link](https://example.com)");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.LParen as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RParen as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_emoji_utf8(env : &mut TestEnv) {
    var input = std::string_view("Hello 🚀 World ✨");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    // Should tokenize as: Text, Text, Text, EndOfFile
    // Emojis should be part of text tokens
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
    
    // Verify the text tokens contain the emojis
    if(tokens.size() >= 4) {
        var hello_token = tokens.get(0);
        var rocket_token = tokens.get(1);
        var world_token = tokens.get(2);
        
        if(!hello_token.value.contains("Hello")) {
            env.error("First token should contain 'Hello'");
        }
        if(!rocket_token.value.contains("🚀")) {
            env.error("Second token should contain rocket emoji");
        }
        if(!world_token.value.contains("World") || !world_token.value.contains("✨")) {
            env.error("Third token should contain 'World' and sparkle emoji");
        }
    }
}

@test
func test_lexer_task_lists(env : &mut TestEnv) {
    var input = std::string_view("- [x] done\n- [ ] pending");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Dash as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Dash as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_custom_containers(env : &mut TestEnv) {
    var input = std::string_view("::: info\nThis is info\n:::");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_abbreviations(env : &mut TestEnv) {
    var input = std::string_view("[HTML]: HyperText Markup Language\nHTML and CSS");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}

@test
func test_lexer_footnotes(env : &mut TestEnv) {
    var input = std::string_view("Text with footnote[^1]\n\n[^1]: Footnote definition");
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var expected_types = std::vector<int>();
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Caret as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.Newline as int);
    expected_types.push(md::MdTokenType.LBracket as int);
    expected_types.push(md::MdTokenType.Caret as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.RBracket as int);
    expected_types.push(md::MdTokenType.Colon as int);
    expected_types.push(md::MdTokenType.Text as int);
    expected_types.push(md::MdTokenType.EndOfFile as int);
    
    test_tokens_equal(env, tokens, expected_types);
}
