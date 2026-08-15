
func parseSimpleSelector(parser : *mut Parser, builder : *mut ASTBuilder, start : Token) : *mut SimpleSelector {
    var s = builder.allocate<SimpleSelector>(); // Need to ensure alignment/allocation matches struct
    s.attr = null;

    switch(start.type) {
        TokenType.Identifier, TokenType.PropertyName => {
            s.kind = SimpleSelectorKind.Tag
            s.value = builder.allocate_view(&start.value)
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
            s.value = builder.allocate_view(&next.value)
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
                 s.value = builder.allocate_view(&start.value)
            }
        }
        // Attribute selectors are handled in parseCompoundSelector, which keeps
        // the live parser position. parseSimpleSelector is kept for completeness.
        
        default => {
             return null;
        }
    }
    return s;
}

func strip_quotes_if_any(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.get(0);
        const last = value.get(value.size() - 1);
        if((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.subview(1, value.size() - 1);
        }
    }
    return value;
}

// Parses an attribute selector starting at the '[' token: [attr], [attr=value],
// [attr^="pre"], [attr$="suf"], [attr*="sub"], [attr~="word"], [attr|="prefix"],
// optionally followed by a case-sensitivity flag (i / s).
// `end_line`/`end_char` receive the position just past the closing ']' so the
// caller can keep building the same compound selector (e.g. `&[data-x="y"]::p`).
func parseAttributeSelector(parser : *mut Parser, builder : *mut ASTBuilder, start : *mut Token, end_line : *mut uint, end_char : *mut uint) : *mut SimpleSelector {
    var s = builder.allocate<SimpleSelector>();
    var attr = builder.allocate<AttributeSelectorData>();
    new (attr) AttributeSelectorData();

    parser.increment(); // consume '['

    // attribute name
    const nameTok = parser.getToken();
    if(nameTok.type != TokenType.Identifier && nameTok.type != TokenType.PropertyName) {
        parser.error("expected attribute name after '['");
        return null;
    }
    attr.name = builder.allocate_view(&nameTok.value);
    parser.increment();

    // optional operator + value
    const opTok = parser.getToken();
    if(opTok.type != TokenType.RBracket) {
        switch(opTok.type) {
            TokenType.Equal => attr.operator = view("=")
            TokenType.ContainsWord => attr.operator = view("~=")
            TokenType.ContainsSubstr => attr.operator = view("*=")
            TokenType.StartsWith => attr.operator = view("^=")
            TokenType.EndsWith => attr.operator = view("$=")
            TokenType.DashSeparatedMatch => attr.operator = view("|=")
            default => {
                parser.error("invalid attribute selector operator");
                return null;
            }
        }
        parser.increment();

        const valTok = parser.getToken();
        if(valTok.type == TokenType.DoubleQuotedValue || valTok.type == TokenType.SingleQuotedValue ||
            valTok.type == TokenType.Identifier || valTok.type == TokenType.PropertyName ||
            valTok.type == TokenType.Number) {
            attr.value = builder.allocate_view(&strip_quotes_if_any(valTok.value));
            parser.increment();
        } else {
            parser.error("expected attribute selector value");
            return null;
        }

        // optional flags (i, s)
        const flagTok = parser.getToken();
        if(flagTok.type == TokenType.Identifier) {
            attr.flags = builder.allocate_view(&flagTok.value);
            parser.increment();
        }
    }

    const closeTok = parser.getToken();
    if(closeTok.type != TokenType.RBracket) {
        parser.error("expected ']' to close attribute selector");
        return null;
    }
    *end_line = closeTok.position.line;
    *end_char = closeTok.position.character + closeTok.value.size();
    parser.increment();

    s.kind = SimpleSelectorKind.Attribute;
    s.value = view("");
    s.attr = attr;
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
    var attr_parsed = false;
    
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
             simple.value = builder.allocate_view(&token.value);
             parser.increment();
        } else if(token.type == TokenType.Id) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Id;
             simple.value = builder.allocate_view(&token.value); // strip #
             parser.increment();
        } else if(token.type == TokenType.Identifier || token.type == TokenType.PropertyName) {
             simple = builder.allocate<SimpleSelector>();
             simple.kind = SimpleSelectorKind.Tag;
             simple.value = builder.allocate_view(&token.value);
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
        } else if(token.type == TokenType.LBracket) {
             var attrEndLine : uint = 0
             var attrEndChar : uint = 0
             simple = parseAttributeSelector(parser, builder, token, &raw mut attrEndLine, &raw mut attrEndChar);
             if(simple == null) {
                 break;
             }
             // The attribute selector consumed through ']' — advance the
             // adjacency cursor to just past it so a directly-following
             // pseudo/class stays in the same compound.
             last_line = attrEndLine
             last_char = attrEndChar
             attr_parsed = true
        } else if(token.type == TokenType.Colon) {
             if(simple != null) {
                 break;
             }
             parser.increment();
             const next = parser.getToken();
             if(next.type == TokenType.Colon) {
                 // Pseudo-element (::)
                 parser.increment();
                 const nameToken = parser.getToken();
                 if(nameToken.type == TokenType.Identifier || nameToken.type == TokenType.PropertyName) {
                     simple = builder.allocate<SimpleSelector>();
                     simple.kind = SimpleSelectorKind.PseudoElement;
                     simple.value = builder.allocate_view(&nameToken.value);
                     parser.increment();
                 } else {
                      // Unexpected token after ::
                      break; 
                 }
             } else if(next.type == TokenType.Identifier || next.type == TokenType.PropertyName) {
                 // Pseudo-class (:)
                 simple = builder.allocate<SimpleSelector>();
                 simple.kind = SimpleSelectorKind.PseudoClass;
                 
                 var val = std::string(next.value)
                 parser.increment()
                 if(parser.increment_if(TokenType.LParen as int)) {
                     val.append('(')
                     while(true) {
                         const t2 = parser.getToken()
                         if(t2.type == TokenType.RParen || t2.type == TokenType.EndOfFile) break
                         val.append_view(&t2.value)
                         parser.increment()
                     }
                     if(parser.increment_if(TokenType.RParen as int)) {
                         val.append(')')
                     }
                 }
                 simple.value = builder.allocate_view(std::string_view(val.data(), val.size()));
             } else {
                 // Unexpected token after :
                 break;
             }
        } else {
             break; 
        }
        
        if(simple != null) {
            comp.simple_selectors.push(simple);
            if(!attr_parsed) {
                last_line = token.position.line;
                last_char = token.position.character + token.value.size();
                if(token.type == TokenType.ClassName || token.type == TokenType.Id) {
                    last_char++; // account for '.' or '#' prefix not included in value
                }
            }
            attr_parsed = false;
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
