public namespace md {

struct TokenParser {
    var tokens : *mut std::vector<MdToken>
    var idx : size_t

    func init(&mut self, tokens : *mut std::vector<MdToken>) {
        self.tokens = tokens
        self.idx = 0
    }

    func get(&self) : MdToken {
        if(self.idx >= self.tokens.size()) {
            return MdToken { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : 0 }
        }
        return self.tokens.get(self.idx)
    }

    func peek_at(&self, off : size_t) : MdToken {
        const i = self.idx + off
        if(i >= self.tokens.size()) {
            return MdToken { type : MdTokenType.EndOfFile as int, value : std::string_view(""), position : 0 }
        }
        return self.tokens.get(i)
    }

    func peek(&self) : MdToken {
        return self.peek_at(1)
    }

    func peek_ahead(&self, off : size_t) : MdToken {
        return self.peek_at(off)
    }

    func bump(&mut self) { if(self.idx < self.tokens.size()) { self.idx += 1; } }
}

func is_end(t : int) : bool { return t == MdTokenType.EndOfFile as int; }
func is_text(t : int) : bool { return t == MdTokenType.Text as int; }
func is_nl(t : int) : bool { return t == MdTokenType.Newline as int; }
func is_line_end(t : int) : bool { return is_nl(t) || is_end(t); }

func is_ws_only(v : std::string_view) : bool {
    var i = 0u;
    while(i < v.size()) {
        const c = v.data()[i];
        if(c != ' ' && c != '\t' && c != '\r') return false;
        i++;
    }
    return true;
}

func skip_blank_lines(p : &mut TokenParser) {
    while(true) {
        const t = p.get();
        if(is_text(t.type) && is_ws_only(t.value)) { p.bump(); continue; }
        if(is_nl(t.type)) { p.bump(); continue; }
        break;
    }
}

func is_list_start(p : &mut TokenParser) : bool {
    const t = p.get().type;
    if(t == MdTokenType.Dash as int || t == MdTokenType.Plus as int || t == MdTokenType.Star as int) {
        const next = p.peek();
        return next.type == MdTokenType.Text as int && next.value.size() > 0 && next.value.data()[0] == ' ';
    }
    return false;
}

func is_ordered_list_start(p : &mut TokenParser) : bool {
    const t = p.get().type;
    if(t == MdTokenType.Number as int) {
        const next = p.peek();
        if(next.type == MdTokenType.Dot as int) {
            const next_next = p.peek_ahead(2);
            return next_next.type == MdTokenType.Text as int && next_next.value.size() > 0 && next_next.value.data()[0] == ' ';
        }
    }
    return false;
}

func is_hr(p : &mut TokenParser) : bool {
    // Check for ---, ***, or ___ on a line
    var i = 0u;
    var dash_count = 0;
    var star_count = 0;
    var underscore_count = 0;
    while(!is_line_end(p.peek_ahead(i).type)) {
        const t = p.peek_ahead(i).type;
        if(t == MdTokenType.Dash as int) dash_count++;
        else if(t == MdTokenType.Star as int) star_count++;
        else if(t == MdTokenType.Underscore as int) underscore_count++;
        else break;
        i++;
    }
    return dash_count >= 3 || star_count >= 3 || underscore_count >= 3;
}

func is_blockquote(p : &mut TokenParser) : bool {
    return p.get().type == MdTokenType.GreaterThan as int;
}

func is_custom_container(p : &mut TokenParser) : bool {
    // Check for ::: type at the beginning of a line (allowing leading whitespace)
    // First, skip any leading whitespace
    var i = 0u;
    while(i < 10 && p.peek_ahead(i).type == MdTokenType.Text as int && is_ws_only(p.peek_ahead(i).value)) {
        i++;
    }
    
    // Check for ::: after whitespace
    if(p.peek_ahead(i).type == MdTokenType.Colon as int) {
        var colon_count = 0;
        var j = i;
        // Look for at least 3 consecutive colons
        while(j < i + 10 && p.peek_ahead(j).type == MdTokenType.Colon as int) {
            colon_count++;
            j++;
        }
        if(colon_count >= 3) {
            // After the colons, there should be text (container type) followed by newline
            while(j < i + 20 && !is_line_end(p.peek_ahead(j).type)) {
                if(p.peek_ahead(j).type == MdTokenType.Text as int && p.peek_ahead(j).value.size() > 0) {
                    return true;
                }
                j++;
            }
        }
    }
    return false;
}

func is_abbreviation_def(p : &mut TokenParser) : bool {
    // Check for pattern: [text]: definition
    if(p.get().type == MdTokenType.LBracket as int) {
        var i = 1u;
        // Look for content inside brackets
        while(i < 20 && p.peek_ahead(i).type != MdTokenType.RBracket as int && !is_line_end(p.peek_ahead(i).type)) {
            if(p.peek_ahead(i).type == MdTokenType.Text as int) {
                i++;
            } else {
                return false;
            }
        }
        if(p.peek_ahead(i).type == MdTokenType.RBracket as int) {
            i++;
            if(p.peek_ahead(i).type == MdTokenType.Colon as int) {
                return true;
            }
        }
    }
    return false;
}
    func is_footnote_def(p : &mut TokenParser) : bool {
    // Check for pattern: [^id]: definition
    if(p.get().type == MdTokenType.LBracket as int) {
        var i = 1u;
        if(p.peek_ahead(i).type == MdTokenType.Caret as int) {
            i++;
            if(p.peek_ahead(i).type == MdTokenType.Text as int) {
                i++;
                if(p.peek_ahead(i).type == MdTokenType.RBracket as int) {
                    i++;
                    if(p.peek_ahead(i).type == MdTokenType.Colon as int) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

func is_definition_list(p : &mut TokenParser) : bool {
    // Check for pattern: Term\n: Definition
    // Be more aggressive - check if we have text followed by newline then colon
    if(is_text(p.get().type) && p.get().value.size() > 0) {
        // Look ahead for newline followed by colon (allowing whitespace)
        var i = 1u;
        while(!is_line_end(p.peek_ahead(i).type) && i < 20) {
            if(is_nl(p.peek_ahead(i).type)) {
                // Check if next non-newline, non-whitespace starts with colon
                var j = i + 1u;
                while(is_nl(p.peek_ahead(j).type)) { j++ };
                // Skip whitespace
                while(j < i + 10 && p.peek_ahead(j).type == MdTokenType.Text as int && is_ws_only(p.peek_ahead(j).value)) { j++ };
                if(p.peek_ahead(j).type == MdTokenType.Colon as int) {
                    return true;
                }
                break;
            }
            i++;
        }
    }
    return false;
}

func is_table_row(p : &mut TokenParser) : bool {
    // DISABLED: Table detection is causing too many false positives
    // The markdown content should not be parsed as tables
    return false;
}

struct MdParser {
    var arena : *mut Arena

    func make_text(&mut self, v : std::string_view) : *mut MdNode {
        var n = self.arena.allocate<MdText>();
        new (n) MdText { base : MdNode { kind : MdNodeKind.Text }, value : v };
        return n as *mut MdNode;
    }

    func parse_inline_text_until_line_end(&mut self, p : &mut TokenParser, out_children : &mut std::vector<*mut MdNode>) {
        while(true) {
            const t = p.get();
            if(is_end(t.type) || is_nl(t.type)) break;

            // Handle inline formatting
            if(t.type == MdTokenType.Star as int || t.type == MdTokenType.Underscore as int) {
                // Could be bold or italic
                const next = p.peek();
                if(next.type == t.type) {
                    // Bold (** or __)
                    p.bump(); p.bump(); // consume both
                    var bold = self.arena.allocate<MdBold>();
                    new (bold) MdBold { base : MdNode { kind : MdNodeKind.Bold }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, t.type, 2, bold.children);
                    out_children.push(bold as *mut MdNode);
                } else {
                    // Italic (* or _)
                    p.bump();
                    var italic = self.arena.allocate<MdItalic>();
                    new (italic) MdItalic { base : MdNode { kind : MdNodeKind.Italic }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, t.type, 1, italic.children);
                    out_children.push(italic as *mut MdNode);
                }
                continue;
            }

            if(t.type == MdTokenType.Tilde as int) {
                // Strikethrough (~~) or subscript
                const next = p.peek();
                if(next.type == MdTokenType.Tilde as int) {
                    // Strikethrough (~~)
                    p.bump(); p.bump();
                    var del = self.arena.allocate<MdStrikethrough>();
                    new (del) MdStrikethrough { base : MdNode { kind : MdNodeKind.Strikethrough }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, MdTokenType.Tilde as int, 2, del.children);
                    out_children.push(del as *mut MdNode);
                } else {
                    // Subscript (~)
                    p.bump();
                    var sub = self.arena.allocate<MdSubscript>();
                    new (sub) MdSubscript { base : MdNode { kind : MdNodeKind.Subscript }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, MdTokenType.Tilde as int, 1, sub.children);
                    out_children.push(sub as *mut MdNode);
                }
                continue;
            }

            if(t.type == MdTokenType.Equal as int) {
                // Mark (==)
                const next = p.peek();
                if(next.type == MdTokenType.Equal as int) {
                    p.bump(); p.bump();
                    var mark = self.arena.allocate<MdMark>();
                    new (mark) MdMark { base : MdNode { kind : MdNodeKind.Mark }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, MdTokenType.Equal as int, 2, mark.children);
                    out_children.push(mark as *mut MdNode);
                    continue;
                }
            }

            if(t.type == MdTokenType.Plus as int) {
                // Insert (++)
                const next = p.peek();
                if(next.type == MdTokenType.Plus as int) {
                    p.bump(); p.bump();
                    var ins = self.arena.allocate<MdInsert>();
                    new (ins) MdInsert { base : MdNode { kind : MdNodeKind.Insert }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, MdTokenType.Plus as int, 2, ins.children);
                    out_children.push(ins as *mut MdNode);
                    continue;
                }
            }

            if(t.type == MdTokenType.Caret as int) {
                // Superscript (^) or footnote reference [^1]
                p.bump();
                if(p.get().type == MdTokenType.LBracket as int) {
                    // Footnote reference [^1]
                    p.bump(); // consume [
                    
                    var footnote_id = self.arena.allocate<std::string>();
                    new (footnote_id) std::string();
                    
                    while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
                        if(is_text(p.get().type)) {
                            footnote_id.append_view(p.get().value);
                        }
                        p.bump();
                    }
                    if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
                    
                    var footnote = self.arena.allocate<MdFootnote>();
                    new (footnote) MdFootnote { 
                        base : MdNode { kind : MdNodeKind.Footnote }, 
                        id : footnote_id.to_view()
                    };
                    out_children.push(footnote as *mut MdNode);
                    continue;
                } else {
                    // Regular superscript
                    var sup = self.arena.allocate<MdSuperscript>();
                    new (sup) MdSuperscript { base : MdNode { kind : MdNodeKind.Superscript }, children : std::vector<*mut MdNode>() };
                    self.parse_inline_until_end_marker(p, MdTokenType.Caret as int, 1, sup.children);
                    out_children.push(sup as *mut MdNode);
                    continue;
                }
            }

            if(t.type == MdTokenType.Backtick as int) {
                // Inline code
                p.bump();
                var code = self.arena.allocate<std::string>();
                new (code) std::string();
                while(p.get().type != MdTokenType.Backtick as int && !is_line_end(p.get().type)) {
                    if(is_text(p.get().type)) {
                        code.append_view(p.get().value);
                    }
                    p.bump();
                }
                if(p.get().type == MdTokenType.Backtick as int) p.bump(); // closing backtick
                
                var code_node = self.arena.allocate<MdInlineCode>();
                new (code_node) MdInlineCode { base : MdNode { kind : MdNodeKind.InlineCode }, value : code.to_view() };
                out_children.push(code_node as *mut MdNode);
                continue;
            }

            if(t.type == MdTokenType.LBracket as int) {
                // Could be a link [text](url) or abbreviation reference
                var link_text = self.arena.allocate<std::string>();
                new (link_text) std::string();
                p.bump(); // consume [
                
                // Parse link text
                while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
                    if(is_text(p.get().type)) {
                        link_text.append_view(p.get().value);
                    }
                    p.bump();
                }
                if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
                
                if(p.get().type == MdTokenType.LParen as int) {
                    // Link [text](url)
                    p.bump(); // consume (
                    
                    var url = self.arena.allocate<std::string>();
                    new (url) std::string();
                    while(p.get().type != MdTokenType.RParen as int && !is_line_end(p.get().type)) {
                        if(is_text(p.get().type)) {
                            url.append_view(p.get().value);
                        } else if(p.get().type == MdTokenType.Colon as int) {
                            url.append_view(p.get().value);
                        }
                        p.bump();
                    }
                    if(p.get().type == MdTokenType.RParen as int) p.bump(); // consume )
                    
                    var link = self.arena.allocate<MdLink>();
                    new (link) MdLink { 
                        base : MdNode { kind : MdNodeKind.Link }, 
                        url : url.to_view(),
                        title : std::string_view(""),
                        children : std::vector<*mut MdNode>()
                    };
                    
                    // Parse link text as inline content
                    var text_node = self.make_text(link_text.to_view());
                    link.children.push(text_node);
                    
                    out_children.push(link as *mut MdNode);
                    continue;
                } else if(p.get().type == MdTokenType.Colon as int) {
                    // Abbreviation definition [text]: definition - this should be handled at block level
                    // For now, treat as text and let block-level parser handle it
                    var text = self.arena.allocate<std::string>();
                    new (text) std::string();
                    text.append('[');
                    text.append_view(link_text.to_view());
                    text.append(']');
                    text.append(':');
                    out_children.push(self.make_text(text.to_view()));
                    continue;
                } else {
                    // Just text in brackets, could be an abbreviation reference
                    // Create abbreviation node that will be resolved later
                    var abbr = self.arena.allocate<MdAbbreviation>();
                    new (abbr) MdAbbreviation { 
                        base : MdNode { kind : MdNodeKind.Abbreviation }, 
                        id : link_text.to_view(),
                        title : std::string_view("")
                    };
                    out_children.push(abbr as *mut MdNode);
                    continue;
                }
            }

            if(t.type == MdTokenType.Exclamation as int && p.peek().type == MdTokenType.LBracket as int) {
                // Image ![alt](url)
                p.bump(); // consume !
                p.bump(); // consume [
                
                var alt = self.arena.allocate<std::string>();
                new (alt) std::string();
                while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
                    if(is_text(p.get().type)) {
                        alt.append_view(p.get().value);
                    }
                    p.bump();
                }
                if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
                
                if(p.get().type == MdTokenType.LParen as int) {
                    p.bump(); // consume (
                    
                    var url = self.arena.allocate<std::string>();
                    new (url) std::string();
                    while(p.get().type != MdTokenType.RParen as int && !is_line_end(p.get().type)) {
                        if(is_text(p.get().type)) {
                            url.append_view(p.get().value);
                        } else if(p.get().type == MdTokenType.Colon as int) {
                            url.append_view(p.get().value);
                        }
                        p.bump();
                    }
                    if(p.get().type == MdTokenType.RParen as int) p.bump(); // consume )
                    
                    var img = self.arena.allocate<MdImage>();
                    new (img) MdImage { 
                        base : MdNode { kind : MdNodeKind.Image }, 
                        url : url.to_view(),
                        alt : alt.to_view(),
                        title : std::string_view("")
                    };
                    
                    out_children.push(img as *mut MdNode);
                    continue;
                }
            }

            // Default: treat as text
            if(is_text(t.type)) {
                out_children.push(self.make_text(t.value));
                p.bump();
            } else {
                // fall back: represent punctuation tokens as text
                out_children.push(self.make_text(t.value));
                p.bump();
            }
        }
    }

    func parse_inline_until_end_marker(&mut self, p : &mut TokenParser, marker_type : int, count : int, out_children : &mut std::vector<*mut MdNode>) {
        while(true) {
            const t = p.get();
            if(is_end(t.type) || is_nl(t.type)) break;
            
            // Check for end marker
            if(t.type == marker_type) {
                if(count == 2) {
                    if(p.peek().type == marker_type) {
                        p.bump(); p.bump(); // consume both
                        break;
                    }
                } else {
                    p.bump(); // consume single
                    break;
                }
            }
            
            // Parse content as text only - no nested inline parsing
            if(is_text(t.type)) {
                out_children.push(self.make_text(t.value));
                p.bump();
            } else {
                // Skip non-text tokens
                p.bump();
            }
        }
    }

    func parse_single_inline_element(&mut self, p : &mut TokenParser, out_children : &mut std::vector<*mut MdNode>) {
        const t = p.get();
        
        // Handle bold/italic
        if(t.type == MdTokenType.Star as int || t.type == MdTokenType.Underscore as int) {
            const next = p.peek();
            if(next.type == t.type) {
                // Bold (** or __)
                p.bump(); p.bump(); // consume both
                var bold = self.arena.allocate<MdBold>();
                new (bold) MdBold { base : MdNode { kind : MdNodeKind.Bold }, children : std::vector<*mut MdNode>() };
                self.parse_inline_until_end_marker(p, t.type, 2, bold.children);
                out_children.push(bold as *mut MdNode);
            } else {
                // Italic (* or _)
                p.bump();
                var italic = self.arena.allocate<MdItalic>();
                new (italic) MdItalic { base : MdNode { kind : MdNodeKind.Italic }, children : std::vector<*mut MdNode>() };
                self.parse_inline_until_end_marker(p, t.type, 1, italic.children);
                out_children.push(italic as *mut MdNode);
            }
            return;
        }
        
        // Handle other inline elements (simplified for now)
        if(t.type == MdTokenType.Tilde as int) {
            const next = p.peek();
            if(next.type == MdTokenType.Tilde as int) {
                p.bump(); p.bump();
                var del = self.arena.allocate<MdStrikethrough>();
                new (del) MdStrikethrough { base : MdNode { kind : MdNodeKind.Strikethrough }, children : std::vector<*mut MdNode>() };
                self.parse_inline_until_end_marker(p, MdTokenType.Tilde as int, 2, del.children);
                out_children.push(del as *mut MdNode);
            } else {
                p.bump();
                var sub = self.arena.allocate<MdSubscript>();
                new (sub) MdSubscript { base : MdNode { kind : MdNodeKind.Subscript }, children : std::vector<*mut MdNode>() };
                self.parse_inline_until_end_marker(p, MdTokenType.Tilde as int, 1, sub.children);
                out_children.push(sub as *mut MdNode);
            }
            return;
        }
        
        // Default: treat as text
        out_children.push(self.make_text(t.value));
        p.bump();
    }

    func parse_header(&mut self, p : &mut TokenParser) : *mut MdNode {
        // count consecutive #
        var level = 0;
        while(p.get().type == MdTokenType.Hash as int) { level += 1; p.bump(); }
        if(level <= 0) level = 1;

        // Skip any whitespace after #
        while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) {
            p.bump();
        }

        var h = self.arena.allocate<MdHeader>();
        new (h) MdHeader { base : MdNode { kind : MdNodeKind.Header }, level : level, children : std::vector<*mut MdNode>() };
        
        // Parse header text with inline formatting
        self.parse_inline_text_until_line_end(p, h.children);
        
        if(is_nl(p.get().type)) p.bump();
        return h as *mut MdNode;
    }

    func parse_fenced_code_block(&mut self, p : &mut TokenParser) : *mut MdNode {
        // current token is FencedCodeStart, value holds language
        const lang = p.get().value;
        p.bump();

        // Allocate the code string in the arena so it lives as long as the AST.
        var code_ptr = self.arena.allocate<std::string>();
        new (code_ptr) std::string();
        while(true) {
            const t = p.get();
            if(t.type == MdTokenType.FencedCodeEnd as int) { p.bump(); break; }
            if(t.type == MdTokenType.CodeContent as int) {
                code_ptr.append_view(t.value);
                code_ptr.append('\n');
                p.bump();
                continue;
            }
            if(is_end(t.type)) break;
            // ignore unexpected tokens
            p.bump();
        }

        var cb = self.arena.allocate<MdCodeBlock>();
        new (cb) MdCodeBlock { base : MdNode { kind : MdNodeKind.CodeBlock }, language : lang, code : code_ptr.to_view() };
        return cb as *mut MdNode;
    }

    func parse_paragraph(&mut self, p : &mut TokenParser) : *mut MdNode {
        var para = self.arena.allocate<MdParagraph>();
        new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
        
        // Parse with inline formatting
        self.parse_inline_text_until_line_end(p, para.children);
        
        if(is_nl(p.get().type)) p.bump();
        return para as *mut MdNode;
    }

    func parse_blockquote(&mut self, p : &mut TokenParser) : *mut MdNode {
        var bq = self.arena.allocate<MdBlockquote>();
        new (bq) MdBlockquote { base : MdNode { kind : MdNodeKind.Blockquote }, children : std::vector<*mut MdNode>() };
        
        while(true) {
            if(p.get().type == MdTokenType.GreaterThan as int) {
                p.bump(); // consume >
                // Skip optional space after >
                if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                    p.bump();
                }
            }
            
            // Parse content with inline formatting
            if(!is_line_end(p.get().type)) {
                var para = self.arena.allocate<MdParagraph>();
                new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
                self.parse_inline_text_until_line_end(p, para.children);
                bq.children.push(para as *mut MdNode);
            }
            
            if(is_nl(p.get().type)) p.bump();
            
            // Continue if next line also starts with >
            if(p.get().type != MdTokenType.GreaterThan as int) break;
        }
        
        return bq as *mut MdNode;
    }

    func parse_custom_container(&mut self, p : &mut TokenParser) : *mut MdNode {
        // Skip leading whitespace before :::
        while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) {
            p.bump();
        }
        
        // Consume ::: (count them)
        var colon_count = 0;
        while(p.get().type == MdTokenType.Colon as int && colon_count < 10) {
            p.bump();
            colon_count++;
        }
        
        // Skip whitespace after :::
        while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) {
            p.bump();
        }
        
