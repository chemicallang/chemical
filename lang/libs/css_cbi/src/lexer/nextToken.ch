
func getNextToken2(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    const provider = &lexer.provider;
    const str = &lexer.str;
    // the position of the current symbol
    const position = provider.getPosition();
    const c = provider.readCharacter();
    switch(c) {
        -1 => {
            return Token {
                type : TokenType.EndOfFile as int,
                value : view(""),
                position : position
            }
        }
        ':' => {
            return Token {
                type : TokenType.Colon as int,
                value : view(":"),
                position : position
            }
        }
        ';' => {
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
            if(isdigit(provider.peek())) {
                str.append('-');
                provider.read_floating_digits(lexer.str);
                return Token {
                    type : TokenType.Number as int,
                    value : str.finalize_view(),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Minus as int,
                    value : view("-"),
                    position : position
                }
            }
        }
        '.' => {
            str.append('.')
            provider.read_digits(lexer.str)
            return Token {
                type : TokenType.Number as int,
                value : str.finalize_view(),
                position : position
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
            return Token {
                type : TokenType.At as int,
                value : view("@"),
                position : position
            }
        }
        '!' => {
            var alpha = provider.read_alpha(*str);
            if(strncmp(alpha.data(), "important", alpha.size()) == 0) {
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
            str.append('#');
            return Token {
                type : TokenType.HexColor as int,
                value : provider.read_alpha_num(*str),
                position : position
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
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.EndsWith as int,
                    value : view("$="),
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
            str.append('"')
            return Token {
                type : TokenType.DoubleQuotedValue as int,
                value : provider.read_double_quoted_value(lexer.str),
                position : position
            }
        }
        '\'' => {
            str.append('\'');
            return Token {
                type : TokenType.SingleQuotedValue as int,
                value : provider.read_single_quoted_value(lexer.str),
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
                provider.readCharacter()
                provider.read_line(lexer.str)
                return Token {
                    type : TokenType.Comment as int,
                    value : str.finalize_view(),
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
            if(css.lb_count == 1) {
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
                str.append(c);
                provider.read_floating_digits(lexer.str);
                return Token {
                    type : TokenType.Number as int,
                    value : str.finalize_view(),
                    position : position
                }
            } else {
                str.append(c);
                var text = provider.read_css_id(lexer.str)
                return Token {
                    type : TokenType.Identifier as int,
                    value : text,
                    position : position
                }
            }
        }
    }
}