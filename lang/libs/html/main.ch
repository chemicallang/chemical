import "@compiler/Token.ch"
import "@compiler/Lexer.ch"
import "@compiler/CBIInfo.ch"
import "@cstd/ctype.ch"

using namespace std;

struct HtmlLexer {

    var has_lt : bool

}

pubic func getCBIInfo() : CBIInfo {
    return CBIInfo {
        is_initialize_lexer : true,
        is_parse_macro_value : true
    }
}

public func initializeLexer(lexer : *Lexer) : *HtmlLexer {
    var ptr = lexer.fileAllocator.allocate_size(#sizeof(HtmlLexer), #alignof(HtmlLexer));
    return (ptr as *HtmlLexer)
}

func (provider : &SourceProvider) read_tag_name() : string {
    var str = string();
    while(true) {
        const c = provider.peek();
        if(isalnum(c as int) || c == '_' || c == '-' || c == ':') {
            str.append(c);
        } else {
            break;
        }
    }
    return str;
}

public func func getNextToken(state : &mut HtmlLexer, lexer : &mut Lexer) : Token {
    var c = html.provider.peek();
    switch(c) {
        '<' => {
            html.has_lt = true;
            html.provider.readCharacter();
            html.lexer.lexOperatorToken('<');
        }
        '/' => {
            if(html.has_lt) {
                html.provider.readCharacter();
                html.lexer.lexOperatorToken('/');
            } else {
                // TODO diagnostic, lt is not open
            }
        }
        '>' => {
            if(html.has_lt) {
                html.has_lt = false;
                html.lexer.lexOperatorToken('>');
            } else {
                // TODO diagnostic, lt is not open, what is '>' doing here
            }
        }
        default => {
            if(html.has_lt) {
                if(isalpha(c as int)) {
                    const tag_name = html.provider.read_tag_name();
                    html.put_token(tag_name, LexTokenType.Keyword);
                } else {
                    // TODO diagnostic, tag names must start with letters
                }
            } else {
                var text = string();
                html.provider.readUntil(&text, '<');
                if(text.size() != 0) {
                    html.put_token(text, LexTokenType.RawToken);
                }
            }
        }
    }
}