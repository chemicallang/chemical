public namespace docgen {

enum TokenType {
    Eof,
    Text,
    Keyword,
    String,
    Number,
    Comment,
    Punctuation,
    Operator,
    Function,
    Type,
    Macro,
    Tag,
    Attribute,
    Variable
}

struct SyntaxLexer {
    var source : std::string_view
    var cursor : uint
}

func is_whitespace(c : char) : bool {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

func is_alpha(c : char) : bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

func is_digit(c : char) : bool {
    return c >= '0' && c <= '9';
}

func is_alphanum(c : char) : bool {
    return is_alpha(c) || is_digit(c);
}

func (lexer : &mut SyntaxLexer) advance() : char {
    if(lexer.cursor >= lexer.source.size()) return '\0';
    const c = lexer.source.data()[lexer.cursor];
    lexer.cursor++;
    return c;
}

func (lexer : &mut SyntaxLexer) peek() : char {
    if(lexer.cursor >= lexer.source.size()) return '\0';
    return lexer.source.data()[lexer.cursor];
}

func (lexer : &mut SyntaxLexer) peek_next() : char {
    if(lexer.cursor + 1 >= lexer.source.size()) return '\0';
    return lexer.source.data()[lexer.cursor + 1];
}

func (lexer : &mut SyntaxLexer) match(s : std::string_view) : bool {
    if(lexer.cursor + s.size() > lexer.source.size()) return false;
    var i = 0u;
    while(i < s.size()) {
        if(lexer.source.data()[lexer.cursor + i] != s.data()[i]) return false;
        i++;
    }
    return true;
}

func make_span(cls : std::string_view, content : std::string_view) : std::string {
    var s = std::string("<span class=\"");
    s.append_view(cls);
    s.append_view("\">");
    
    var i = 0u;
    while(i < content.size()) {
        const c = content.data()[i];
        if(c == '<') s.append_view("&lt;");
        else if(c == '>') s.append_view("&gt;");
        else if(c == '&') s.append_view("&amp;");
        else s.append(c);
        i++;
    }
    s.append_view("</span>");
    return s;
}

func escape_html(content : std::string_view) : std::string {
    var s = std::string();
    var i = 0u;
    while(i < content.size()) {
        const c = content.data()[i];
        if(c == '<') s.append_view("&lt;");
        else if(c == '>') s.append_view("&gt;");
        else if(c == '&') s.append_view("&amp;");
        else s.append(c);
        i++;
    }
    return s;
}

// ----------------------------------------------------------------------
// Generic C-Like Highlighter
// Supports: Chemical, C, C++, JS, Java, etc.
// ----------------------------------------------------------------------

struct ClikeConfig {
    var keywords : &std::vector<std::string>
    var types : &std::vector<std::string>
}

func highlight_clike(code : std::string_view, config : *ClikeConfig) : std::string {
    var lexer = SyntaxLexer { source : code, cursor : 0 }
    var html = std::string();
    
    while(lexer.cursor < lexer.source.size()) {
        const start = lexer.cursor;
        const c = lexer.peek();
        
        if(is_whitespace(c)) {
            html.append(lexer.advance());
        } else if(c == '/') {
             if(lexer.peek_next() == '/') {
                 // Line comment
                 var startc = lexer.cursor;
                 while(lexer.cursor < lexer.source.size() && lexer.peek() != '\n') {
                     lexer.advance();
                 }
                 html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
             } else if(lexer.peek_next() == '*') {
                 // Block comment
                 var startc = lexer.cursor;
                 lexer.advance(); lexer.advance();
                 while(lexer.cursor < lexer.source.size()) {
                     if(lexer.peek() == '*' && lexer.peek_next() == '/') {
                         lexer.advance(); lexer.advance();
                         break;
                     }
                     lexer.advance();
                 }
                 html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
             } else {
                 html.append(lexer.advance());
             }
        } else if(c == '"' || c == '\'') {
            // String
            const quote = lexer.advance();
            var startc = start;
            var escaped = false;
            while(lexer.cursor < lexer.source.size()) {
                const ch = lexer.peek();
                if(escaped) {
                    lexer.advance();
                    escaped = false;
                } else if(ch == '\\') {
                    lexer.advance();
                    escaped = true;
                } else if(ch == quote) {
                    lexer.advance();
                    break;
                } else {
                    lexer.advance();
                }
            }
            html.append_view(make_span("tok-str", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else if(is_digit(c)) {
            // Number
            var startc = start;
            while(lexer.cursor < lexer.source.size() && (is_alphanum(lexer.peek()) || lexer.peek() == '.')) {
                 lexer.advance();
            }
            html.append_view(make_span("tok-num", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else if(is_alpha(c)) {
            // Identifier
            var startc = start;
            while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) {
                lexer.advance();
            }
            var len = lexer.cursor - startc;
            var text = std::string_view(lexer.source.data() + startc, len);
            
            var is_kwd = false;
            var i = 0u;
            while(i < config.keywords.size()) {
                if(config.keywords.get(i).equals_view(text)) { is_kwd = true; break; }
                i++;
            }
            
            if(is_kwd) {
                html.append_view(make_span("tok-kwd", text).to_view());
            } else {
                var is_type = false;
                 i = 0u;
                while(i < config.types.size()) {
                    if(config.types.get(i).equals_view(text)) { is_type = true; break; }
                     i++;
                }
                
                if(is_type) {
                     html.append_view(make_span("tok-type", text).to_view());
                } else if(lexer.peek() == '(') {
                    html.append_view(make_span("tok-fn", text).to_view());
                } else {
                    html.append_view(text);
                }
            }
        } else {
             html.append_view(escape_html(std::string_view(lexer.source.data() + lexer.cursor, 1)).to_view());
             lexer.advance();
        }
    }
    return html;
}

// ----------------------------------------------------------------------
// Specific Highlighters
// ----------------------------------------------------------------------

func highlight_chemical(code : std::string_view) : std::string {
    var kwds = std::vector<std::string>();
    // From Lexer.cpp
    const list = "func|var|const|struct|enum|namespace|public|private|if|else|while|for|return|break|continue|switch|case|default|import|using|as|in|true|false|null|defer|unsafe|impl|interface|union|bitfield|comptime|type|extend|trait|mut|self|Self|this|is|dyn|loop|new|destruct|dealloc|delete|provide|init|try|catch|throw|from|do|sizeof|alignof|protected|internal|any|void|alias|variant";
    
    // Split macro
    var i = 0u; var start = 0u;
    var sv = std::string_view(list);
    while(i < sv.size()){
        if(sv.data()[i] == '|') {
            kwds.push_back(std::string(std::string_view(sv.data() + start, i - start)));
            start = i + 1u;
        }
        i++;
    }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));
    
    var types = std::vector<std::string>();
    const tlist = "i8|i16|i32|i64|u8|u16|u32|u64|f32|f64|bool|char|int|long|float|double|uint|ulong|short|ushort|uchar";
    i = 0; start = 0; sv = std::string_view(tlist);
    while(i < sv.size()){
        if(sv.data()[i] == '|') {
            types.push_back(std::string(std::string_view(sv.data() + start, i - start)));
            start = i + 1u;
        }
        i++;
    }
    types.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));

    var config = ClikeConfig { keywords : kwds, types : types };
    
    // Custom wrapper to handle attributes @foo
    var lexer = SyntaxLexer { source : code, cursor : 0 }
    var html = std::string();
    
    // We reuse generic logic but intercept @
    while(lexer.cursor < lexer.source.size()) {
        if(lexer.peek() == '@') {
             var startc = lexer.cursor;
             lexer.advance();
             while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) {
                 lexer.advance();
             }
             html.append_view(make_span("tok-macro", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else {
            // Consume one token via standard logic (hacky partial consume)
            // Ideally we'd factor out "next token" but for now let's just use Clike
            // Actually, we can just run Clike on chunks? No, context.
            // Let's just reimplement the loop here calling helper for chunks?
            // Easier: just let Clike handle it and post-process? No.
            // Let's just copy-paste the Clike loop here structure since Chemical has specifics.
            
            const start = lexer.cursor;
            const c = lexer.peek();
             if(is_whitespace(c)) {
                html.append(lexer.advance());
             } else if(c == '/') {
                 // Comments (same as Clike)
                 if(lexer.peek_next() == '/') {
                     var startc = lexer.cursor;
                     while(lexer.cursor < lexer.source.size() && lexer.peek() != '\n') { lexer.advance() };
                     html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
                 } else if(lexer.peek_next() == '*') {
                     var startc = lexer.cursor;
                     lexer.advance(); lexer.advance();
                     while(lexer.cursor < lexer.source.size()) {
                         if(lexer.peek() == '*' && lexer.peek_next() == '/') { lexer.advance(); lexer.advance(); break; }
                         lexer.advance();
                     }
                     html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
                 } else { html.append(lexer.advance()); }
             } else if(c == '"' || c == '\'') {
                 // Strings (same as Clike)
                 const quote = lexer.advance();
                 var startc = start;
                 var escaped = false;
                 while(lexer.cursor < lexer.source.size()) {
                     const ch = lexer.peek();
                     if(escaped) { lexer.advance(); escaped = false;
                     } else if(ch == '\\') { lexer.advance(); escaped = true;
                     } else if(ch == quote) { lexer.advance(); break;
                     } else { lexer.advance(); }
                 }
                 html.append_view(make_span("tok-str", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
             } else if(is_digit(c)) {
                 var startc = start;
                 while(lexer.cursor < lexer.source.size() && (is_alphanum(lexer.peek()) || lexer.peek() == '.')) { lexer.advance() };
                 html.append_view(make_span("tok-num", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
             } else if(is_alpha(c)) {
                 var startc = start;
                 while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) { lexer.advance() };
                 var text = std::string_view(lexer.source.data() + startc, lexer.cursor - startc);
                 
                 var is_kwd = false;
                 var i = 0u; while(i < kwds.size()) { if(kwds.get(i).equals_view(text)) { is_kwd = true; break; } i++; }
                 
                 if(is_kwd) html.append_view(make_span("tok-kwd", text).to_view());
                 else {
                     var is_type = false;
                     i = 0; while(i < types.size()) { if(types.get(i).equals_view(text)) { is_type = true; break; } i++; }
                     if(is_type) html.append_view(make_span("tok-type", text).to_view());
                     else if(lexer.peek() == '(') html.append_view(make_span("tok-fn", text).to_view());
                     else html.append_view(text);
                 }
             } else {
                 html.append_view(escape_html(std::string_view(lexer.source.data() + lexer.cursor, 1)).to_view());
                 lexer.advance();
             }
        }
    }
    return html;
}

func highlight_chmod(code : std::string_view) : std::string {
    var kwds = std::vector<std::string>();
    const list = "module|source|link|import|interface|var|const";
    var i = 0u; var start = 0u; var sv = std::string_view(list);
    while(i < sv.size()){ if(sv.data()[i] == '|') { kwds.push_back(std::string(std::string_view(sv.data() + start, i - start))); start = i + 1u; } i++; }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));
    
    var config = ClikeConfig { keywords : kwds, types : std::vector<std::string>() };
    return highlight_clike(code, &config);
}

func highlight_c(code : std::string_view) : std::string {
    var kwds = std::vector<std::string>();
    const list = "auto|break|case|char|const|continue|default|do|double|else|enum|extern|float|for|goto|if|int|long|register|return|short|signed|sizeof|static|struct|switch|typedef|union|unsigned|void|volatile|while";
    var i = 0u; var start = 0u; var sv = std::string_view(list);
    while(i < sv.size()){ if(sv.data()[i] == '|') { kwds.push_back(std::string(std::string_view(sv.data() + start, i - start))); start = i + 1u; } i++; }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));
    
    var config = ClikeConfig { keywords : kwds, types : std::vector<std::string>() };
    return highlight_clike(code, &config);
}

func highlight_cpp(code : std::string_view) : std::string {
    var kwds = std::vector<std::string>();
    const list = "alignas|alignof|and|and_eq|asm|auto|bitand|bitor|bool|break|case|catch|char|char16_t|char32_t|class|compl|const|constexpr|const_cast|continue|decltype|default|delete|do|double|dynamic_cast|else|enum|explicit|export|extern|false|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|not|not_eq|nullptr|operator|or|or_eq|private|protected|public|register|reinterpret_cast|return|short|signed|sizeof|static|static_assert|static_cast|struct|switch|template|this|thread_local|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|wchar_t|while|xor|xor_eq";
    var i = 0u; var start = 0u; var sv = std::string_view(list);
    while(i < sv.size()){ if(sv.data()[i] == '|') { kwds.push_back(std::string(std::string_view(sv.data() + start, i - start))); start = i + 1u; } i++; }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));
    
    var config = ClikeConfig { keywords : kwds, types : std::vector<std::string>() };
    return highlight_clike(code, &config);
}

func highlight_js(code : std::string_view) : std::string {
    var kwds = std::vector<std::string>();
    const list = "break|case|catch|class|const|continue|debugger|default|delete|do|else|export|extends|false|finally|for|function|if|import|in|instanceof|new|null|return|super|switch|this|throw|true|try|typeof|var|void|while|with|let|static|yield|await|async";
    var i = 0u; var start = 0u; var sv = std::string_view(list);
    while(i < sv.size()){ if(sv.data()[i] == '|') { kwds.push_back(std::string(std::string_view(sv.data() + start, i - start))); start = i + 1u; } i++; }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));
    
    var config = ClikeConfig { keywords : kwds, types : std::vector<std::string>() };
    return highlight_clike(code, &config);
}

