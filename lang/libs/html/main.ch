import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "@compiler/CBIInfo.ch"
import "./TokenType.ch"
import "@cstd/ctype.ch"

using namespace std;

struct HtmlLexer {

    var has_lt : bool

}

public func getCBIInfo() : CBIInfo {
    return CBIInfo {
        is_initialize_lexer : true,
        is_parse_macro_value : true
    }
}

public func initializeLexer(lexer : *Lexer) : *HtmlLexer {
    var ptr = lexer.fileAllocator.allocate_size(#sizeof(HtmlLexer), #alignof(HtmlLexer));
    return (ptr as *HtmlLexer)
}

func (provider : &SourceProvider) read_tag_name(str : &SerialStrAllocator) : std::string_view {
    while(true) {
        const c = provider.peek();
        if(isalnum(c as int) || c == '_' || c == '-' || c == ':') {
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
        if(c != '<') {
            str.append(provider.readCharacter());
        } else {
            break;
        }
    }
    return str.finalize_view();
}

@comptime
func view(str : literal<string>) : std::string_view {
    return std::string_view(str);
}

public func getNextToken(html : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    const provider = &lexer.provider;
    const str = &lexer.str;
    // the position of the current symbol
    const position = provider.getPosition();
    const c = provider.readCharacter();
    switch(c) {
        '<' => {
            html.has_lt = true;
            return Token {
                type : TokenType.LT,
                value : view("<"),
                position : position
            }
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