        // Parse container type
        var container_type = self.arena.allocate<std::string>();
        new (container_type) std::string();
        while(!is_line_end(p.get().type)) {
            if(is_text(p.get().type)) {
                container_type.append_view(p.get().value);
            }
            p.bump();
        }
        if(is_nl(p.get().type)) p.bump();
        
        var container = self.arena.allocate<MdCustomContainer>();
        new (container) MdCustomContainer { 
            base : MdNode { kind : MdNodeKind.CustomContainer }, 
            type : container_type.to_view(),
            children : std::vector<*mut MdNode>()
        };
        
        // Parse container content until closing :::
        while(true) {
            // Check for closing :::
            if(p.get().type == MdTokenType.Colon as int) {
                var closing_colons = 0u;
                while(p.peek_ahead(closing_colons).type == MdTokenType.Colon as int && closing_colons < 10) {
                    closing_colons++;
                }
                if(closing_colons >= 3) {
                    // Consume the closing colons
                    var i = 0u;
                    while(i < closing_colons) {
                        p.bump();
                        i++;
                    }
                    // Skip newline after closing :::
                    if(is_nl(p.get().type)) p.bump();
                    break;
                }
            }
            
            if(is_end(p.get().type)) break;
            
            // Parse content as paragraph
            skip_blank_lines(p);
            if(is_end(p.get().type) || is_custom_container(p)) break;
            
            var para = self.arena.allocate<MdParagraph>();
            new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
            self.parse_inline_text_until_line_end(p, para.children);
            
            if(para.children.size() > 0) {
                container.children.push(para as *mut MdNode);
            }
            
            if(is_nl(p.get().type)) p.bump();
        }
        
