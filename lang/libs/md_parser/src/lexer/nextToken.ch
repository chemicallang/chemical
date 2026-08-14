/**
 * Shared markdown lexer core.
 *
 * Tokenizes markdown from a compiler `SourceProvider` without the chemical
 * interpolation wrapper (that lives in the md_cbi plugin). Both the runtime
 * `md` library (through compiler_runtime) and the `md_cbi` compiler plugin
 * drive this same function, so the two share identical tokenization logic.
 */
using namespace std;

func isEndMd(provider : *SourceProvider) : bool {
    // Check if current position has "endmd"
    const ptr = provider.data_ptr;
    if(ptr + 5 > provider.data_end) return false;
    if(*ptr != 'e') return false;
    if(*(ptr + 1) != 'n') return false;
    if(*(ptr + 2) != 'd') return false;
    if(*(ptr + 3) != 'm') return false;
    if(*(ptr + 4) != 'd') return false;
    // Make sure it's followed by whitespace or newline or EOF
    if(ptr + 5 >= provider.data_end) return true;
    const d5 = *(ptr + 5);
    return d5 == '\0' || d5 == '\n' || d5 == '\r' || d5 == ' ' || d5 == '\t';
}

func countBackticks(provider : *SourceProvider) : int {
    var count = 0;
    const ptr = provider.data_ptr;
    while(ptr + count < provider.data_end && *(ptr + count) == '`') {
        count++;
    }
    return count;
}

