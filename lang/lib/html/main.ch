import "../compiler/Lexer.ch"
import "../compiler/CSTConverter.ch"
import "@system/ctype.h"

struct HtmlLexer {

    var lexer : *Lexer

    var provider : *SourceProvider

    var has_lt : bool

}

func (html : &HtmlLexer) put_token(value : &string, type : LexTokenType) : *CSTToken {
    return html.lexer.put(value, type, html.provider.getLineNumber(), html.provider.getLineCharNumber());
}

func (provider : &SourceProvider) read_tag_name() : string {
    var str = string();
    while(true) {
        const c = provider.peek();
        if(isalnum(c) || c == '_' || c == '-' || c == ':') {
            str.append(c);
        } else {
            break;
        }
    }
    return str;
}

func (html : &HtmlLexer) put_next_token() {
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
                if(isalpha(c)) {
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

public func lexMacro(lexer : *Lexer) {
    lexer.lexNumberToken();
}

public func parseMacro(converter : *CSTConverter, token : *CSTToken) {
    const contained = token.tokens();
    const interested = contained.get(1);
    const value = converter.make_uint_value(33, token);
    converter.put_value(value, interested);
}