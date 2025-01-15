import "@compiler/Lexer.ch"
import "./TokenType.ch"
import "./readutils.ch"
import "../utils/comptime_utils.ch"


// TODO Greater than or equal to (>=)
// TODO Less than or equal to (<=)
// TODO Exact match (=)
// TODO Contains word (~=)
// TODO Contains substring (*=)
// TODO Starts with (^=)
// TODO Ends with ($=)
// TODO Dash-separated match (|=)

func getNextToken2(css : &mut CSSLexer, lexer : &mut Lexer) : Token {
    const provider = &lexer.provider;
    const str = &lexer.str;
    // the position of the current symbol
    const position = provider.getPosition();
    const c = provider.readCharacter();
    printf("reading character : %d\n", c);
    switch(c) {
        ':' => {
            return Token {
                type : TokenType.Colon,
                value : view(":"),
                position : position
            }
        }
        ';' => {
            return Token {
                type : TokenType.Semicolon,
                value : view(";"),
                position : position
            }
        }
        ',' => {
            return Token {
                type : TokenType.Comma,
                value : view(","),
                position : position
            }
        }
        '+' => {
            return Token {
                type : TokenType.Plus,
                value : view("+"),
                position : position
            }
        }
        '-' => {
            return Token {
                type : TokenType.Minus,
                value : view("-"),
                position : position
            }
        }
        '@' => {
            return Token {
                type : TokenType.At,
                value : view("@"),
                position : position
            }
        }
        '!' => {
            var view = provider.read_alpha(str);
            if(strncmp(view.data(), "important", view.size()) == 0) {
                return Token {
                    type : TokenType.Important,
                    value : view("!important"),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view(""),
                    position : position
                }
            }
        }
        '@' => {
            return Token {
                type : TokenType.At,
                value : view("@"),
                position : position
            }
        }
        '*' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.ContainsSubstr,
                    value : view("*="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Multiply,
                    value : view("*"),
                    position : position
                }
            }
        }
        '^' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.StartsWith,
                    value : view("^="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view(""),
                    position : position
                }
            }
        }
        '$' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.EndsWith,
                    value : view("$="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view(""),
                    position : position
                }
            }
        }
        '|' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.DashSeparatedMatch,
                    value : view("|="),
                    position : position
                }
            }  else {
                return Token {
                    type : TokenType.Unexpected,
                    value : view(""),
                    position : position
                }
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
        '\'' => {
            str.append('\'');
            return Token {
                type : TokenType.SingleQuotedValue,
                value : provider.read_single_quoted_value(lexer.str),
                position : position
            }
        }
        '~' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.ContainsWord,
                    value : view("~="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.GeneralSibling,
                    value : view("~"),
                    position : position
                }
            }
        }
        '>' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.GreaterThanOrEqual,
                    value : view(">="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.GreaterThan,
                    value : view(">"),
                    position : position
                }
            }
        }
        '<' => {
            if(provider.peek() == '=') {
                provider.readCharacter();
                return Token {
                    type : TokenType.LessThanOrEqual,
                    value : view("<="),
                    position : position
                }
            } else {
                return Token {
                    type : TokenType.LessThan,
                    value : view("<"),
                    position : position
                }
            }
        }
        '/' => {
            return Token {
                type : TokenType.Divide,
                value : view("/"),
                position : position
            }
        }
        '=' => {
            return Token {
                type : TokenType.Equal,
                value : view("="),
                position : position
            }
        }
        '}' => {
            if(css.lb_count == 1) {
                printf("exiting, didn't expect an rb\n");
                css.reset();
                lexer.unsetUserLexer();
            } else {
                css.lb_count--;
                printf("lb_count decreased to %d\n", css.lb_count);
            }
            return Token {
                type : TokenType.RBrace,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            // here always lb_count > 0
            if(css.lb_count == 1) {
                printf("turning on chemical mode\n");
                css.other_mode = true;
                css.chemical_mode = true;
            }
            css.lb_count++;
            printf("lb_count increased to %d\n", css.lb_count);
            return Token {
                type : TokenType.LBrace,
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
                    type : TokenType.Number,
                    value : str.finalize_view(),
                    position : position
                }
            } else {
                printf("starting text with character : %d\n", c);
                str.append(c);
                var text = provider.read_text(lexer.str)
                if(text.size() != 0) {
                    return Token {
                        type : TokenType.Identifier,
                        value : text,
                        position : position
                    }
                }
            }
        }
    }
}