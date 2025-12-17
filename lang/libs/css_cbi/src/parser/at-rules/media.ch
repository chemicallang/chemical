
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

    var query_str = std::string();

    while(true) {
        token = parser.getToken();
        const type = token.type;
        
        if(type == TokenType.LBrace) {
            break;
        }
        
        if(type == TokenType.EndOfFile) {
            parser.error("unexpected end of file while parsing media query");
            return false;
        }
        
        query_str.append_view(token.value);
        query_str.append(' ');
        
        parser.increment();
    }
    
    const view = std::string_view(query_str.data(), query_str.size())
    const allocated_query = builder.allocate_view(view);

    var rule = builder.allocate<CSSMediaRule>();
    new (rule) CSSMediaRule {
        query : allocated_query,
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