        return container as *mut MdNode;
    }

    func parse_definition_list(&mut self, p : &mut TokenParser) : *mut MdNode {
        // Parse each term/definition pair as separate definition lists
        // This matches the md cbi behavior where each pair gets its own <dl> tag
        
        var first_dl = self.arena.allocate<MdDefinitionList>();
        new (first_dl) MdDefinitionList { base : MdNode { kind : MdNodeKind.DefinitionList }, children : std::vector<*mut MdNode>() };
        
        while(true) {
            if(is_end(p.get().type)) break;
            
            // Parse term
            if(is_text(p.get().type) && p.get().value.size() > 0) {
                var dt = self.arena.allocate<MdDefinitionTerm>();
                new (dt) MdDefinitionTerm { base : MdNode { kind : MdNodeKind.DefinitionTerm }, children : std::vector<*mut MdNode>() };
                
                // Parse term content
                while(!is_line_end(p.get().type)) {
                    const t = p.get();
                    if(is_text(t.type)) {
                        dt.children.push(self.make_text(t.value));
                        p.bump();
                    } else {
                        p.bump();
                    }
                }
                
                first_dl.children.push(dt as *mut MdNode);
                
                // Skip newlines
                while(is_nl(p.get().type)) { p.bump() };
                
                // Parse definition (starts with :)
                if(p.get().type == MdTokenType.Colon as int) {
                    p.bump(); // consume :
                    
                    var dd = self.arena.allocate<MdDefinitionData>();
                    new (dd) MdDefinitionData { base : MdNode { kind : MdNodeKind.DefinitionData }, children : std::vector<*mut MdNode>() };
                    
                    // Skip leading spaces
                    if(is_text(p.get().type) && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                        p.bump();
                    }
                    
                    // Parse definition content
                    while(!is_line_end(p.get().type)) {
                        const t = p.get();
                        if(is_text(t.type)) {
                            dd.children.push(self.make_text(t.value));
                            p.bump();
                        } else {
                            p.bump();
                        }
                    }
                    
                    first_dl.children.push(dd as *mut MdNode);
                }
            }
            
            // Skip newlines between entries
            while(is_nl(p.get().type)) { p.bump() };
            
            // Check if next looks like another definition - if so, return and let main loop handle it
            if(is_definition_list(p)) break;
        }
        
        return first_dl as *mut MdNode;
    }

    func parse_horizontal_rule(&mut self, p : &mut TokenParser) : *mut MdNode {
        // Consume the HR tokens
        while(!is_line_end(p.get().type)) { p.bump(); }
        if(is_nl(p.get().type)) p.bump();
        
        var hr = self.arena.allocate<MdHorizontalRule>();
        new (hr) MdHorizontalRule { base : MdNode { kind : MdNodeKind.Hr } };
        return hr as *mut MdNode;
    }

    func parse_table(&mut self, p : &mut TokenParser) : *mut MdNode {
        var table = self.arena.allocate<MdTable>();
        new (table) MdTable { base : MdNode { kind : MdNodeKind.Table }, alignments : std::vector<MdTableAlign>(), children : std::vector<*mut MdNode>() };
        
        var row_index = 0;
        while(!is_line_end(p.get().type)) {
            var row = self.arena.allocate<MdTableRow>();
            new (row) MdTableRow { base : MdNode { kind : MdNodeKind.TableRow }, is_header : (row_index == 0), children : std::vector<*mut MdNode>() };
            
            // Skip leading pipe if present
            if(p.get().type == MdTokenType.Pipe as int) p.bump();
            
            while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
                var cell = self.arena.allocate<MdTableCell>();
                new (cell) MdTableCell { base : MdNode { kind : MdNodeKind.TableCell }, children : std::vector<*mut MdNode>() };
                
                // Parse cell content
                while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
                    const t = p.get();
                    if(is_text(t.type)) {
                        cell.children.push(self.make_text(t.value));
                        p.bump();
                    } else {
                        p.bump(); // skip non-text tokens for now
                    }
                }
                
                row.children.push(cell as *mut MdNode);
                
                if(p.get().type == MdTokenType.Pipe as int) p.bump();
            }
            
            table.children.push(row as *mut MdNode);
            
            if(is_nl(p.get().type)) p.bump();
            
            // Skip separator line (---|---|---) after first row
            if(row_index == 0 && p.get().type == MdTokenType.Dash as int) {
                while(!is_line_end(p.get().type)) { p.bump(); }
                if(is_nl(p.get().type)) p.bump();
            }
            
            row_index++;
        }
        
        return table as *mut MdNode;
    }

    func parse_list(&mut self, p : &mut TokenParser, ordered : bool) : *mut MdNode {
        var list = self.arena.allocate<MdList>();
        new (list) MdList { base : MdNode { kind : MdNodeKind.List }, ordered : ordered, start : 1, children : std::vector<*mut MdNode>() };
        
        while(true) {
            skip_blank_lines(p);
            if(is_end(p.get().type)) break;
            
            const is_ordered_item = is_ordered_list_start(p);
            const is_unordered_item = is_list_start(p);
            
            if((ordered && !is_ordered_item) || (!ordered && !is_unordered_item)) break;
            
            var item = self.arena.allocate<MdListItem>();
            new (item) MdListItem { base : MdNode { kind : MdNodeKind.ListItem }, children : std::vector<*mut MdNode>() };
            
            // Consume list marker
            if(ordered) {
                p.bump(); // number
                p.bump(); // dot
            } else {
                p.bump(); // -, +, or *
            }
            
            // Skip leading space
            if(is_text(p.get().type) && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                p.bump();
            }
            
            // Parse item content with inline formatting and task checkboxes
            while(!is_line_end(p.get().type)) {
                // Check for task checkbox at start of content
                if(p.get().type == MdTokenType.LBracket as int) {
                    const next = p.peek();
                    if(next.type == MdTokenType.Text as int && next.value.size() == 1 && 
                       (next.value.data()[0] == 'x' || next.value.data()[0] == ' ')) {
                        const is_checked = next.value.data()[0] == 'x';
                        p.bump(); // consume [
                        p.bump(); // consume x or space
                        if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
                        
                        var checkbox = self.arena.allocate<MdTaskCheckbox>();
                        new (checkbox) MdTaskCheckbox { base : MdNode { kind : MdNodeKind.TaskCheckbox }, checked : is_checked };
                        item.children.push(checkbox as *mut MdNode);
                        
                        // Skip space after checkbox
                        if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                            p.bump();
                        }
                        // Don't continue - fall through to parse the text content
                    }
                }
                
                // Check for nested lists
                if(is_list_start(p) || is_ordered_list_start(p)) {
                    var nested_list = self.parse_list(p, is_ordered_list_start(p));
                    item.children.push(nested_list);
                    break;
                }
                
                // Parse inline content directly into list item (no paragraph wrapper)
                self.parse_inline_text_until_line_end(p, item.children);
                break;
            }
            
            if(is_nl(p.get().type)) p.bump();
            list.children.push(item as *mut MdNode);
        }
        
        return list as *mut MdNode;
    }

    func parse_table(&mut self, p : &mut TokenParser) : *mut MdNode {
        var table = self.arena.allocate<MdTable>();
        new (table) MdTable { base : MdNode { kind : MdNodeKind.Table }, alignments : std::vector<MdTableAlign>(), children : std::vector<*mut MdNode>() };
        
        var is_header = true;
        
        while(!is_line_end(p.get().type)) {
            var row = self.arena.allocate<MdTableRow>();
            new (row) MdTableRow { base : MdNode { kind : MdNodeKind.TableRow }, is_header : is_header, children : std::vector<*mut MdNode>() };
            
            // Skip leading pipe if present
            if(p.get().type == MdTokenType.Pipe as int) p.bump();
            
            while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
                var cell = self.arena.allocate<MdTableCell>();
                new (cell) MdTableCell { base : MdNode { kind : MdNodeKind.TableCell }, children : std::vector<*mut MdNode>() };
                
                // Parse cell content
                while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
                    const t = p.get();
                    if(is_text(t.type)) {
                        cell.children.push(self.make_text(t.value));
                        p.bump();
                    } else {
                        p.bump(); // skip non-text tokens for now
                    }
                }
                
                row.children.push(cell as *mut MdNode);
                
                if(p.get().type == MdTokenType.Pipe as int) p.bump();
            }
            
            table.children.push(row as *mut MdNode);
            
            if(is_nl(p.get().type)) p.bump();
            
            // Handle separator line and alignment parsing
            if(is_header) {
                if(p.get().type == MdTokenType.Dash as int) {
                    // Parse alignment row
                    var alignments = std::vector<MdTableAlign>();
                    var parsing_alignment = true;
                    
                    while(parsing_alignment && !is_line_end(p.get().type)) {
                        var align = MdTableAlign.None;
                        
                        if(p.get().type == MdTokenType.Colon as int) {
                            p.bump();
                            align = MdTableAlign.Left;
                            
                            // Parse dashes
                            while(p.get().type == MdTokenType.Dash as int) { p.bump(); }
                            
                            if(p.get().type == MdTokenType.Colon as int) {
                                p.bump();
                                align = MdTableAlign.Center;
                            }
                        } else if(p.get().type == MdTokenType.Dash as int) {
                            // Parse dashes
                            while(p.get().type == MdTokenType.Dash as int) { p.bump(); }
                            
                            if(p.get().type == MdTokenType.Colon as int) {
                                p.bump();
                                align = MdTableAlign.Right;
                            }
                        }
                        
                        alignments.push(align);
                        
                        // Skip separator
                        while(!is_line_end(p.get().type) && 
                              !(p.get().type == MdTokenType.Colon as int || p.get().type == MdTokenType.Dash as int)) {
                            p.bump();
                        }
                        
                        if(is_line_end(p.get().type)) {
                            parsing_alignment = false;
                        }
                    }
                    
                    table.alignments = alignments;
                }
                
                // Skip the separator line completely
                while(!is_line_end(p.get().type)) { p.bump(); }
                if(is_nl(p.get().type)) p.bump();
                
                is_header = false;
            }
        }
        
        return table as *mut MdNode;
    }

    func parse_abbreviation_def(&mut self, p : &mut TokenParser) : *mut MdNode {
        // Parse [text]: definition
        p.bump(); // consume [
        
        var abbr_id = self.arena.allocate<std::string>();
        new (abbr_id) std::string();
        
        while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
            if(is_text(p.get().type)) {
                abbr_id.append_view(p.get().value);
            }
            p.bump();
        }
        if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
        if(p.get().type == MdTokenType.Colon as int) p.bump(); // consume :
        
        // Skip leading spaces
        while(is_text(p.get().type) && is_ws_only(p.get().value)) {
            p.bump();
        }
        
        var abbr_def = self.arena.allocate<std::string>();
        new (abbr_def) std::string();
        
        while(!is_line_end(p.get().type)) {
            if(is_text(p.get().type)) {
                abbr_def.append_view(p.get().value);
            }
            p.bump();
        }
        
        // Create abbreviation definition node
        var abbr = self.arena.allocate<MdAbbreviation>();
        new (abbr) MdAbbreviation { 
            base : MdNode { kind : MdNodeKind.Abbreviation }, 
            id : abbr_id.to_view(),
            title : abbr_def.to_view()
        };
        
        if(is_nl(p.get().type)) p.bump();
        return abbr as *mut MdNode;
    }

    func parse_footnote_def(&mut self, p : &mut TokenParser) : *mut MdNode {
        // Parse [^id]: definition
        p.bump(); // consume [
        p.bump(); // consume ^
        
        var footnote_id = self.arena.allocate<std::string>();
        new (footnote_id) std::string();
        
        while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
            if(is_text(p.get().type)) {
                footnote_id.append_view(p.get().value);
            }
            p.bump();
        }
        if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
        if(p.get().type == MdTokenType.Colon as int) p.bump(); // consume :
        
        // Skip leading spaces
        while(is_text(p.get().type) && is_ws_only(p.get().value)) {
            p.bump();
        }
        
        var footnote_def = self.arena.allocate<MdFootnoteDef>();
        new (footnote_def) MdFootnoteDef { 
            base : MdNode { kind : MdNodeKind.FootnoteDef }, 
            id : footnote_id.to_view(),
            children : std::vector<*mut MdNode>()
        };
        
        // Parse definition content as paragraph
        var para = self.arena.allocate<MdParagraph>();
        new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
        self.parse_inline_text_until_line_end(p, para.children);
        
        if(para.children.size() > 0) {
            footnote_def.children.push(para as *mut MdNode);
        }
        
        if(is_nl(p.get().type)) p.bump();
        return footnote_def as *mut MdNode;
    }

    func parse_block(&mut self, p : &mut TokenParser) : *mut MdNode {
        const t = p.get();
        if(t.type == MdTokenType.Hash as int) return self.parse_header(p);
        if(t.type == MdTokenType.FencedCodeStart as int) return self.parse_fenced_code_block(p);
        if(is_hr(p)) return self.parse_horizontal_rule(p);
        if(is_blockquote(p)) return self.parse_blockquote(p);
        if(is_custom_container(p)) return self.parse_custom_container(p);
        if(is_abbreviation_def(p)) return self.parse_abbreviation_def(p);
        if(is_footnote_def(p)) return self.parse_footnote_def(p);
        if(is_definition_list(p)) return self.parse_definition_list(p);
        if(is_list_start(p)) return self.parse_list(p, false);
        if(is_ordered_list_start(p)) return self.parse_list(p, true);
        // Check table last - it's the most specific and can cause false positives
        if(is_table_row(p)) return self.parse_table(p);
        return self.parse_paragraph(p);
    }
}

public func parse(tokens : *mut std::vector<MdToken>, arena : *mut Arena) : *mut MdRoot {
    var p : TokenParser; p.init(tokens)
    var mdp = MdParser { arena : arena }

    var root = arena.allocate<MdRoot>();
    new (root) MdRoot { base : MdNode { kind : MdNodeKind.Root }, children : std::vector<*mut MdNode>() };

    while(!is_end(p.get().type)) {
        skip_blank_lines(p)
        if(is_end(p.get().type)) break
        var n = mdp.parse_block(p)
        if(n != null) root.children.push(n)
    }

    return root;
}

}
