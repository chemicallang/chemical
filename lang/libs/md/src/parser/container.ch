public namespace md {

public func is_custom_container(p : &mut TokenParser) : bool {
    var i = 0u;
    while(i < 5 && p.peek_ahead(i).type == MdTokenType.Text as int && is_ws_only(p.peek_ahead(i).value)) { i++; }
    
    if(p.peek_ahead(i).type == MdTokenType.Colon as int &&
       p.peek_ahead(i + 1u).type == MdTokenType.Colon as int &&
       p.peek_ahead(i + 2u).type == MdTokenType.Colon as int) {
        return true;
    }
    return false;
}

public func parse_custom_container(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) { p.bump(); }
    
    var colon_count = 0;
    while(p.get().type == MdTokenType.Colon as int && colon_count < 10) { p.bump(); colon_count++; }
    
    while(p.get().type == MdTokenType.Text as int && is_ws_only(p.get().value)) { p.bump(); }

    const arena = parser.arena;
    var container_type = arena.allocate<std::string>();
    new (container_type) std::string();
    while(!is_line_end(p.get().type)) {
        if(is_text(p.get().type)) { container_type.append_view(p.get().value); }
        p.bump();
    }
    if(is_nl(p.get().type)) p.bump();
    
    // Trim
    while(container_type.size() > 0 && container_type.data()[0] == ' ') { container_type.erase(0, 1); }
    while(container_type.size() > 0 && container_type.data()[container_type.size() - 1] == ' ') { container_type.erase(container_type.size() - 1, 1); }

    var container = arena.allocate<MdCustomContainer>();
    new (container) MdCustomContainer { 
        base : MdNode { kind : MdNodeKind.CustomContainer }, 
        type : container_type.to_view(),
        children : std::vector<*mut MdNode>()
    };
    
    while(true) {
        if(p.get().type == MdTokenType.Colon as int) {
            var closing_colons = 0u;
            while(p.peek_ahead(closing_colons).type == MdTokenType.Colon as int && closing_colons < 10) { closing_colons++; }
            if(closing_colons >= 3) {
                 var i = 0u; while(i < closing_colons) { p.bump(); i++; }
                 if(is_nl(p.get().type)) p.bump();
                 break;
            }
        }
        
        if(is_end(p.get().type)) break;
        skip_blank_lines(p);
        if(is_end(p.get().type) || is_custom_container(p)) break;

        var para = arena.allocate<MdParagraph>();
        new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
        parse_inline_text_until_line_end(parser, p, para.children);
        if(para.children.size() > 0) container.children.push(para as *mut MdNode);
        if(is_nl(p.get().type)) p.bump();
    }
    return container as *mut MdNode;
}

public func is_abbreviation_def(p : &mut TokenParser) : bool {
    if(p.get().type == MdTokenType.LBracket as int) {
        var i = 1u;
        while(i < 20 && p.peek_ahead(i).type != MdTokenType.RBracket as int && !is_line_end(p.peek_ahead(i).type)) {
            if(p.peek_ahead(i).type == MdTokenType.Text as int) { i++; } else { return false; }
        }
        if(p.peek_ahead(i).type == MdTokenType.RBracket as int) {
            i++;
            if(p.peek_ahead(i).type == MdTokenType.Colon as int) { return true; }
        }
    }
    return false;
}

public func parse_abbreviation_def(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    p.bump(); // [
    const arena = parser.arena;
    var abbr_id = arena.allocate<std::string>();
    new (abbr_id) std::string();
    while(p.get().type != MdTokenType.RBracket as int) {
        if(is_text(p.get().type)) abbr_id.append_view(p.get().value);
        p.bump();
    }
    p.bump(); // ]
    p.bump(); // :
    
    while(is_text(p.get().type) && is_ws_only(p.get().value)) { p.bump(); }
    
    var abbr_def = arena.allocate<std::string>();
    new (abbr_def) std::string();
    while(!is_line_end(p.get().type)) {
        if(is_text(p.get().type)) abbr_def.append_view(p.get().value);
        p.bump();
    }
    var def_value = abbr_def.to_view();
    if(def_value.size() > 0 && def_value.data()[0] == ' ') {
        def_value = std::string_view(def_value.data() + 1, def_value.size() - 1);
    }
    
    var abbr = arena.allocate<MdAbbreviation>();
    new (abbr) MdAbbreviation { base : MdNode { kind : MdNodeKind.Abbreviation }, id : abbr_id.to_view(), title : def_value };
    if(is_nl(p.get().type)) p.bump();
    return abbr as *mut MdNode;
}

