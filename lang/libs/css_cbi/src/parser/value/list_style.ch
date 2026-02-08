func (cssParser : &mut CSSParser) parseListStyle(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var ls = builder.allocate<CSSListStyleValueData>()
    new (ls) CSSListStyleValueData {
        type = CSSKeywordValueData { kind = CSSKeywordKind.Unknown },
        position = CSSKeywordValueData { kind = CSSKeywordKind.Unknown },
        image = CSSValue()
    }
    value.kind = CSSValueKind.ListStyle
    value.data = ls

    while(true) {
        const token = parser.getToken()
        if(token.type == TokenType.Semicolon || token.type == TokenType.RBrace) break

        if(token.type == TokenType.Identifier) {
            const hash = token.fnv1()
            // Simplified: if it's 'none', it could be type
            if(token.value.equals("none")) {
                parser.increment()
                ls.type = CSSKeywordValueData { kind = CSSKeywordKind.None, value = builder.allocate_view(token.value) }
            } else if(token.value.equals("inside") || token.value.equals("outside")) {
                parser.increment()
                ls.position = CSSKeywordValueData { kind = CSSKeywordKind.Auto, value = builder.allocate_view(token.value) }
            } else {
                // Assume it's list-style-type
                parser.increment()
                ls.type = CSSKeywordValueData { kind = CSSKeywordKind.Auto, value = builder.allocate_view(token.value) }
            }
        } else {
            // list-style-image? url(...)
            if(!cssParser.parseRandomValue(parser, builder, ls.image)) {
                break
            }
        }
    }
}
