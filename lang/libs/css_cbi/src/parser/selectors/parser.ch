
func parseSimpleSelector(parser : *mut Parser, builder : *mut ASTBuilder, start : Token) : *mut SimpleSelector {
    var s = builder.allocate<SimpleSelector>(); // Need to ensure alignment/allocation matches struct
    
    switch(start.type) {
        TokenType.Identifier => {
            s.kind = SimpleSelectorKind.Tag
            s.value = builder.allocate_view(start.value)
        }
        TokenType.Multiply => {
            s.kind = SimpleSelectorKind.Universal
            s.value = std::string_view("*")
        }
        TokenType.Ampersand => {
            s.kind = SimpleSelectorKind.Ampersand
            s.value = std::string_view("&")
        }
        TokenType.Dot => {
            parser.increment(); // consume dot
            const next = parser.getToken();
            if(next.type != TokenType.Identifier) {
                parser.error("expected identifier after '.'");
                return null;
            }
            s.kind = SimpleSelectorKind.Class
            s.value = builder.allocate_view(next.value)
        }
        TokenType.Hash => {
            parser.increment(); 
            const next = parser.getToken();
            // Hash token includes the name usually in lexer? 
            // Lexer: 'number' or 'id' logic.
            // Let's check nextToken.ch
            // Case '#': if Selector mode, reads css_id -> TokenType.Id with value.
            // So 'start' token already has the value (e.g., "#id").
            // Wait, looking at `nextToken.ch`:
            // `TokenType.Id` includes `#`?
            // "value : std::string_view(start, provider.current_data() - start)"
            // Yes, it includes `#`.
            // But strict parser might separate `#` and `name`?
            // Lexer says: `TokenType.Id` for `#...`.
            // So we don't need to consume next token if it came as `TokenType.Id`.
            // But if it came as `#` (HexColor context problem?), Lexer handles context.
            // We set context to Selector. So we should get `TokenType.Id`.
            s.kind = SimpleSelectorKind.Id
            // Value includes `#`?
            // If we want just name, we strip it. CSSOM usually keeps `#` for ID selector string?
            // Or just name. `SimpleSelector` value usually implies just the name for ID/Class?
            // Let's keep strict check.
            if(start.value.get(0) == '#') {
                 s.value = builder.allocate_view(start.value.skip(1))
            } else {
                 s.value = builder.allocate_view(start.value)
            }
        }
        // Attribute selector [ ... ]
        // TokenType.LBracket? (Not in enum shown)
        
        default => {
             return null;
        }
    }
    return s;
}

func parseCompoundSelector(parser : *mut Parser, builder : *mut ASTBuilder) : *mut CompoundSelector {
    var comp = builder.allocate<CompoundSelector>();
    new (comp) CompoundSelector {
         simple_selectors : std::vector<*mut SimpleSelector>()
    }
    
    var last_line : uint = 0;
    var last_char : uint = 0;
    var first = true;
    
    while(true) {
        const token = parser.getToken();
        
        // Adjacency check for subsequent simple selectors
        if(!first) {
            // If space between tokens, loop ends (it's a combinator)
            if(token.position.line > last_line) {
                 break;
            }
            if(token.position.line == last_line && token.position.character > last_char) { 
                 break;
            }
        }
        
        var simple : *mut SimpleSelector = null;

        
        if(token.type == TokenType.ClassName) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Class;
             simple.value = builder.allocate_view(token.value);
             parser.increment();
        } else if(token.type == TokenType.Id) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Id;
             simple.value = builder.allocate_view(token.value); // strip # 
             parser.increment();
        } else if(token.type == TokenType.Identifier) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Tag;
             simple.value = builder.allocate_view(token.value);
             parser.increment();
        } else if(token.type == TokenType.Ampersand) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Ampersand;
             simple.value = std::string_view("&");
             parser.increment();
        } else if(token.type == TokenType.Multiply) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Universal;
             simple.value = std::string_view("*");
             parser.increment();
        } else {
             break; 
        }
        
        if(simple != null) {
            comp.simple_selectors.push(simple);
            last_line = token.position.line;
            last_char = token.position.character + token.value.size();
            first = false;
        } else {
            break; 
        }
    }
    
    if(comp.simple_selectors.empty()) return null;
    return comp;
}

func parseComplexSelector(parser : *mut Parser, builder : *mut ASTBuilder) : *mut ComplexSelector {
    var head = builder.allocate<ComplexSelector>();
    head.combinator = Combinator.None;
    head.next = null;
    
    head.compound = parseCompoundSelector(parser, builder);
    if(head.compound == null) return null;
    
    var curr = head;
    
    while(true) {
        const token = parser.getToken();
        var combinator = Combinator.None;
        var consumed = false;
        
        switch(token.type) {
            TokenType.Plus => {
                combinator = Combinator.NextSibling;
                parser.increment();
                consumed = true;
            }
            TokenType.GeneralSibling => {
                combinator = Combinator.SubsequentSibling;
                parser.increment();
                consumed = true;
            }
            TokenType.GreaterThan => {
                combinator = Combinator.Child;
                parser.increment();
                consumed = true;
            }
            TokenType.Comma, TokenType.LBrace, TokenType.EndOfFile => {
                return head;
            }
            default => {
                // If we have a compound selector next, it's a Descendant combinator (space)
                // We check if it parses as compound.
                // implied space.
                combinator = Combinator.Descendant;
            }
        }
        
        // If we consumed a explicit combinator, we expect a compound selector next.
        // If implied (Descendant), we try to parse.
        
        var next_compound = parseCompoundSelector(parser, builder); // recursing doesn't use initial_token
        if(next_compound == null) {
            if(consumed) {
                 parser.error("expected selector after combinator");
            }
            break;
        }
        
        // Link
        curr.combinator = combinator; // The combinator linking current to next
        var next_node = builder.allocate<ComplexSelector>();
        next_node.compound = next_compound;
        next_node.combinator = Combinator.None;
        next_node.next = null;
        
        curr.next = next_node;
        curr = next_node;
    }
    
    return head;
}

func parseSelectorList(parser : *mut Parser, builder : *mut ASTBuilder) : *mut SelectorList {
    var list = builder.allocate<SelectorList>();
    new (list) SelectorList {
        selectors : std::vector<*mut ComplexSelector>()
    }
    
    // var first = true;
    
    while(true) {
        var sel = parseComplexSelector(parser, builder);
        // first = false;
        
        if(sel != null) {
            list.selectors.push(sel);
        } else {
            // Error?
            break;
        }
        
        const token = parser.getToken();
        if(token.type == TokenType.Comma) {
            parser.increment();
            continue;
        } else {
            break;
        }
    }
    
    return list;
}
