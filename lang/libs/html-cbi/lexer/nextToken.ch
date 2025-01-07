import "@compiler/Lexer.ch"
import "./TokenType.ch"
import "./readutils.ch"
import "../utils/comptime_utils.ch"

func getNextToken2(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    const provider = &lexer.provider;
    const str = &lexer.str;
    // the position of the current symbol
    const position = provider.getPosition();
    const c = provider.readCharacter();
    printf("reading character : %d\n", c);
    switch(c) {
        '<' => {
            html.has_lt = true;
            return Token {
                type : TokenType.LessThan,
                value : view("<"),
                position : position
            }
        }
        '}' => {
            if(html.lb_count == 1) {
                printf("exiting, didn't expect an rb\n");
                html.reset();
                lexer.unsetUserLexer();
            } else {
                html.lb_count--;
                printf("lb_count decreased to %d\n", html.lb_count);
            }
            return Token {
                type : TokenType.RBrace,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            // here always lb_count > 0
            if(html.lb_count == 1) {
                printf("turning on chemical mode\n");
                html.other_mode = true;
                html.chemical_mode = true;
            }
            html.lb_count++;
            printf("lb_count increased to %d\n", html.lb_count);
            return Token {
                type : TokenType.LBrace,
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
                    type : TokenType.FwdSlash,
                    value : view("/"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        '>' => {
            if(html.has_lt) {
                html.has_lt = false;
                return Token {
                    type : TokenType.GreaterThan,
                    value : view(">"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view("lt is not open"),
                    position : position
                }
            }
        }
        default => {
            if(html.has_lt) {
                if(isalpha(c as int)) {
                    str.append(c);
                    const tag_name = provider.read_tag_name(lexer.str);
                    return Token {
                        type : TokenType.Identifier,
                        value : tag_name,
                        position : position
                    }
                } else {
                    switch(c) {
                        '=' => {
                           return Token {
                                type : TokenType.Equal,
                                value : view("="),
                                position : position
                           }
                        }
                        '\'' => {
                            str.append('\'');
                            return Token {
                                type : TokenType.SingleQuotedValue,
                                value : provider.read_single_quoted_value(lexer.str),
                                position : position
                            }
                        }
                        '"' => {
                            str.append('"')
                            return Token {
                                type : TokenType.DoubleQuotedValue,
                                value : provider.read_double_quoted_value(lexer.str),
                                position : position
                            }
                        }
                        default => {
                            if(isdigit(c)) {
                                str.append(c);
                                provider.read_floating_digits(lexer.str);
                                return Token {
                                    type : TokenType.Number,
                                    value : str.finalize_view(),
                                    position : position
                                }
                            } else {
                                return Token {
                                    type : TokenType.Unexpected,
                                    value : view("tag names must start with letters"),
                                    position : position
                                }
                            }
                        }
                    }
                }
            } else {
                printf("starting text with character : %d\n", c);
                str.append(c);
                var text = provider.read_text(lexer.str)
                if(text.size() != 0) {
                    return Token {
                        type : TokenType.Text,
                        value : text,
                        position : position
                    }
                }
            }
        }
    }
}