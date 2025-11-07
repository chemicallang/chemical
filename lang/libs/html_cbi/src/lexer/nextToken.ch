
func getNextToken2(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
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
        '<' => {
            const p = provider.peek()
            if(p == '!') {
                provider.readCharacter();
                const next = provider.peek()
                if(next == '-') {

                    // its a comment
                    provider.readCharacter()
                    if(provider.peek() == '-') {
                        provider.readCharacter()
                    }
                    html.is_comment = true;

                    return Token {
                        type : TokenType.CommentStart as int,
                        value : view("<!--"),
                        position : position
                    }

                } else if(isalpha(next)) {

                    // its a directive
                    // TODO handle this case
                    return Token {
                        type : TokenType.Unexpected as int,
                        value : view("expected directive or comment"),
                        position : position
                    }

                } else {
                    return Token {
                        type : TokenType.Unexpected as int,
                        value : view("expected directive or comment"),
                        position : position
                    }
                }
            } else if(p == '/') {
                html.has_lt = true;
                provider.readCharacter();
                return Token {
                    type : TokenType.TagEnd as int,
                    value : view("</"),
                    position : position
                }
            } else {
                html.has_lt = true;
                return Token {
                    type : TokenType.LessThan as int,
                    value : view("<"),
                    position : position
                }
            }
        }
        '}' => {
            if(html.lb_count == 1) {
                html.reset();
                lexer.unsetUserLexer();
            } else {
                html.lb_count--;
            }
            return Token {
                type : TokenType.RBrace as int,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            // here always lb_count > 0
            if(html.lb_count == 1) {
                html.other_mode = true;
                html.chemical_mode = true;
            }
            html.lb_count++;
            return Token {
                type : TokenType.LBrace as int,
                value : view("{"),
                position : position
            }
        }
        ' ', '\t', '\n', '\r' => {
            provider.skip_whitespaces();
            return getNextToken2(html, lexer);
        }
        '/' => {
            if(html.has_lt) {
                return Token {
                    type : TokenType.FwdSlash as int,
                    value : view("/"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected as int,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        '>' => {
            if(html.has_lt) {
                // reset back
                html.has_lt = false;
                html.lexed_tag_name = false;
                return Token {
                    type : TokenType.GreaterThan as int,
                    value : view(">"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected as int,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        default => {
            if(html.has_lt) {
                if(isalpha(c as int)) {
                    if(html.lexed_tag_name) {
                        provider.read_attr_name();
                        return Token {
                            type : TokenType.AttrName as int,
                            value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                            position : position
                        }
                    } else {
                        html.lexed_tag_name = true;
                        provider.read_tag_name();
                        return Token {
                            type : TokenType.TagName as int,
                            value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                            position : position
                        }
                    }
                } else {
                    switch(c) {
                        '=' => {
                           return Token {
                                type : TokenType.Equal as int,
                                value : view("="),
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
                        '"' => {
                            provider.read_double_quoted_value()
                            return Token {
                                type : TokenType.DoubleQuotedValue as int,
                                value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                                position : position
                            }
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
                                return Token {
                                    type : TokenType.Unexpected as int,
                                    value : view("tag names must start with letters"),
                                    position : position
                                }
                            }
                        }
                    }
                }
            } else {
                provider.read_text()
                return Token {
                    type : TokenType.Text as int,
                    value : std::string_view(data_ptr, provider.current_data() - data_ptr),
                    position : position
                }
            }
        }
    }
}