public func is_footnote_def(p : &mut TokenParser) : bool {
    if(p.get().type == MdTokenType.LBracket as int) {
        var i = 1u;
        if(p.peek_ahead(i).type == MdTokenType.Caret as int) {
            i++;
            if(p.peek_ahead(i).type == MdTokenType.Text as int) {
                i++;
                if(p.peek_ahead(i).type == MdTokenType.RBracket as int) {
                    i++;
                    if(p.peek_ahead(i).type == MdTokenType.Colon as int) return true;
                }
            }
        }
    }
    return false;
}

public func parse_footnote_def(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    p.bump(); // [
    p.bump(); // ^

    const arena = parser.arena;
    var footnote_id = arena.allocate<std::string>();
    new (footnote_id) std::string();
    while(p.get().type != MdTokenType.RBracket as int) {
        if(is_text(p.get().type)) footnote_id.append_view(p.get().value);
        p.bump();
    }
    p.bump(); // ]
    p.bump(); // :
    
    while(is_text(p.get().type) && is_ws_only(p.get().value)) { p.bump(); }
    
    var footnote_def = arena.allocate<MdFootnoteDef>();
    new (footnote_def) MdFootnoteDef { 
        base : MdNode { kind : MdNodeKind.FootnoteDef }, 
        id : footnote_id.to_view(),
        children : std::vector<*mut MdNode>()
    };
    
    var para = arena.allocate<MdParagraph>();
    new (para) MdParagraph { base : MdNode { kind : MdNodeKind.Paragraph }, children : std::vector<*mut MdNode>() };
    parse_inline_text_until_line_end(parser, p, para.children);
    if(para.children.size() > 0) footnote_def.children.push(para as *mut MdNode);
    
    if(is_nl(p.get().type)) p.bump();
    return footnote_def as *mut MdNode;
}

public func is_definition_list(p : &mut TokenParser) : bool {
    // Only return true if we are sure it's a def list.
    // Case 1: : Definition (this is just a DD, we handle it if we are in loop, but start needs term)
    // Actually, definition list starts with Term.
    // is_definition_list is checking if *current line* is a Term followed by a DD.
    
    if(is_text(p.get().type) && p.get().value.size() > 0) {
        // Look ahead for newline followed by colon
        var i = 1u;
        while(!is_line_end(p.peek_ahead(i).type) && i < 20) { i++; }
        
        if(is_nl(p.peek_ahead(i).type)) { // Found newline
            var j = i + 1u;
            // Skip multiple newlines? No, definition must be immediate or separated?
            while(is_nl(p.peek_ahead(j).type)) { j++; }
            if(p.peek_ahead(j).type == MdTokenType.Colon as int) return true;
        }
    }
    return false;
}

public func parse_definition_list(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    const arena = parser.arena;
    var dl = arena.allocate<MdDefinitionList>();
    new (dl) MdDefinitionList { base : MdNode { kind : MdNodeKind.DefinitionList }, children : std::vector<*mut MdNode>() };
    
    while(true) {
         if(is_end(p.get().type)) break;
         
         // Parse Term
         var dt = arena.allocate<MdDefinitionTerm>();
         new (dt) MdDefinitionTerm { base : MdNode { kind : MdNodeKind.DefinitionTerm }, children : std::vector<*mut MdNode>() };
         
         while(!is_line_end(p.get().type)) {
              // Parse inline formatting for term
              parse_single_inline_element(parser, p, dt.children);
         }
         dl.children.push(dt as *mut MdNode);
         while(is_nl(p.get().type)) { p.bump(); }
         
         // Parse Definition
         if(p.get().type == MdTokenType.Colon as int) {
             p.bump(); // :
             var dd = arena.allocate<MdDefinitionData>();
             new (dd) MdDefinitionData { base : MdNode { kind : MdNodeKind.DefinitionData }, children : std::vector<*mut MdNode>() };
             
             // Skip space
             if(is_text(p.get().type) && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                 if(p.get().value.size() == 1) p.bump();
                 else {
                     // Trim
                     var v = p.get().value;
                     p.bump();
                     dd.children.push(parser.make_text(std::string_view(v.data()+1, v.size()-1)));
                 }
             }
             
             parse_inline_text_until_line_end(parser, p, dd.children);
             dl.children.push(dd as *mut MdNode);
         }
         
         while(is_nl(p.get().type)) { p.bump(); }
         if(!is_definition_list(p)) break;
    }
    
    return dl as *mut MdNode;
}

}
