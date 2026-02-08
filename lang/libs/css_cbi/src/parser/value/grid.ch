func (cssParser : &mut CSSParser) parseGridTemplateTracks(
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
        } else if(token.type == TokenType.Identifier) {
             const view = token.value;
             if(view.equals("auto") || view.equals("min-content") || view.equals("max-content") || view.equals("fit-content")) {
                 parser.increment()
                 var kwKind = CSSKeywordKind.Auto
                 if(view.equals("min-content")) kwKind = CSSKeywordKind.MinContent
                 else if(view.equals("max-content")) kwKind = CSSKeywordKind.MaxContent
                 else if(view.equals("fit-content")) kwKind = CSSKeywordKind.FitContent
                 
                 var kwVal = builder.allocate<CSSKeywordValueData>()
                 new (kwVal) CSSKeywordValueData {
                     kind : kwKind,
                     value : builder.allocate_view(view)
                 }
                 trackValue.kind = CSSValueKind.Keyword
                 trackValue.data = kwVal
                 vals.values.push(trackValue)
                 continue
             }
        }
        
        // If we reach here, it's an unknown track or we should break
        // Support for repeat() could be added here
        break
    }
}

func (cssParser : &mut CSSParser) parseGridTemplateRows(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridTemplateTracks(parser, builder, value)
}

func (cssParser : &mut CSSParser) parseGridTemplateColumns(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseGridTemplateTracks(parser, builder, value)
}
