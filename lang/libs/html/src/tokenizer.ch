/**
 * Runtime html tokenizer.
 *
 * Reuses the shared html_parser lexer (`getNextToken2`) at runtime by
 * constructing a real `SourceProvider` (backed by the runtime symbols in
 * compiler_runtime) and a real `Lexer` struct, then driving the lexer until
 * EndOfFile. This produces exactly the same token stream the CBI plugin path
 * produces.
 *
 * html_cbi's `getNextToken` wrapper adds handling for comment text and
 * chemical-embedded modes. This runtime wrapper replicates the comment-text
 * handling so comments lex as CommentText tokens (matching the CBI path);
 * chemical-embedded modes (@{...}) are not supported at runtime and fall
 * through to the plain lexer.
 */
using namespace std;

public func get_next_token_runtime(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    if(html.is_comment) {
        const provider = &mut lexer.provider
        const position = provider.getPosition();
        const data_ptr = provider.current_data()
        const has_end = provider.read_comment_text()
        if(has_end) {
            // comment has ended
            html.is_comment = false;
        }
        var end_offset = 0
        if(has_end) {
            end_offset = 3
        }
        return Token {
            type : TokenType.CommentText as int,
            value : std::string_view(data_ptr, (provider.current_data() - end_offset) - data_ptr),
            position : position
        }
    }
    const t = getNextToken2(html, lexer)
    html.after_chem_expr = false
    return t
}

public func tokenize_html_impl(view : std::string_view) : std::vector<Token> {
    var tokens = std::vector<Token>()

    var provider = SourceProvider {
        data_ptr : view.data() as *mut char,
        data_len : view.size(),
        data_end : view.data() as *mut char + view.size(),
        lineNumber : 0,
        lineCharacterNumber : 0
    }

    var lexer = Lexer {
        LexerState : LexerState {
            other_mode : false,
            user_mode : false
        },
        provider : provider,
        user_lexer : UserLexerFn { instance : null, subroutine : null }
    }

    var html = HtmlLexer {
        has_lt : false,
        lexed_tag_name : false,
        is_comment : false,
        other_mode : false,
        chemical_mode : false,
        lb_count : 0,
        paren_count : 0,
        chem_start_lb : 0,
        in_paren_expr : false,
        expecting_html_block : false,
        last_token_was_if : false,
        after_chem_expr : false
    }

    while(true) {
        const t = get_next_token_runtime(&mut html, &mut lexer)
        if(t.type == TokenType.EndOfFile as int) {
            tokens.push(t)
            break
        }
        tokens.push(t)
    }

    return tokens
}
