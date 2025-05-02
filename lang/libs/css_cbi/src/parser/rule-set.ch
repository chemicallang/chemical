func (cssParser : &mut CSSParser) parseCSSRuleSet(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : CSSRuleSet {

    var set = CSSRuleSet()

    while(true) {
        const selector = cssParser.parseCSSSelector(parser, builder)
        if(selector != null) {
            set.selectors.push(selector)
            const next = parser.getToken()
            if(next.type == TokenType.Comma) {
                parser.increment()
            } else {
                break;
            }
        } else {
            break;
        }
    }

    const lb = parser.getToken()
    if(lb.type == TokenType.LBrace) {
        parser.increment()
    } else {
        parser.error("expected a '{' after the selector(s) for rule");
        return set
    }

    while(true) {
        var decl = cssParser.parseDeclaration(parser, builder);
        if(decl) {
            set.declarations.push(decl)
        } else {
            break;
        }
    }

    const rb = parser.getToken()
    if(rb.type == TokenType.RBrace) {
        parser.increment()
    } else {
        parser.error("expected a '}' after the declaration(s) for rule");
        return set
    }

    return set;

}