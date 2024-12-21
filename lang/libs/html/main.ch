import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "./TokenType.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@cstd/ctype.ch"

using namespace std;

struct HtmlLexer {

    var has_starting_lb : bool

    var has_lt : bool

    var has_lb : bool

}

public func parseMacroValue(parser : *mut Parser, builder : *mut ASTBuilder) : *mut Value {
    printf("wow create macro\n");
    const loc = compiler::get_raw_location();
    return builder.make_int_value(10, loc);
}

func (provider : &SourceProvider) read_tag_name(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && (isalnum(c as int) || c == '_' || c == '-' || c == ':')) {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) read_text(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(c != -1 && c != '<' && c != '{') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

func (provider : &SourceProvider) skip_whitespaces() {
    while(true) {
        const c = provider.peek();
        switch(c) {
            ' ', '\t', '\n', '\r' => {
                provider.readCharacter();
            }
            default => {
                return;
            }
        }
    }
}

@comptime
func view(str : literal<string>) : std::string_view {
    return std::string_view(str);
}

public func getNextToken2(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
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
                type : TokenType.LT,
                value : view("<"),
                position : position
            }
        }
        '}' => {
            printf("found an rb\n");
            if(html.has_lb) {
                html.has_lb = false;
            } else {
                printf("exiting, didn't expect an rb\n");
                lexer.user_mode = false;
                lexer.other_mode = false;
            }
            return Token {
                type : TokenType.RB,
                value : view("}"),
                position : position
            }
        }
        '{' => {
            if(html.has_starting_lb) {
                printf("found the starting lb\n");
                html.has_starting_lb = false;
            } else {
                html.has_lb = true;
            }
            return Token {
                type : TokenType.LB,
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
                    type : TokenType.GT,
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
                    return Token {
                        type : TokenType.Unexpected,
                        value : view("tag names must start with letters"),
                        position : position
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

public func getNextToken(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    const t = getNextToken2(html, lexer);
    printf("I created token : '%s' with type %d\n", t.value.data(), t.type);
    return t;
}

public func initializeLexer(lexer : *mut Lexer) {
    const file_allocator = lexer.getFileAllocator();
    const ptr = file_allocator.allocate_size(#sizeof(HtmlLexer), #alignof(HtmlLexer)) as *mut HtmlLexer;
    const x = new (ptr) HtmlLexer {
        has_starting_lb : true,
        has_lt : false,
        has_lb : false
    }
    lexer.other_mode = true;
    lexer.user_mode = true;
    lexer.user_lexer = UserLexerFn {
        instance : ptr,
        subroutine : getNextToken
    }
}