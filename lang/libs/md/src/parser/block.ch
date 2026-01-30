public namespace md {

public func parse_header(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    // count consecutive #
    var level = 0;
    while(p.get().type == MdTokenType.Hash as int) { level += 1; p.bump(); }
    if(level <= 0) level = 1;

    // Skip any whitespace after #
    while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) {
        p.bump();
    }

    const arena = parser.arena
    var h = arena.allocate<MdHeader>();
    new (h) MdHeader { base : MdNode { kind : MdNodeKind.Header }, level : level, children : std::vector<*mut MdNode>() };
    
    // Handle case where first text token has leading space
    if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
        var text_val = p.get().value;
        if(text_val.size() > 1) {
            var trimmed = std::string_view(text_val.data() + 1, text_val.size() - 1);
            var text_node = arena.allocate<MdText>();
            new (text_node) MdText { base : MdNode { kind : MdNodeKind.Text }, value : trimmed };
            h.children.push(text_node as *mut MdNode);
        }
        p.bump();
    }
    
    parse_inline_text_until_line_end(parser, p, h.children);
    
    if(is_nl(p.get().type)) p.bump();
    return h as *mut MdNode;
}

public func parse_paragraph(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {

    const arena = parser.arena;
    var para = arena.allocate<MdParagraph>();
    new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
    
    parse_inline_text_until_line_end(parser, p, para.children);
    
    if(is_nl(p.get().type)) p.bump();
    return para as *mut MdNode;
}

public func parse_fenced_code_block(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    const lang = p.get().value;
    p.bump();

    const arena = parser.arena;
    var code_ptr = arena.allocate<std::string>();
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
        p.bump();
    }

    var cb = arena.allocate<MdCodeBlock>();
    new (cb) MdCodeBlock { base : MdNode { kind : MdNodeKind.CodeBlock }, language : lang, code : code_ptr.to_view() };
    return cb as *mut MdNode;
}

public func is_hr(p : &mut TokenParser) : bool {
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

public func parse_horizontal_rule(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    while(!is_line_end(p.get().type)) { p.bump(); }
    if(is_nl(p.get().type)) p.bump();
    const arena = parser.arena;
    var hr = arena.allocate<MdHorizontalRule>();
    new (hr) MdHorizontalRule { base : MdNode { kind : MdNodeKind.Hr } };
    return hr as *mut MdNode;
}

public func is_blockquote(p : &mut TokenParser) : bool {
    return p.get().type == MdTokenType.GreaterThan as int;
}

public func parse_blockquote(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    const arena = parser.arena;
    var bq = arena.allocate<MdBlockquote>();
    new (bq) MdBlockquote { base : MdNode { kind : MdNodeKind.Blockquote }, children : std::vector<*mut MdNode>() };
    
    while(true) {
        var content_parsed = false;
        if(p.get().type == MdTokenType.GreaterThan as int) {
            p.bump(); // consume >
            
            if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                 if(p.get().value.size() == 1) {
                     p.bump(); 
                 }
            }
            
            if(!is_line_end(p.get().type)) {
                const arena = parser.arena;
                 var para = arena.allocate<MdParagraph>();
                 new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
                 parse_inline_text_until_line_end(parser, p, para.children);
                 bq.children.push(para as *mut MdNode);
                 content_parsed = true;
            }
        }
        
        if(!content_parsed && !is_line_end(p.get().type)) {
             var para = arena.allocate<MdParagraph>();
             new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
             parse_inline_text_until_line_end(parser, p, para.children);
             bq.children.push(para as *mut MdNode);
        }
        
        if(is_nl(p.get().type)) p.bump();
        if(p.get().type != MdTokenType.GreaterThan as int) break;
    }
    
    return bq as *mut MdNode;
}

public func parse_block(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    const t = p.get();
    if(is_custom_container(p)) return parse_custom_container(parser, p);
    if(t.type == MdTokenType.Hash as int) return parse_header(parser, p);
    if(t.type == MdTokenType.FencedCodeStart as int) return parse_fenced_code_block(parser, p);
    if(is_hr(p)) return parse_horizontal_rule(parser, p);
    if(is_blockquote(p)) return parse_blockquote(parser, p);
    if(is_abbreviation_def(p)) return parse_abbreviation_def(parser, p);
    if(is_footnote_def(p)) return parse_footnote_def(parser, p);
    if(is_definition_list(p)) return parse_definition_list(parser, p);
    if(is_list_start(p)) return parse_list(parser, p, false);
    if(is_ordered_list_start(p)) return parse_list(parser, p, true);
    if(is_table_row(p)) return parse_table(parser, p);
    return parse_paragraph(parser, p);
}

}