func highlight_bash(code : std::string_view) : std::string {
    var lexer = SyntaxLexer { source : code, cursor : 0 }
    var html = std::string();
    
    // Simple keywords
    var kwds = std::vector<std::string>();
    const list = "if|then|else|elif|fi|case|esac|for|select|while|until|do|done|in|function|time|coproc";
     var i = 0u; var start = 0u; var sv = std::string_view(list);
    while(i < sv.size()){ if(sv.data()[i] == '|') { kwds.push_back(std::string(std::string_view(sv.data() + start, i - start))); start = i + 1u; } i++; }
    kwds.push_back(std::string(std::string_view(sv.data() + start, sv.size() - start)));

    while(lexer.cursor < lexer.source.size()) {
        const startc = lexer.cursor;
        const c = lexer.peek();
        
        if(is_whitespace(c)) {
            html.append(lexer.advance());
        } else if(c == '#') {
             // Comment
             while(lexer.cursor < lexer.source.size() && lexer.peek() != '\n') { lexer.advance(); }
             html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else if(c == '"' || c == '\'') {
             // String
             const quote = lexer.advance();
             var escaped = false;
             while(lexer.cursor < lexer.source.size()) {
                 const ch = lexer.peek();
                 if(escaped) { lexer.advance(); escaped = false;
                 } else if(ch == '\\') { lexer.advance(); escaped = true;
                 } else if(ch == quote) { lexer.advance(); break;
                 } else { lexer.advance(); }
             }
             html.append_view(make_span("tok-str", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else if(is_alpha(c)) {
             while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) { lexer.advance(); }
             var text = std::string_view(lexer.source.data() + startc, lexer.cursor - startc);
             var is_kwd = false;
             i = 0; while(i < kwds.size()) { if(kwds.get(i).equals_view(text)) { is_kwd = true; break; } i++; }
             if(is_kwd) html.append_view(make_span("tok-kwd", text).to_view());
             else html.append_view(text);
        } else if(c == '$') {
             lexer.advance();
             while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) { lexer.advance(); }
             html.append_view(make_span("tok-var", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else {
             html.append_view(escape_html(std::string_view(lexer.source.data() + lexer.cursor, 1)).to_view());
             lexer.advance();
        }
    }
    return html;
}

func highlight_html(code : std::string_view) : std::string {
    var lexer = SyntaxLexer { source : code, cursor : 0 }
    var html = std::string();
    
    while(lexer.cursor < lexer.source.size()) {
        const startc = lexer.cursor;
        const c = lexer.peek();
        
        if(c == '<') {
            if(lexer.match("<!--")) {
                // Comment
                lexer.advance(); lexer.advance(); lexer.advance(); lexer.advance();
                while(lexer.cursor < lexer.source.size()) {
                    if(lexer.match("-->")) { lexer.advance(); lexer.advance(); lexer.advance(); break; }
                     lexer.advance();
                }
                html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
            } else {
                // Tag
                html.append_view("&lt;");
                lexer.advance();
                if(lexer.peek() == '/') { html.append('/'); lexer.advance(); }
                
                var tstart = lexer.cursor;
                while(lexer.cursor < lexer.source.size() && is_alphanum(lexer.peek())) { lexer.advance(); }
                html.append_view(make_span("tok-tag", std::string_view(lexer.source.data() + tstart, lexer.cursor - tstart)).to_view());
                
                // Attributes
                while(lexer.cursor < lexer.source.size() && lexer.peek() != '>') {
                    const ac = lexer.peek();
                    if(is_whitespace(ac)) { html.append(lexer.advance()); }
                    else if(is_alpha(ac)) {
                        var astart = lexer.cursor;
                        while(lexer.cursor < lexer.source.size() && (is_alphanum(lexer.peek()) || lexer.peek() == '-')) { lexer.advance(); }
                        html.append_view(make_span("tok-attr", std::string_view(lexer.source.data() + astart, lexer.cursor - astart)).to_view());
                    } else if(ac == '"') {
                         const quote = lexer.advance();
                         var sstart = lexer.cursor - 1;
                         while(lexer.cursor < lexer.source.size() && lexer.peek() != '"') { lexer.advance(); }
                         if(lexer.peek() == '"') lexer.advance();
                         html.append_view(make_span("tok-str", std::string_view((lexer.source.data() + sstart) as *char, (lexer.cursor - sstart) as size_t)).to_view());
                    } else {
                        var ch = lexer.advance();
                         if(ch == '<') html.append_view("&lt;");
                         else if(ch == '>') html.append_view("&gt;"); // Should break loop?
                         else if(ch == '&') html.append_view("&amp;");
                         else html.append(ch);
                    }
                }
            }
        } else {
             var ch = lexer.advance();
             if(ch == '<') html.append_view("&lt;");
             else if(ch == '>') html.append_view("&gt;");
             else if(ch == '&') html.append_view("&amp;");
             else html.append(ch);
        }
    }
    return html;
}

func highlight_css(code : std::string_view) : std::string {
    var lexer = SyntaxLexer { source : code, cursor : 0 }
    var html = std::string();
    
    // Very basic: Selectors { Property: Value; }
    // State: 0=Selector, 1=Property, 2=Value
    var state = 0;
    
    while(lexer.cursor < lexer.source.size()) {
        const startc = lexer.cursor;
        const c = lexer.peek();
        
        if(is_whitespace(c)) {
            html.append(lexer.advance());
        } else if(c == '/' && lexer.peek_next() == '*') {
             // Comment
             lexer.advance(); lexer.advance();
             while(lexer.cursor < lexer.source.size()) {
                 if(lexer.peek() == '*' && lexer.peek_next() == '/') { lexer.advance(); lexer.advance(); break; }
                 lexer.advance();
             }
             html.append_view(make_span("tok-com", std::string_view(lexer.source.data() + startc, lexer.cursor - startc)).to_view());
        } else if(c == '{') {
            html.append(lexer.advance());
            state = 1;
        } else if(c == '}') {
            html.append(lexer.advance());
            state = 0;
        } else if(c == ':') {
             html.append(lexer.advance());
             if(state == 1) state = 2;
        } else if(c == ';') {
             html.append(lexer.advance());
             if(state == 2) state = 1;
        } else {
             // Token
             var tstart = lexer.cursor;
             while(lexer.cursor < lexer.source.size() && !is_whitespace(lexer.peek()) && lexer.peek() != '{' && lexer.peek() != '}' && lexer.peek() != ':' && lexer.peek() != ';') {
                 lexer.advance();
             }
             var text = std::string_view(lexer.source.data() + tstart, lexer.cursor - tstart);
             
             if(state == 0) {
                 html.append_view(make_span("tok-fn", text).to_view()); // Selector -> Function color
             } else if(state == 1) {
                 html.append_view(make_span("tok-attr", text).to_view()); // Property -> Attribute color
             } else {
                 html.append_view(make_span("tok-num", text).to_view()); // Value -> Number/String color (simplified)
             }
        }
    }
    return html;
}

}
