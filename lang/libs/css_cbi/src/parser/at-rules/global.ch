func (cssParser : &mut CSSParser) parseGlobalBlock(
    parser : *mut Parser,
    builder : *mut ASTBuilder
) : GlobalBlock {

    var block = GlobalBlock()

    const lb = parser.getToken()
    if(lb.type == TokenType.LBrace) {
        parser.increment()
    } else {
        parser.error("expected a '{' for the global block")
        return block;
    }

    while(true) {
        const set = cssParser.parseCSSRuleSet(parser, builder)
        if(set.selectors.empty()) {
            break;
        } else {
            block.rules.push(set)
        }
    }

    const rb = parser.getToken()
    if(rb.type == TokenType.RBrace) {
        parser.increment()
    } else {
        parser.error("expected a '}' for the global block")
        return block;
    }

    return block;

}