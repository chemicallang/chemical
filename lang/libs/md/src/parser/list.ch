public namespace md {

public func is_list_start(p : &mut TokenParser) : bool {
    const t = p.get().type;
    if(t == MdTokenType.Dash as int || t == MdTokenType.Plus as int || t == MdTokenType.Star as int) {
        const next = p.peek();
        return next.type == MdTokenType.Text as int && next.value.size() > 0 && next.value.data()[0] == ' ';
    }
    return false;
}

public func is_ordered_list_start(p : &mut TokenParser) : bool {
    const t = p.get().type;
    if(t == MdTokenType.Number as int) {
        const next = p.peek();
        if(is_text(next.type) && next.value.size() > 0 && next.value.data()[0] == '.') {
            const next_next = p.peek_ahead(2);
            return is_end(next_next.type) || is_nl(next_next.type) || 
                   (is_text(next_next.type) && next_next.value.size() > 0);
        }
    }
    return false;
}

public func parse_list(parser : &mut MdParser, p : &mut TokenParser, ordered : bool) : *mut MdNode {

    const arena = parser.arena;
    var list = arena.allocate<MdList>();
    new (list) MdList { base : MdNode { kind : MdNodeKind.List }, ordered : ordered, start : 1, children : std::vector<*mut MdNode>() };
    
    while(true) {
        skip_blank_lines(p);
        if(is_end(p.get().type)) break;
        
        const is_ordered_item = is_ordered_list_start(p);
        const is_unordered_item = is_list_start(p);
        
        if((ordered && !is_ordered_item) || (!ordered && !is_unordered_item)) break;

        const arena = parser.arena;
        var item = arena.allocate<MdListItem>();
        new (item) MdListItem { base : MdNode { kind : MdNodeKind.ListItem }, children : std::vector<*mut MdNode>() };
        
        // Consume list marker
        if(ordered) {
            p.bump(); // number
            if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == '.') {
                if(p.get().value.size() == 1) {
                    p.bump();
                } else {
                    // Split token logic
                    var text_val = p.get().value;
                    var trimmed = std::string_view(text_val.data() + 1, text_val.size() - 1);
                    if(trimmed.size() > 0 && trimmed.data()[0] == ' ') {
                        if(trimmed.size() == 1) {
                            p.bump();
                        } else {
                            var content = std::string_view(trimmed.data() + 1, trimmed.size() - 1);
                            var text_node = parser.make_text(content);
                            item.children.push(text_node);
                            p.bump();
                        }
                    } else {
                        var text_node = parser.make_text(trimmed);
                        item.children.push(text_node);
                        p.bump();
                    }
                }
            }
        } else {
            p.bump(); // -, +, or *
        }
        
        // Skip leading space
        if(is_text(p.get().type) && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
            if(p.get().value.size() == 1) {
                p.bump();
            } else {
                var text_val = p.get().value;
                var trimmed = std::string_view(text_val.data() + 1, text_val.size() - 1);
                var text_node = parser.make_text(trimmed);
                item.children.push(text_node);
                p.bump();
            }
        }
        
        // Parse item content
        while(!is_line_end(p.get().type)) {
            // Task Checkbox Fix
            if(p.get().type == MdTokenType.LBracket as int) {
                const next = p.peek();
                if(next.type == MdTokenType.Text as int && next.value.size() == 1 && 
                   (next.value.data()[0] == 'x' || next.value.data()[0] == 'X' || next.value.data()[0] == ' ')) {
                    const is_checked = (next.value.data()[0] == 'x' || next.value.data()[0] == 'X');
                    p.bump(); // [
                    p.bump(); // x or space
                    if(p.get().type == MdTokenType.RBracket as int) p.bump(); // ]

                    const arena = parser.arena;
                    var checkbox = arena.allocate<MdTaskCheckbox>();
                    new (checkbox) MdTaskCheckbox { base : MdNode { kind : MdNodeKind.TaskCheckbox }, checked : is_checked };
                    item.children.push(checkbox as *mut MdNode);
                    
                    // Consume space after checkbox
                    if(p.get().type == MdTokenType.Text as int && p.get().value.size() > 0 && p.get().value.data()[0] == ' ') {
                         // Fix: if token is just space, consume. If more content, trim.
                         if(p.get().value.size() == 1) {
                             p.bump();
                         } else {
                             // This was the bug: it was consuming the whole token or not processing the rest?
                             // Original code: if(size > 0 && [0] == ' ') { p.bump(); }
                             // This would swallow the text if it was " Task text" in one token.
                             var val = p.get().value;
                             var content = std::string_view(val.data() + 1, val.size() - 1);
                             item.children.push(parser.make_text(content));
                             p.bump();
                         }
                    }
                    
                    // Continue parsing the rest of line
                    parse_inline_text_until_line_end(parser, p, item.children);
                    break;
                }
            }
            
            if(is_list_start(p) || is_ordered_list_start(p)) {
                var nested_list = parse_list(parser, p, is_ordered_list_start(p));
                item.children.push(nested_list);
                break;
            }
            
            parse_inline_text_until_line_end(parser, p, item.children);
            break;
        }
        
        if(is_nl(p.get().type)) p.bump();
        list.children.push(item as *mut MdNode);
    }
    
    return list as *mut MdNode;
}

}
