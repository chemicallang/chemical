
func (cssParser : &mut CSSParser) parseMediaRule(om : &mut CSSOM, parser : *mut Parser, builder : *mut ASTBuilder) : bool {

    var token = parser.getToken();
    if(token.type == TokenType.At) {
        parser.increment();
    } else {
        return false;
    }

    token = parser.getToken();
    if(token.type == TokenType.PropertyName) {
        if(fnv1_hash_view(token.value) == comptime_fnv1_hash("media")) {
             parser.increment();
        } else {
            parser.error("expected 'media'");
            return false;
        }
    } else {
        parser.error("expected identifier 'media' after '@'");
        return false;
    }

    // Parse the media query list using proper AST parsing
    const queryList = cssParser.parseMediaQueryList(parser, builder)

    var rule = builder.allocate<CSSMediaRule>();
    new (rule) CSSMediaRule {
        queryList : queryList,
        declarations : std::vector<*mut CSSDeclaration>(),
        nested_rules : std::vector<*mut CSSNestedRule>(),
        parent : om.parent
    }

    om.media_queries.push(rule);

    if (parser.increment_if(TokenType.LBrace as int)) {
        while(true) {
            const body_token = parser.getToken();
            switch(body_token.type) {
                TokenType.RBrace => {
                    parser.increment();
                    return true;
                }
                TokenType.EndOfFile => {
                    parser.error("unexpected end of file in media query block");
                    return false;
                }
                TokenType.At => {
                    parser.error("nested at-rules inside media are not supported yet");
                    return false;
                }
                default => {}
            }
            var decl = cssParser.parseDeclaration(parser, builder);
            if(decl) {
                rule.declarations.push(decl)
            } else {
                if(!cssParser.parseNestedRuleContentInto(rule.nested_rules, parser, builder)) {
                    parser.error("expected a declaration or nested rule inside media query block");
                    return false;
                }
            }
        }
    } else {
        parser.error("expected '{' to start media query block");
    }

    return true;

}
