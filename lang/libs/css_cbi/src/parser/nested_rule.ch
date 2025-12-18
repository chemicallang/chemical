func (cssParser : &mut CSSParser) parseNestedRule(om : &mut CSSOM, parser : *mut Parser, builder : *mut ASTBuilder) : bool {
    
    // Parse selector using the new AST parser
    var selector = parseSelectorList(parser, builder);
    if(selector == null) {
        // If we failed to parse a selector, it might be because it's not a rule but something else?
        // But context implies we expect a rule here effectively.
        // We return false to let caller handle it (maybe end of block?).
        return false;
    }
    
    if(!parser.increment_if(TokenType.LBrace as int)) {
        // Missing brace means invalid rule
        // parser.error("expected '{' after selector");
        return false;
    }
    
    var rule = builder.allocate<CSSNestedRule>();
    new (rule) CSSNestedRule {
        selector : selector,
        declarations : std::vector<*mut CSSDeclaration>(),
        nested_rules : std::vector<*mut CSSNestedRule>()
    }
    
    // Parse body (declarations or nested rules)
    while(true) {
        var token = parser.getToken();
        switch(token.type) {
             TokenType.RBrace => {
                 parser.increment();
                 om.nested_rules.push(rule);
                 return true;
             }
             TokenType.EndOfFile => {
                 parser.error("unexpected end of file in nested rule");
                 return false;
             }
             TokenType.PropertyName, TokenType.Identifier => {
                 const decl = cssParser.parseDeclaration(parser, builder);
                 if(decl) {
                     rule.declarations.push(decl);
                 } else {
                     // Not a declaration (missing colon).
                     // Try parsing as nested rule starting with this identifier.
                     // We recurse into parseNestedRule, passing 'rule' as parent (we need to cast/wrap it?)
                     // Wait, 'om' in signature is 'CSSOM'. We need to pass 'CSSNestedRule'.
                     // This function needs to be generic or take a common interface.
                     // For now, let's duplicate or make a helper. 
                     // Since Chemical doesn't support generics easily in this context without `interface` or `trait`?
                     // Actually `CSSOM` structure is distinct.
                     // We can define `parseNestedRuleContent` that takes vectors directly?
                     
                     parser.increment();
                     if(!cssParser.parseNestedRuleContent(rule, parser, builder)) {
                         // If fallback fails, it fails.
                         parser.error("unexpected identifier in nested rule");
                         return false; 
                     }
                 }
             }
             default => {
                 // Try parsing nested rule (starting with non-identifier, e.g. .class, &)
                 // Pass 'rule' as parent.
                 if(!cssParser.parseNestedRuleContent(rule, parser, builder)) {
                      parser.error("unexpected token in nested rule body");
                      return false;
                 }
             }
        }
    }
    return true;
}

// Helper to parse content into a nested rule (acting as parent)
func (cssParser : &mut CSSParser) parseNestedRuleContent(parent : *mut CSSNestedRule, parser : *mut Parser, builder : *mut ASTBuilder) : bool {
    var selector = parseSelectorList(parser, builder);
    if(selector == null) return false;
    
    if(!parser.increment_if(TokenType.LBrace as int)) return false;
    
    var rule = builder.allocate<CSSNestedRule>();
    new (rule) CSSNestedRule {
        selector : selector,
        declarations : std::vector<*mut CSSDeclaration>(),
        nested_rules : std::vector<*mut CSSNestedRule>()
    }
    
    while(true) {
        var token = parser.getToken();
        switch(token.type) {
             TokenType.RBrace => {
                 parser.increment();
                 parent.nested_rules.push(rule);
                 return true;
             }
             TokenType.EndOfFile => {
                 parser.error("unexpected end of file");
                 return false;
             }
             TokenType.PropertyName, TokenType.Identifier => {
                 const decl = cssParser.parseDeclaration(parser, builder);
                 if(decl) {
                     rule.declarations.push(decl);
                 } else {
                     parser.increment();
                     if(!cssParser.parseNestedRuleContent(rule, parser, builder)) {
                         return false;
                     }
                 }
             }
             default => {
                 if(!cssParser.parseNestedRuleContent(rule, parser, builder)) {
                      return false;
                 }
             }
        }
    }
    return true;
}