public func getNextToken2(md : &mut MdLexer, lexer : &mut Lexer) : Token {
    const provider = &raw lexer.provider;
    const position = provider.getPosition();
    const data_ptr = provider.current_data();

    // Inside fenced code block - read until closing fence
    if(md.in_fenced_code) {
        const ptr = provider.current_data();
        const end = provider.data_end;

        if (ptr >= end || *ptr == '\0') {
            md.in_fenced_code = false;
        } else {
            // Check for #endmd to avoid swallowing the macro terminator
            var is_end_md = false;
            if (*ptr == '#') {
                if (ptr + 5 < end && ptr[1] == 'e' && ptr[2] == 'n' && ptr[3] == 'd' && ptr[4] == 'm' && ptr[5] == 'd') {
                    if (ptr + 6 >= end) is_end_md = true;
                    else {
                        const d6 = ptr[6];
                        if (d6 == '\0' || d6 == '\n' || d6 == '\r' || d6 == ' ' || d6 == '\t') is_end_md = true;
                    }
                }
            }

            if (is_end_md) {
                md.in_fenced_code = false;
            } else {
                // Check for closing fence allowing up to 3 spaces indentation
                var offset = 0;
                var spaces = 0;

                while(spaces < 3 && ptr + offset < end && *(ptr + offset) == ' ') {
                    offset++;
                    spaces++;
                }

                var backtick_count = 0;
                while(ptr + offset + backtick_count < end && *(ptr + offset + backtick_count) == '`') {
                    backtick_count++;
                }

                if(backtick_count >= md.fence_count) {
                    // Check if rest of line is valid (empty or whitespace)
                    var p_end = offset + backtick_count;
                    var valid = true;
                    while(ptr + p_end < end) {
                        const c = *(ptr + p_end);
                        if(c == '\n' || c == '\0' || c == '\r') break;
                        if(c != ' ' && c != '\t') { valid = false; break; }
                        p_end++;
                    }

                    if(valid) {
                        // Consume spaces + backticks
                        var k = 0;
                        while(k < offset + backtick_count) {
                             provider.readCharacter();
                             k++;
                        }

                        md.in_fenced_code = false;
                        md.fence_count = 0;
                        // Skip rest of line
                        while(provider.peek() != '\n' && provider.peek() != '\0') {
                            provider.readCharacter();
                        }
                        if(provider.peek() == '\n') {
                            provider.readCharacter();
                        }
                        return Token { type : MdTokenType.FencedCodeEnd as int, value : std::string_view("```"), position : position }
                    }
                }

                // Read until end of line or closing fence
                while(provider.peek() != '\n' && provider.peek() != '\0') {
                    provider.readCharacter();
                }
                const code_line = std::string_view(data_ptr, provider.current_data() - data_ptr);
                if(provider.peek() == '\n') {
                    provider.readCharacter();
                }
                return Token { type : MdTokenType.CodeContent as int, value : code_line, position : position }
            }
        }
    }

    const c = provider.readCharacter();

    switch(c) {
        '\0' => {
            return Token { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : position }
        }
        '#' => {
            // Check for #endmd
            if(isEndMd(provider)) {
                // Consume "endmd"
                provider.readCharacter(); // e
                provider.readCharacter(); // n
                provider.readCharacter(); // d
                provider.readCharacter(); // m
                provider.readCharacter(); // d
                md.reset();
                lexer.unsetUserLexer();
                return Token { type : MdTokenType.EndMd as int, value : std::string_view("#endmd"), position : position }
            }
            return Token { type : MdTokenType.Hash as int, value : std::string_view("#"), position : position }
        }
        '$' => {
            if(provider.peek() == '{') {
                provider.readCharacter(); // consume {
                md.lb_count = 1;
                md.chemical_mode = true;
                return Token { type : MdTokenType.ChemicalStart as int, value : std::string_view("${"), position : position }
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view("$"), position : position }
        }
        '`' => {
            // Check for fenced code block (```)
            if(provider.peek() == '`') {
                provider.readCharacter(); // second `
                if(provider.peek() == '`') {
                    provider.readCharacter(); // third `
                    // Count any additional backticks
                    var count = 3;
                    while(provider.peek() == '`') {
                        provider.readCharacter();
                        count++;
                    }
                    // Skip spaces before language
                    while(provider.peek() == ' ' || provider.peek() == '\t') {
                        provider.readCharacter();
                    }
                    // Read language identifier
                    const lang_start = provider.current_data();
                    while(provider.peek() != '\n' && provider.peek() != '\r' && provider.peek() != '\0' && provider.peek() != ' ') {
                        provider.readCharacter();
                    }
                    const lang = std::string_view(lang_start, provider.current_data() - lang_start);
                    // Skip rest of line
                    while(provider.peek() != '\n' && provider.peek() != '\0') {
                        provider.readCharacter();
                    }
                    if(provider.peek() == '\n') {
                        provider.readCharacter();
                    }
                    md.in_fenced_code = true;
                    md.fence_char = '`';
                    md.fence_count = count;
                    // Return token with language
                    if(lang.size() > 0) {
                        return Token { type : MdTokenType.FencedCodeStart as int, value : lang, position : position }
                    }
                    return Token { type : MdTokenType.FencedCodeStart as int, value : std::string_view(""), position : position }
                }
                // Just two backticks, treat as inline code
                return Token { type : MdTokenType.Backtick as int, value : std::string_view("``"), position : position }
            }
            return Token { type : MdTokenType.Backtick as int, value : std::string_view("`"), position : position }
        }
        '{' => {
            return Token { type : MdTokenType.LBrace as int, value : std::string_view("{"), position : position }
        }
        '}' => {
            return Token { type : MdTokenType.RBrace as int, value : std::string_view("}"), position : position }
        }
        '*' => { return Token { type : MdTokenType.Star as int, value : std::string_view("*"), position : position } }
        '_' => { return Token { type : MdTokenType.Underscore as int, value : std::string_view("_"), position : position } }
        '[' => { return Token { type : MdTokenType.LBracket as int, value : std::string_view("["), position : position } }
        ']' => { return Token { type : MdTokenType.RBracket as int, value : std::string_view("]"), position : position } }
        '(' => { return Token { type : MdTokenType.LParen as int, value : std::string_view("("), position : position } }
        ')' => { return Token { type : MdTokenType.RParen as int, value : std::string_view(")"), position : position } }
        '!' => { return Token { type : MdTokenType.Exclamation as int, value : std::string_view("!"), position : position } }
        '>' => { return Token { type : MdTokenType.GreaterThan as int, value : std::string_view(">"), position : position } }
        '-' => { return Token { type : MdTokenType.Dash as int, value : std::string_view("-"), position : position } }
        '+' => { return Token { type : MdTokenType.Plus as int, value : std::string_view("+"), position : position } }
        '|' => { return Token { type : MdTokenType.Pipe as int, value : std::string_view("|"), position : position } }
        '~' => { return Token { type : MdTokenType.Tilde as int, value : std::string_view("~"), position : position } }
        ':' => { return Token { type : MdTokenType.Colon as int, value : std::string_view(":"), position : position } }
        '=' => { return Token { type : MdTokenType.Equal as int, value : std::string_view("="), position : position } }
        '^' => { return Token { type : MdTokenType.Caret as int, value : std::string_view("^"), position : position } }
        '.' => { return Token { type : MdTokenType.Dot as int, value : std::string_view("."), position : position } }
        '\n' => { return Token { type : MdTokenType.Newline as int, value : std::string_view("\n"), position : position } }
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' => {
            // Read full number
            while(provider.peek() >= '0' && provider.peek() <= '9') {
                provider.readCharacter();
            }
            return Token { type : MdTokenType.Number as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
        default => {
            while(true) {
                const next = provider.peek();
                if(next == '\0' || next == '#' || next == '*' || next == '_' || next == '[' || next == ']' ||
                   next == '(' || next == ')' || next == '!' || next == '`' || next == '>' || next == '-' ||
                   next == '+' || next == '|' || next == '\n' || next == '{' || next == '}' || next == '$' ||
                   next == '~' || next == ':' || next == '=' || next == '^' || next == '.' ||
                   (next >= '0' && next <= '9')) {
                    break;
                }
                provider.readCharacter();
            }
            return Token { type : MdTokenType.Text as int, value : std::string_view(data_ptr, provider.current_data() - data_ptr), position : position }
        }
    }
}
