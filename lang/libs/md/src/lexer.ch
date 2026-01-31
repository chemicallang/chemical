public namespace md {

public struct Token {
    var type : MdTokenType
    var value : std::string_view
    // Simple position tracking if needed, or just offset?
    // conversion currently doesn't seem to use position much for error reporting (since we are lenient).
    // We can keep it simple.
}

struct Lexer {
    var text : std::string_view
    var pos : size_t
    var in_fenced_code : bool
    var fence_char : char
    var fence_count : int

    func peek(&mut self) : char {
        if(pos >= text.size()) return '\0';
        return text.data()[pos];
    }

    func peek_at(&mut self, offset : size_t) : char {
        if(pos + offset >= text.size()) return '\0';
        return text.data()[pos + offset];
    }

    func advance(&mut self) : char {
        if(pos >= text.size()) return '\0';
        const c = text.data()[pos];
        pos++;
        return c;
    }

    func countBackticks(&mut self) : int {
        var count = 0;
        var p = pos;
        while(p < text.size() && text.data()[p] == '`') {
            count++;
            p++;
        }
        return count;
    }

    func consume_until_newline(&mut self) : std::string_view {
        const start = pos;
        while(pos < text.size()) {
            const c = text.data()[pos];
            if(c == '\n' || c == '\0') break;
            pos++;
        }
        return std::string_view(text.data() + start, pos - start);
    }
    
    func consume_newline(&mut self) {
         if(pos < text.size() && text.data()[pos] == '\n') {
            pos++;
        }
    }

    func next_token(&mut self) : Token {
        if(pos >= text.size()) {
            return Token { type : MdTokenType.EndOfFile, value : std::string_view("") }
        }

        // Fenced Code Block content handling
        if(in_fenced_code) {
             // Check for closing fence allowing up to 3 spaces indentation
            var saved_pos = pos;
            var spaces = 0;
            // Check for indented fence (CommonMark allows up to 3 spaces)
            while(spaces < 3 && pos < text.size() && text.data()[pos] == ' ') {
                pos++;
                spaces++;
            }
            
            var backtick_count = countBackticks() as size_t;
            if(backtick_count >= fence_count) {
                 // Verify that the rest of the line is empty or just whitespace
                 var p_check = pos + backtick_count;
                 var valid_end = true;
                 while(p_check < text.size()) {
                     const c = text.data()[p_check];
                     if(c == '\n' || c == '\0' || c == '\r') break;
                     if(c != ' ' && c != '\t') {
                         valid_end = false;
                         break;
                     }
                     p_check++;
                 }
                 
                 if(valid_end) {
                     // Found valid closing fence
                     pos += backtick_count;
                     in_fenced_code = false;
                     fence_count = 0;
                     consume_until_newline();
                     consume_newline();
     
                     return Token { type : MdTokenType.FencedCodeEnd, value : std::string_view("```") }
                 }
            }
            
            // Not a closing fence: Restore pos to include spaces in content
            pos = saved_pos;

            // Read code content line
            const line = consume_until_newline();
            consume_newline();
            
            return Token { type : MdTokenType.CodeContent, value : line }
        }

        const start = pos;
        const c = advance();

        switch(c) {
            '\0' => { return Token { type : MdTokenType.EndOfFile, value : std::string_view("") } }
            '#' => { return Token { type : MdTokenType.Hash, value : std::string_view("#") } }
            // '$' case removed (no chemical interpolation)
            '`' => {
                if(peek() == '`') {
                    advance(); // 2nd `
                    if(peek() == '`') {
                        advance(); // 3rd `
                        // Fenced Code Start
                        var count = 3;
                        while(peek() == '`') {
                            advance();
                            count++;
                        }
                        
                        // Skip spaces
                        while(peek() == ' ' || peek() == '\t') {
                            advance();
                        }
                        
                        // Read language
                        const lang_start = pos;
                        while(peek() != '\n' && peek() != '\r' && peek() != '\0' && peek() != ' ') {
                            advance();
                        }
                        const lang = std::string_view(text.data() + lang_start, pos - lang_start);
                        
                        // Skip rest of line
                        consume_until_newline();
                        consume_newline();
                        
                        in_fenced_code = true;
                        fence_char = '`';
                        fence_count = count;
                        
                        return Token { type : MdTokenType.FencedCodeStart, value : lang }
                    }
                    return Token { type : MdTokenType.Backtick, value : std::string_view("``") }
                }
                return Token { type : MdTokenType.Backtick, value : std::string_view("`") }
            }
            '{' => { return Token { type : MdTokenType.LBrace, value : std::string_view("{") } }
            '}' => { return Token { type : MdTokenType.RBrace, value : std::string_view("}") } }
            '*' => { return Token { type : MdTokenType.Star, value : std::string_view("*") } }
            '_' => { return Token { type : MdTokenType.Underscore, value : std::string_view("_") } }
            '[' => { return Token { type : MdTokenType.LBracket, value : std::string_view("[") } }
            ']' => { return Token { type : MdTokenType.RBracket, value : std::string_view("]") } }
            '(' => { return Token { type : MdTokenType.LParen, value : std::string_view("(") } }
            ')' => { return Token { type : MdTokenType.RParen, value : std::string_view(")") } }
            '!' => { return Token { type : MdTokenType.Exclamation, value : std::string_view("!") } }
            '>' => { return Token { type : MdTokenType.GreaterThan, value : std::string_view(">") } }
            '-' => { return Token { type : MdTokenType.Dash, value : std::string_view("-") } }
            '+' => { return Token { type : MdTokenType.Plus, value : std::string_view("+") } }
            '|' => { return Token { type : MdTokenType.Pipe, value : std::string_view("|") } }
            '~' => { return Token { type : MdTokenType.Tilde, value : std::string_view("~") } }
            ':' => { return Token { type : MdTokenType.Colon, value : std::string_view(":") } }
            '=' => { return Token { type : MdTokenType.Equal, value : std::string_view("=") } }
            '^' => { return Token { type : MdTokenType.Caret, value : std::string_view("^") } }
            '.' => { return Token { type : MdTokenType.Dot, value : std::string_view(".") } }
            '\n' => { return Token { type : MdTokenType.Newline, value : std::string_view("\n") } }
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' => {
                while(peek() >= '0' && peek() <= '9') {
                    advance();
                }
                return Token { type : MdTokenType.Number, value : std::string_view(text.data() + start, pos - start) }
            }
            default => {
                // Read text until a special char
                while(true) {
                    const next = peek();
                    if(next == '\0' || next == '#' || next == '*' || next == '_' || next == '[' || next == ']' || 
                       next == '(' || next == ')' || next == '!' || next == '`' || next == '>' || next == '-' || 
                       next == '+' || next == '|' || next == '\n' || next == '{' || next == '}' || 
                       next == '~' || next == ':' || next == '=' || next == '^' || next == '.' ||
                       (next >= '0' && next <= '9')) {
                        break;
                    }
                    advance();
                }
                return Token { type : MdTokenType.Text, value : std::string_view(text.data() + start, pos - start) }
            }
        }
    }
}

public func lex(text : std::string_view) : std::vector<Token> {
    var lexer = Lexer { 
        text : text, 
        pos : 0, 
        in_fenced_code : false, 
        fence_char : '\0', 
        fence_count : 0 
    }
    
    var tokens = std::vector<Token>();
    // Pre-reserve?
    
    while(true) {
        var tok = lexer.next_token();
        tokens.push_back(tok);
        if(tok.type == MdTokenType.EndOfFile) {
            break;
        }
    }
    return tokens;
}

} // namespace md
