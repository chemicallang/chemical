public namespace docgen {

enum ChemTokenType {
    Eof,
    Text,
    Keyword,
    String,
    Number,
    Comment,
    Punctuation,
    Operator,
    Type,
    Function,
    Macro
}

struct ChemLexer {
    var source : std::string_view
    var cursor : uint
}

func is_chem_whitespace(c : char) : bool {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

func is_chem_alpha(c : char) : bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

func is_chem_digit(c : char) : bool {
    return c >= '0' && c <= '9';
}

func is_chem_alphanum(c : char) : bool {
    return is_chem_alpha(c) || is_chem_digit(c);
}

func (lexer : &mut ChemLexer) advance() : char {
    if(lexer.cursor >= lexer.source.size()) return '\0';
    const c = lexer.source.data()[lexer.cursor];
    lexer.cursor++;
    return c;
}

func (lexer : &mut ChemLexer) peek() : char {
    if(lexer.cursor >= lexer.source.size()) return '\0';
    return lexer.source.data()[lexer.cursor];
}

func (lexer : &mut ChemLexer) peek_next() : char {
    if(lexer.cursor + 1 >= lexer.source.size()) return '\0';
    return lexer.source.data()[lexer.cursor + 1];
}

// Simple keyword set
func is_keyword(s : std::string_view) : bool {
    if(s.equals("func")) return true;
    if(s.equals("var")) return true;
    if(s.equals("const")) return true;
    if(s.equals("struct")) return true;
    if(s.equals("enum")) return true;
    if(s.equals("namespace")) return true;
    if(s.equals("public")) return true;
    if(s.equals("private")) return true;
    if(s.equals("if")) return true;
    if(s.equals("else")) return true;
    if(s.equals("while")) return true;
    if(s.equals("for")) return true;
    if(s.equals("return")) return true;
    if(s.equals("break")) return true;
    if(s.equals("continue")) return true;
    if(s.equals("switch")) return true;
    if(s.equals("case")) return true;
    if(s.equals("default")) return true;
    if(s.equals("import")) return true;
    if(s.equals("using")) return true;
    if(s.equals("as")) return true;
    if(s.equals("in")) return true;
    if(s.equals("true")) return true;
    if(s.equals("false")) return true;
    if(s.equals("null")) return true;
    if(s.equals("defer")) return true;
    if(s.equals("unsafe")) return true;
    if(s.equals("impl")) return true;
    if(s.equals("interface")) return true;
    if(s.equals("union")) return true;
    if(s.equals("bitfield")) return true;
    if(s.equals("comptime")) return true;
    if(s.equals("type")) return true;
    if(s.equals("extend")) return true;
    if(s.equals("trait")) return true;
    return false;
}

func highlight_chemical(code : std::string_view) : std::string {
    var lexer = ChemLexer { source : code, cursor : 0 }
    var html = std::string();
    
    while(lexer.cursor < lexer.source.size()) {
        const start = lexer.cursor;
        const c = lexer.peek();
        
        if(is_chem_whitespace(c)) {
            html.append(lexer.advance());
        } else if(c == '/') {
             if(lexer.peek_next() == '/') {
                 // Line comment
                 html.append_view("<span class=\"chem-comment\">");
                 while(lexer.cursor < lexer.source.size() && lexer.peek() != '\n') {
                     var ch = lexer.advance();
                     if(ch == '<') html.append_view("&lt;");
                     else if(ch == '>') html.append_view("&gt;");
                     else if(ch == '&') html.append_view("&amp;");
                     else html.append(ch);
                 }
                 html.append_view("</span>");
             } else if(lexer.peek_next() == '*') {
                 // Block comment
                 html.append_view("<span class=\"chem-comment\">");
                 lexer.advance(); // /
                 lexer.advance(); // *
                 html.append_view("/*");
                 while(lexer.cursor < lexer.source.size()) {
                     if(lexer.peek() == '*' && lexer.peek_next() == '/') {
                         lexer.advance();
                         lexer.advance();
                         html.append_view("*/");
                         break;
                     }
                     var ch = lexer.advance();
                     if(ch == '<') html.append_view("&lt;");
                     else if(ch == '>') html.append_view("&gt;");
                     else if(ch == '&') html.append_view("&amp;");
                     else html.append(ch);
                 }
                 html.append_view("</span>");
             } else {
                 html.append(lexer.advance()); // just a slash
             }
        } else if(c == '"' || c == '\'') {
            // String
            const quote = lexer.advance();
            html.append_view("<span class=\"chem-str\">");
            html.append(quote);
            var escaped = false;
            while(lexer.cursor < lexer.source.size()) {
                const ch = lexer.peek();
                if(escaped) {
                    html.append(lexer.advance());
                    escaped = false;
                } else if(ch == '\\') {
                    html.append(lexer.advance());
                    escaped = true;
                } else if(ch == quote) {
                    html.append(lexer.advance());
                    break;
                } else {
                    var x = lexer.advance();
                    if(x == '<') html.append_view("&lt;");
                    else if(x == '>') html.append_view("&gt;");
                    else if(x == '&') html.append_view("&amp;");
                    else html.append(x);
                }
            }
            html.append_view("</span>");
        } else if(is_chem_digit(c)) {
            // Number
            html.append_view("<span class=\"chem-num\">");
            while(lexer.cursor < lexer.source.size() && (is_chem_alphanum(lexer.peek()) || lexer.peek() == '.')) {
                 html.append(lexer.advance());
            }
             html.append_view("</span>");
        } else if(is_chem_alpha(c)) {
            // Identifier or Keyword
            var start_idx = lexer.cursor;
            while(lexer.cursor < lexer.source.size() && is_chem_alphanum(lexer.peek())) {
                lexer.advance();
            }
            var len = lexer.cursor - start_idx;
            var text = std::string_view(lexer.source.data() + start_idx, len);
            
            if(is_keyword(text)) {
                html.append_view("<span class=\"chem-kwd\">");
                html.append_view(text);
                html.append_view("</span>");
            } else if(lexer.peek() == '(') {
                 // Function call heuristic
                html.append_view("<span class=\"chem-func\">");
                html.append_view(text);
                html.append_view("</span>");
            } else {
                 // Just text
                 html.append_view(text);
            }
        } else if(c == '@') {
             // Attribute or Macro
             html.append_view("<span class=\"chem-macro\">");
             html.append(lexer.advance()); // @
             while(lexer.cursor < lexer.source.size() && is_chem_alphanum(lexer.peek())) {
                 html.append(lexer.advance());
             }
             html.append_view("</span>");
        } else {
             // Punctuation / Operator
             var ch = lexer.advance();
             if(ch == '<') html.append_view("&lt;");
             else if(ch == '>') html.append_view("&gt;");
             else if(ch == '&') html.append_view("&amp;");
             else html.append(ch);
        }
    }
    
    return html;
}

}
