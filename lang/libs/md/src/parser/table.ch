public namespace md {

public func is_table_row(p : &mut TokenParser) : bool {
    var i = 0u;
    var pipe_count = 0;
    var found_content = false;
    
    while(i < 50 && !is_line_end(p.peek_ahead(i).type)) {
        const token = p.peek_ahead(i);
        if(token.type == MdTokenType.Pipe as int) {
            pipe_count++;
        } else if(token.type == MdTokenType.Text as int && token.value.size() > 0) {
            found_content = true;
        }
        i++;
    }
    
    return pipe_count >= 2 && found_content;
}

public func parse_table(parser : &mut MdParser, p : &mut TokenParser) : *mut MdNode {
    const arena = parser.arena;
    var table = arena.allocate<MdTable>();
    new (table) MdTable { base : MdNode { kind : MdNodeKind.Table }, alignments : std::vector<MdTableAlign>(), children : std::vector<*mut MdNode>() };
    
    var is_header = true;
    
    while(!is_line_end(p.get().type)) {
        var row = arena.allocate<MdTableRow>();
        new (row) MdTableRow { base : MdNode { kind : MdNodeKind.TableRow }, is_header : is_header, children : std::vector<*mut MdNode>() };
        
        if(p.get().type == MdTokenType.Pipe as int) p.bump();
        
        while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
            var cell = arena.allocate<MdTableCell>();
            new (cell) MdTableCell { base : MdNode { kind : MdNodeKind.TableCell }, children : std::vector<*mut MdNode>() };
            
            while(!is_line_end(p.get().type) && p.get().type != MdTokenType.Pipe as int) {
                const t = p.get();
                if(is_text(t.type)) {
                    var text_value = t.value;
                    var start = 0u;
                    var end = text_value.size();
                    
                    // Trim leading
                    while(start < end && (text_value.data()[start] == ' ' || text_value.data()[start] == '\t')) { start++; }
                    // Trim trailing
                    while(end > start && (text_value.data()[end - 1] == ' ' || text_value.data()[end - 1] == '\t')) { end--; }
                    
                    // Remove leading '>' if present (Parser fix to match Renderer behavior if needed, 
                    // or actually if renderer was doubled, we keep this to be safe, but cleaner to fix renderer. 
                    // However, original parser had this so I will keep it to avoid regression)
                    if(start < end && text_value.data()[start] == '>') {
                        start++;
                        while(start < end && (text_value.data()[start] == ' ' || text_value.data()[start] == '\t')) { start++; }
                    }
                    
                    if(start < end) {
                        var trimmed_value = std::string_view(text_value.data() + start, end - start);
                        cell.children.push(parser.make_text(trimmed_value));
                    }
                    p.bump();
                } else if(t.type == MdTokenType.GreaterThan as int) {
                    p.bump();
                } else {
                     // Support inline formatting in cells? Original parser skipped non-text.
                     // IMPORTANT: Tables should support inline elements.
                     // But for now, sticking to original logic (skipping tokens) unless I decide to enhance.
                     // The failure was about extra '>', not missing formatting.
                     // I will stick to original logic to minimize risk scope creep.
                    p.bump();
                }
            }
            
            row.children.push(cell as *mut MdNode);
            if(p.get().type == MdTokenType.Pipe as int) p.bump();
        }
        
        table.children.push(row as *mut MdNode);
        
        if(is_nl(p.get().type)) p.bump();
        
        if(is_header) {
            if(p.get().type == MdTokenType.Dash as int || p.get().type == MdTokenType.Pipe as int || p.get().type == MdTokenType.Colon as int) {
                 // Check if it's separator line
                 // Original logic checked for dash immediately.
                 // Let's implement robust separator parsing.
                 
                 var alignments = std::vector<MdTableAlign>();
                 var parsing_alignment = true;
                 var save_idx = p.idx; // Backtrack if fails? No, Md doesn't backtrack usually.
                 
                 // If line starts with pipe, consume it
                 var has_pipe = false;
                 if(p.get().type == MdTokenType.Pipe as int) { p.bump(); has_pipe = true; }
                 
                 // Consume alignments
                 while(parsing_alignment && !is_line_end(p.get().type)) {
                     var align = MdTableAlign.None;
                     var has_colon_left = false;
                     var has_colon_right = false;
                     
                     if(p.get().type == MdTokenType.Colon as int) {
                         p.bump(); has_colon_left = true;
                     }
                     
                     while(p.get().type == MdTokenType.Dash as int) { p.bump(); }
                     
                     if(p.get().type == MdTokenType.Colon as int) {
                         p.bump(); has_colon_right = true;
                     }
                     
                     if(has_colon_left && has_colon_right) align = MdTableAlign.Center;
                     else if(has_colon_left) align = MdTableAlign.Left;
                     else if(has_colon_right) align = MdTableAlign.Right;
                     
                     alignments.push(align);
                     
                     if(p.get().type == MdTokenType.Pipe as int) {
                         p.bump();
                     } else {
                         // End of row?
                     }
                     
                     if(is_line_end(p.get().type)) parsing_alignment = false;
                 }
                 
                 table.alignments = alignments;
                 
                 if(is_nl(p.get().type)) p.bump();
                 is_header = false;
            }
        }
    }
    
    return table as *mut MdNode;
}

}
