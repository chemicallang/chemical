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
 * handling so comments lex as CommentText tokens (matching the CBI path).
 *
 * chemical-embedded modes are NOT supported at runtime, and this wrapper is
 * where that is enforced. The shared lexer is written for the #html macro: it
 * treats '{' as the start of a chemical value, '@{ ... }' as a nested statement
 * level, '@( ... )' as an explicit value, and '@if(...)' / '@else' as control
 * flow. None of that means anything to the runtime parser, and because the mode
 * flags the lexer sets are only cleared by html_cbi's wrapper, those tokens
 * used to be emitted and then silently dropped by the runtime parser -- so
 * "<div>{ x }</div>" came back as "<div> x </div>", and "<div>@{ x }</div>"
 * did the same. That is data loss, not a rendering choice.
 *
 * So those characters are intercepted here, before the shared lexer can act on
 * them, and handed back as plain Text tokens. A browser does exactly this: in
 * html text only '<' and '&' are special, and a brace is a character.
 * Intercepting up front also leaves lb_count and the mode flags untouched,
 * which patching an already-returned token could not do.
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

    // there is no chemical syntax at runtime: '{', '}' and '@' are characters
    {
        const provider = &mut lexer.provider;
        const position = provider.getPosition();
        const start = provider.current_data();
        const at = *start;
        if(at == '{' || at == '}') {
            provider.readCharacter();
            // keep reading the run so that the whitespace between this
            // character and the next is carried inside a Text token. Handing
            // back one character at a time would split the run, and the
            // lexer's whitespace rule drops a run that sits before a '}'
            // (it assumes a chemical value is closing), which turned
            // "<pre>{ }</pre>" into "<pre>{}</pre>".
            provider.read_literal_text();
            return Token {
                type : TokenType.Text as int,
                value : std::string_view(start, provider.current_data() - start),
                position : position
            }
        }
        if(at == '@') {
            provider.readCharacter();
            const after = provider.peek();
            if(after == '{' || after == '(') {
                // "@{" or "@(" -- meaningless at runtime, so just text
                provider.readCharacter();
            } else if(isalpha(after as int)) {
                // "@name" -- a decorator, an annotation, an e-mail local part
                provider.read_tag_name();
            }
            provider.read_literal_text();
            return Token {
                type : TokenType.Text as int,
                value : std::string_view(start, provider.current_data() - start),
                position : position
            }
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
        in_paren_value : false,
        expecting_html_block : false,
        last_token_was_if : false,
        after_chem_expr : false,
        pre_depth : 0,
        pre_brace_depth : 0,
        in_end_tag : false,
        last_tag_pre : false,
        preserve_whitespace : true,
        pending_script : false,
        in_script : false
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
