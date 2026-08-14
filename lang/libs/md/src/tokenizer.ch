/**
 * Runtime markdown tokenizer.
 *
 * Reuses the shared md_parser lexer (`getNextToken2`) at runtime by
 * constructing a real `SourceProvider` (backed by the runtime symbols in
 * compiler_runtime) and a real `Lexer` struct, then driving the lexer until
 * EndOfFile. This produces exactly the same token stream the CBI plugin path
 * produces.
 *
 * Chemical interpolation (`${...}`) cannot be evaluated without the compiler,
 * so at runtime it degrades gracefully: the `$` is lexed as a ChemicalStart
 * token and the interpolation node ends up with a null value which the runtime
 * converter skips (mirroring how the runtime html converter skips chemical
 * children).
 */
using namespace std;

public func tokenize_md_impl(view : std::string_view) : std::vector<Token> {
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

    var md = MdLexer {
        in_fenced_code : false,
        fence_char : '\0',
        fence_count : 0,
        chemical_mode : false,
        lb_count : 0
    }

    while(true) {
        const t = getNextToken2(&mut md, &mut lexer)
        if(t.type == MdTokenType.EndOfFile as int) {
            tokens.push(t)
            break
        }
        tokens.push(t)
    }

    return tokens
}
