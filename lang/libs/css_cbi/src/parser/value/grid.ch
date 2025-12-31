
func (cssParser : &mut CSSParser) parseGridTemplateRows(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var vals = builder.allocate<CSSMultipleValues>()
    new (vals) CSSMultipleValues {
        values : std::vector<CSSValue>()
    }
    value.kind = CSSValueKind.Multiple
    value.data = vals

    while(true) {
        const token = parser.getToken()
        if(token.type == TokenType.Semicolon || token.type == TokenType.RBrace) {
            break
        }
        
        var trackValue = CSSValue()
        if(cssParser.parseLength(parser, builder, trackValue)) {
            vals.values.push(trackValue)
            continue
        } else if(token.type == TokenType.Identifier && token.value.equals("auto")) {
             parser.increment()
             var autoVal = builder.allocate<CSSKeywordValueData>()
             new (autoVal) CSSKeywordValueData {
                 kind : CSSKeywordKind.Auto,
                 value : token.value
             }
             trackValue.kind = CSSValueKind.Keyword
             trackValue.data = autoVal
             vals.values.push(trackValue)
             continue
        } else {
            // Support repeat() or other functions if needed later
            // For now, if we can't parse it as length or auto, error/break
            parser.error("unexpected token in grid-template-rows")
            break
        }
    }
}
