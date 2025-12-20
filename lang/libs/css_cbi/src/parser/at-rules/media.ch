
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
        parent : om.parent
    }

    om.media_queries.push(rule);

    if (parser.increment_if(TokenType.LBrace as int)) {
        while(true) {
            var decl = cssParser.parseDeclaration(parser, builder);
            if(decl) {
                rule.declarations.push(decl)
            } else {
                break;
            }
        }
        if (!parser.increment_if(TokenType.RBrace as int)) {
            parser.error("expected '}' after media query block");
        }
    } else {
        parser.error("expected '{' to start media query block");
    }

    return true;

}
