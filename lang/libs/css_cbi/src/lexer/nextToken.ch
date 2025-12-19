func is_ident_start(next : char) : bool {
    return next == '_' || isalpha(next) || (next >= 0x80) || (next == '\\') || (next == '-');
}

func getNextToken2(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    const provider = &mut lexer.provider;
    // the position of the current symbol
    const position = provider.getPosition();
    const data_ptr = provider.current_data()
    const c = provider.readCharacter();
    switch(c) {
        '\0' => {
            return Token {
                type : TokenType.EndOfFile as int,
                value : view(""),
                position : position
            }
        }
        '&' => {
            css.where = CSSLexerWhere.Selector;
            return Token {
                type : TokenType.Ampersand as int,
                value : view("&"),
                position : position
            }
        }
        ':' => {
            if(css.where == CSSLexerWhere.Declaration) {
                css.where = CSSLexerWhere.Value
            }
            return Token {
                type : TokenType.Colon as int,
                value : view(":"),
                position : position
            }
        }
        ';' => {
            if(css.where == CSSLexerWhere.Value) {
                css.where = CSSLexerWhere.Declaration
            }
            return Token {
                type : TokenType.Semicolon as int,
                value : view(";"),
                position : position
            }
        }
        ',' => {
            return Token {
                type : TokenType.Comma as int,
                value : view(","),
                position : position
            }
        }
        '+' => {
            return Token {
                type : TokenType.Plus as int,
                value : view("+"),
                position : position
            }
        }
        '-' => {
            const next = provider.peek()
            const next2 = provider.peek2()

            if(isdigit(next) || (next == '.' && isdigit(next2))) {
                provider.read_floating_digits()
                return Token {
                    type : TokenType.Number as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            }

            if (next == '-' || is_ident_start(next)) {
                provider.read_css_id();
                const value = std::string_view(data_ptr, provider.current_data() - data_ptr);
                if (css.where == CSSLexerWhere.Declaration) {
                    return Token{
                        type: TokenType.PropertyName as int,
                        value: value,
                        position: position
                    };
                } else {
                    return Token{
                        type: TokenType.Identifier as int,
                        value: value,
                        position: position
                    };
                }
            }

            return Token {
                type : TokenType.Minus as int,
                value : view("-"),
                position : position
            }

        }
        '.' => {
            if(css.where == CSSLexerWhere.GlobalBlock || css.where == CSSLexerWhere.Selector) {
                const start = provider.current_data()
                provider.read_css_id()
                return Token {
                    type : TokenType.ClassName as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            } else {
                provider.read_digits()
                return Token {
                    type : TokenType.Number as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            }
        }
        '(' => {
            return Token {
                type : TokenType.LParen as int,
                value : view("("),
                position : position
            }
        }
        ')' => {
            return Token {
                type : TokenType.RParen as int,
                value : view(")"),
                position : position
            }
        }
        '@' => {
            css.at_rule = true;
            return Token {
                type : TokenType.At as int,
                value : view("@"),
                position : position
            }
        }
        '!' => {
            const start = provider.current_data()
            provider.read_alpha();
            if(strncmp(start, "important", provider.current_data() - start) == 0) {
                return Token {
                    type : TokenType.Important as int,
                    value : view("!important"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected as int,
                    value : view(""),
                    position : position
                }
            }
        }
        '#' => {
            if(css.where == CSSLexerWhere.GlobalBlock || css.where == CSSLexerWhere.Selector) {
                const start = provider.current_data()
                provider.read_css_id()
                return Token {
                    type : TokenType.Id as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            } else {
                provider.read_alpha_num()
                return Token {
                    type : TokenType.HexColor as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            }
        }
        '%' => {
            return Token {
                type : TokenType.Percentage as int,
                value : view("%"),
                position : position
            }
        }
        '*' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.ContainsSubstr as int,
                    value : view("*="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Multiply as int,
                    value : view("*"),
                    position : position
                }
            }
        }
        '^' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.StartsWith as int,
                    value : view("^="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Unexpected as int,
                    value : view(""),
                    position : position
                }
            }
        }
        '$' => {
            switch(provider.peek()) {
                '{' => {
                    provider.readCharacter();
                    css.other_mode = true;
                    css.chemical_mode = true;
                    css.lb_count++;
                    return Token {
                        type : TokenType.DollarLBrace as int,
                        value : view("${"),
                        position : position
                    }
                }
                '=' => {
                    provider.readCharacter();
                    return Token {
                        type : TokenType.EndsWith as int,
                        value : view("$="),
                        position : position
                    }
                }
                default => {
                    return Token {
                        type : TokenType.Unexpected as int,
                        value : view(""),
                        position : position
                    }
                }
            }
        }
        '|' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.DashSeparatedMatch as int,
                    value : view("|="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Unexpected as int,
                    value : view(""),
                    position : position
                }
            }
        }
        '"' => {
            provider.read_double_quoted_value()
            return Token {
                type : TokenType.DoubleQuotedValue as int,
                value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                position : position
            }
        }
        '\'' => {
            provider.read_single_quoted_value()
            return Token {
                type : TokenType.SingleQuotedValue as int,
                value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                position : position
            }
        }
        '~' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.ContainsWord as int,
                    value : view("~="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.GeneralSibling as int,
                    value : view("~"),
                    position : position
                }
            }
        }
        '>' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.GreaterThanOrEqual as int,
                    value : view(">="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.GreaterThan as int,
                    value : view(">"),
                    position : position
                }
            }
        }
        '<' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.LessThanOrEqual as int,
                    value : view("<="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.LessThan as int,
                    value : view("<"),
                    position : position
                }
            }
        }
        '/' => {
            if(provider.peek() == '/') {
                provider.increment()
                const start = provider.current_data()
                provider.read_line()
                return Token {
                    type : TokenType.Comment as int,
                    value : std::string_view(start, provider.current_data() - start),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Divide as int,
                    value : view("/"),
                    position : position
                }
            }
        }
        '=' => {
            return Token {
                type : TokenType.Equal as int,
                value : view("="),
                position : position
            }
        }
        '}' => {
            if(css.lb_count == 1) {
                css.reset();
                lexer.unsetUserLexer();
            } else {
                css.lb_count--;
            }
            return Token {
                type : TokenType.RBrace as int,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            // here always lb_count > 0
            if(css.at_rule) {
                css.at_rule = false;
            } else if(css.where == CSSLexerWhere.Selector) {
                css.where = CSSLexerWhere.Declaration;
            } else if(css.where == CSSLexerWhere.Value && css.lb_count == 1) {
                css.other_mode = true;
                css.chemical_mode = true;
            }
            css.lb_count++;
            return Token {
                type : TokenType.LBrace as int,
                value : view("{"),
                position : position
            }
        }
        ' ', '\t', '\n', '\r' => {
            provider.skip_whitespaces();
            return getNextToken2(css, lexer);
        }
        default => {
            if(isdigit(c)) {
                provider.read_floating_digits();
                return Token {
                    type : TokenType.Number as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            } else {
                provider.read_css_id()
                var value = std::string_view(data_ptr, provider.current_data() - data_ptr)
                if(css.where == CSSLexerWhere.Declaration) {
                    return Token {
                        type : TokenType.PropertyName as int,
                        value : value,
                        position : position
                    }
                } else {
                    return Token {
                        type : TokenType.Identifier as int,
                        value : value,
                        position : position
                    }
                }
            }
        }
    }
}