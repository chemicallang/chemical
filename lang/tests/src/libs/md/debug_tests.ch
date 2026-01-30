using namespace std;
using namespace md;

// Simple debug tests to isolate specific issues

@test
func debug_simple_table(env : &mut TestEnv) {
    env.info("=== DEBUG SIMPLE TABLE ===");
    
    var input = std::string_view("| A | B |\n|---|---|\n| 1 | 2 |");
    
    env.info("Input:");
    env.info(input.data());
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    env.info("Token count:");
    var count_str = std::string();
    count_str.append_uinteger(tokens.size());
    env.info(count_str.data());
    
    var i = 0u;
    while(i < tokens.size() && i < 20) {
        var token = tokens.get(i);
        var msg = std::string("Token ");
        msg.append_uinteger(i);
        msg.append_view(": type=");
        msg.append_integer(token.type);
        if(token.value.size() > 0) {
            msg.append_view(" value='");
            msg.append_view(token.value);
            msg.append_view("'");
        }
        env.info(msg.data());
        i++;
    }
    
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    env.info("Output:");
    env.info(output.data());
}

@test
func debug_simple_header(env : &mut TestEnv) {
    env.info("=== DEBUG SIMPLE HEADER ===");
    
    var input = std::string_view("# Header");
    
    env.info("Input:");
    env.info(input.data());
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    env.info("Token count:");
    var count_str = std::string();
    count_str.append_uinteger(tokens.size());
    env.info(count_str.data());
    
    var i = 0u;
    while(i < tokens.size() && i < 10) {
        var token = tokens.get(i);
        var msg = std::string("Token ");
        msg.append_uinteger(i);
        msg.append_view(": type=");
        msg.append_integer(token.type);
        if(token.value.size() > 0) {
            msg.append_view(" value='");
            msg.append_view(token.value);
            msg.append_view("'");
        }
        env.info(msg.data());
        i++;
    }
    
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    env.info("Output:");
    env.info(output.data());
}

@test
func debug_simple_link(env : &mut TestEnv) {
    env.info("=== DEBUG SIMPLE LINK ===");
    
    var input = std::string_view("[link](https://example.com)");
    
    env.info("Input:");
    env.info(input.data());
    
    var arena = md::Arena();
    var lexer = md::Lexer(input);
    var tokens = lexer.lex();
    
    env.info("Token count:");
    var count_str = std::string();
    count_str.append_uinteger(tokens.size());
    env.info(count_str.data());
    
    var i = 0u;
    while(i < tokens.size() && i < 15) {
        var token = tokens.get(i);
        var msg = std::string("Token ");
        msg.append_uinteger(i);
        msg.append_view(": type=");
        msg.append_integer(token.type);
        if(token.value.size() > 0) {
            msg.append_view(" value='");
            msg.append_view(token.value);
            msg.append_view("'");
        }
        env.info(msg.data());
        i++;
    }
    
    var root = md::parse(&mut tokens, &mut arena);
    var output = md::render_to_html(root);
    
    env.info("Output:");
    env.info(output.data());
}
