/**
 * Runtime css tokenizer.
 *
 * Reuses the shared css_parser lexer (`getNextToken2`) at runtime by
 * constructing a real `SourceProvider` (backed by the runtime symbols in
 * compiler_runtime) and a real `Lexer` + `CSSLexer` struct, then driving the
 * lexer until EndOfFile. This produces exactly the same token stream the CBI
 * plugin path produces.
 *
 * css_cbi's `getNextToken` wrapper adds handling for chemical-embedded modes
 * (`${...}`). Those are not supported at runtime and fall through to the
 * plain lexer (the `other_mode`/`chemical_mode` flags simply never trigger
 * embedded-token reads because there is no compiler to read them).
 */
using namespace std;

public func tokenize_css_impl(view : std::string_view) : std::vector<Token> {
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

    var css = CSSLexer {
        other_mode : false,
        chemical_mode : false,
        lb_count : 0,
        at_rule : false,
        start_chemical_lb_count : 1,
        has_chemical_in_value : false,
        tokens_since_colon : 0,
        where_state : CSSLexerWhere.Declaration
    }

    while(true) {
        const t = getNextToken2(&mut css, &mut lexer)
        if(t.type == TokenType.EndOfFile as int) {
            tokens.push(t)
            break
        }
        tokens.push(t)
    }

    return tokens
}
