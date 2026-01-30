public namespace md {

// Helper to append token value to string
func append_token_value(s : *mut std::string, t : MdToken) {
    if(t.value.size() > 0) {
        s.append_view(t.value);
    }
}

public func parse_inline_until_end_marker(parser : &mut MdParser, p : &mut TokenParser, marker_type : int, count : int, out_children : &mut std::vector<*mut MdNode>) {
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
        
        // Recursive parsing for nested elements
        parse_single_inline_element(parser, p, out_children);
    }
}

public func parse_single_inline_element(parser : &mut MdParser, p : &mut TokenParser, out_children : &mut std::vector<*mut MdNode>) {
    const arena = parser.arena;
    const t = p.get();
    
    // Bold/Italic (* or _)
    if(t.type == MdTokenType.Star as int || t.type == MdTokenType.Underscore as int) {
        const next = p.peek();
        if(next.type == t.type) {
            // Bold (** or __)
            p.bump(); p.bump(); // consume both
            var bold = arena.allocate<MdBold>();
            new (bold) MdBold { base : MdNode { kind : MdNodeKind.Bold }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, t.type, 2, bold.children);
            out_children.push(bold as *mut MdNode);
        } else {
            // Italic (* or _)
            p.bump();
            var italic = arena.allocate<MdItalic>();
            new (italic) MdItalic { base : MdNode { kind : MdNodeKind.Italic }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, t.type, 1, italic.children);
            out_children.push(italic as *mut MdNode);
        }
        return;
    }

    // Strikethrough/Subscript (~)
    if(t.type == MdTokenType.Tilde as int) {
        const next = p.peek();
        if(next.type == MdTokenType.Tilde as int) {
            // Strikethrough (~~)
            p.bump(); p.bump();
            var del = arena.allocate<MdStrikethrough>();
            new (del) MdStrikethrough { base : MdNode { kind : MdNodeKind.Strikethrough }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, MdTokenType.Tilde as int, 2, del.children);
            out_children.push(del as *mut MdNode);
        } else {
            // Subscript (~)
            p.bump();
            var sub = arena.allocate<MdSubscript>();
            new (sub) MdSubscript { base : MdNode { kind : MdNodeKind.Subscript }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, MdTokenType.Tilde as int, 1, sub.children);
            out_children.push(sub as *mut MdNode);
        }
        return;
    }

    // Mark (==)
    if(t.type == MdTokenType.Equal as int) {
        const next = p.peek();
        if(next.type == MdTokenType.Equal as int) {
            p.bump(); p.bump();
            var mark = arena.allocate<MdMark>();
            new (mark) MdMark { base : MdNode { kind : MdNodeKind.Mark }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, MdTokenType.Equal as int, 2, mark.children);
            out_children.push(mark as *mut MdNode);
            return;
        }
    }

    // Insert (++)
    if(t.type == MdTokenType.Plus as int) {
        const next = p.peek();
        if(next.type == MdTokenType.Plus as int) {
            p.bump(); p.bump();
            var ins = arena.allocate<MdInsert>();
            new (ins) MdInsert { base : MdNode { kind : MdNodeKind.Insert }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, MdTokenType.Plus as int, 2, ins.children);
            out_children.push(ins as *mut MdNode);
            return;
        }
    }

    // Superscript (^) or Footnote Reference [^1]
    if(t.type == MdTokenType.Caret as int) {
        p.bump();
        if(p.get().type == MdTokenType.LBracket as int) {
            // Footnote reference [^1]
            p.bump(); // consume [
            
            var footnote_id = arena.allocate<std::string>();
            new (footnote_id) std::string();
            
            while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
                append_token_value(footnote_id, p.get());
                p.bump();
            }
            if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
            
            var footnote = arena.allocate<MdFootnote>();
            new (footnote) MdFootnote { 
                base : MdNode { kind : MdNodeKind.Footnote }, 
                id : footnote_id.to_view()
            };
            out_children.push(footnote as *mut MdNode);
        } else {
            // Regular superscript
            var sup = arena.allocate<MdSuperscript>();
            new (sup) MdSuperscript { base : MdNode { kind : MdNodeKind.Superscript }, children : std::vector<*mut MdNode>() };
            parse_inline_until_end_marker(parser, p, MdTokenType.Caret as int, 1, sup.children);
            out_children.push(sup as *mut MdNode);
        }
        return;
    }

    // Inline Code (`)
    if(t.type == MdTokenType.Backtick as int) {
        p.bump();
        var code = arena.allocate<std::string>();
        new (code) std::string();
        
        while(p.get().type != MdTokenType.Backtick as int && !is_line_end(p.get().type)) {
            // Fix: Capture ALL token values, not just Text
            append_token_value(code, p.get());
            p.bump();
        }
        
        if(p.get().type == MdTokenType.Backtick as int) p.bump(); // closing backtick
        
        // Trim trailing spaces (standard MD behavior for inline code often trims leading/trailing space if both present, but simple trim here)
        while(code.size() > 0 && code.data()[code.size() - 1] == ' ') {
            code.erase(code.size() - 1, 1);
        }
        
        var code_node = arena.allocate<MdInlineCode>();
        new (code_node) MdInlineCode { base : MdNode { kind : MdNodeKind.InlineCode }, value : code.to_view() };
        out_children.push(code_node as *mut MdNode);
        return;
    }

    // Links and Images ([...], ![...])
    if(t.type == MdTokenType.Exclamation as int && p.peek().type == MdTokenType.LBracket as int) {
        // Image ![alt](url)
        p.bump(); // consume !
        p.bump(); // consume [
        
        var alt = arena.allocate<std::string>();
        new (alt) std::string();
        while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
            append_token_value(alt, p.get());
            p.bump();
        }
        if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
        
        if(p.get().type == MdTokenType.LParen as int) {
            p.bump(); // consume (
            
            var url = arena.allocate<std::string>();
            new (url) std::string();
            while(p.get().type != MdTokenType.RParen as int && !is_line_end(p.get().type)) {
                append_token_value(url, p.get());
                p.bump();
            }
            if(p.get().type == MdTokenType.RParen as int) p.bump(); // consume )
            
            var img = arena.allocate<MdImage>();
            new (img) MdImage { 
                base : MdNode { kind : MdNodeKind.Image }, 
                url : url.to_view(),
                alt : alt.to_view(),
                title : std::string_view("")
            };
            out_children.push(img as *mut MdNode);
            return;
        }
        // Fallback if not an image structure? Just text? 
        // For simplicity, we assume valid image struct or just fail gracefully.
    }

    if(t.type == MdTokenType.LBracket as int) {
        // Link [text](url) or abbreviation
        var link_text = arena.allocate<std::string>();
        new (link_text) std::string();
        p.bump(); // consume [
        
        while(p.get().type != MdTokenType.RBracket as int && !is_line_end(p.get().type)) {
           append_token_value(link_text, p.get());
           p.bump();
        }
        if(p.get().type == MdTokenType.RBracket as int) p.bump(); // consume ]
        
        if(p.get().type == MdTokenType.LParen as int) {
            // Link [text](url)
            p.bump(); // consume (
            
            var url = arena.allocate<std::string>();
            new (url) std::string();
            while(p.get().type != MdTokenType.RParen as int && !is_line_end(p.get().type)) {
                append_token_value(url, p.get());
                p.bump();
            }
            if(p.get().type == MdTokenType.RParen as int) p.bump(); // consume )
            
            var link = arena.allocate<MdLink>();
            new (link) MdLink { 
                base : MdNode { kind : MdNodeKind.Link }, 
                url : url.to_view(),
                title : std::string_view(""),
                children : std::vector<*mut MdNode>()
            };
            
            // Parse link text as inline content
            // Need to parse the link_text string again
            // This is tricky because we already consumed it into a string.
            // We can just create a text node for now, or we'd need to re-lex the string.
            // Existing parser just made it text. We will follow that.
            var text_node = parser.make_text(link_text.to_view());
            link.children.push(text_node);
            
            out_children.push(link as *mut MdNode);
            return;
        } else if(p.get().type == MdTokenType.Colon as int) {
             // Abbreviation definition - handled at block level usually, but if here, treat as text
             var text = arena.allocate<std::string>();
             new (text) std::string();
             text.append('[');
             text.append_view(link_text.to_view());
             text.append(']');
             text.append(':');
             out_children.push(parser.make_text(text.to_view()));
             return;
        } else {
            // Abbreviation reference or just brackets
            var abbr = arena.allocate<MdAbbreviation>();
            new (abbr) MdAbbreviation { 
                base : MdNode { kind : MdNodeKind.Abbreviation }, 
                id : link_text.to_view(),
                title : std::string_view("")
            };
            out_children.push(abbr as *mut MdNode);
            return;
        }
    }
    
    // Default: treat as text
    out_children.push(parser.make_text(t.value));
    p.bump();
}

public func parse_inline_text_until_line_end(parser : &mut MdParser, p : &mut TokenParser, out_children : &mut std::vector<*mut MdNode>) {
    while(true) {
        const t = p.get();
        if(is_end(t.type) || is_nl(t.type)) break;
        parse_single_inline_element(parser, p, out_children);
    }
}

}
