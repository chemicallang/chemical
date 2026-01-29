using namespace std;
using namespace md;

// Helper functions for md tests

public func test_markdown_roundtrip(env : &mut TestEnv, input : std::string_view, expected : std::string_view) {
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    if(!output.equals_view(expected)) {
        env.error("Markdown roundtrip failed");
        
        var exp_msg = std::string("expected: \"");
        exp_msg.append_view(expected);
        exp_msg.append_view("\"");
        env.info(exp_msg.data());
        
        var got_msg = std::string("got: \"");
        got_msg.append_view(output.to_view());
        got_msg.append_view("\"");
        env.info(got_msg.data());
    }
}

public func test_tokens_equal(env : &mut TestEnv, tokens : &mut std::vector<md::MdToken>, expected_types : &mut std::vector<int>) {
    if(tokens.size() != expected_types.size()) {
        env.error("Token count mismatch");
        return;
    }
    
    var i = 0u;
    while(i < tokens.size()) {
        var token = tokens.get(i);
        var expected_type = expected_types.get(i);
        if(token.type != expected_type) {
            env.error("Token type mismatch");
            return;
        }
        i++;
    }
}

public func expect_html_contains(env : &mut TestEnv, html : std::string_view, expected_fragment : std::string_view, msg : *char) {
    if(!html.contains(expected_fragment)) {
        env.error(msg);
        
        var frag_msg = std::string("expected fragment: \"");
        frag_msg.append_view(expected_fragment);
        frag_msg.append_view("\"");
        env.info(frag_msg.data());
        
        var html_msg = std::string("actual html: \"");
        html_msg.append_view(html);
        html_msg.append_view("\"");
        env.info(html_msg.data());
    }
}
