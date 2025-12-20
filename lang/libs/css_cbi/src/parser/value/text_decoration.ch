
func (cssParser : &mut CSSParser) parseTextDecorationColor(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
     if(!cssParser.parseCSSColor(parser, builder, value)) {
         parser.error("expected a color for text-decoration-color")
     }
}

func (cssParser : &mut CSSParser) parseTextDecorationThickness(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken()
    if(token.value.equals("auto") || token.value.equals("from-font")) {
        parser.increment()
        alloc_value_keyword(builder, value, (if(token.value.equals("auto")) CSSKeywordKind.Auto else CSSKeywordKind.FromFont), token.value)
    } else {
        if(!cssParser.parseLength(parser, builder, value)) {
            parser.error("expected a length, auto or from-font for text-decoration-thickness")
        }
    }
}

func (cssParser : &mut CSSParser) parseTextDecoration(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var data = builder.allocate<CSSTextDecorationValueData>()
    new (data) CSSTextDecorationValueData {
        line : CSSValue(),
        style : CSSValue(),
        color : CSSValue(),
        thickness : CSSValue()
    }

    value.kind = CSSValueKind.TextDecoration
    value.data = data

    var i = 0;
    while(i < 4) {
        i++;
        const token = parser.getToken();
        if(token.type == TokenType.Semicolon || token.type == TokenType.RBrace) {
            return;
        }

        // Try line keyword
        const line = getTextDecorationLineKeywordKind(token.fnv1())
        if(line != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, data.line, line, token.value)
            continue;
        }

        // Try style keyword
        const style = getTextDecorationStyleKeywordKind(token.fnv1())
        if(style != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, data.style, style, token.value)
            continue;
        }

        // Try thickness keyword or length
        if(token.value.equals("auto") || token.value.equals("from-font")) {
             parser.increment()
             const kwKind = if(token.value.equals("auto")) CSSKeywordKind.Auto else CSSKeywordKind.FromFont
             alloc_value_keyword(builder, data.thickness, kwKind, token.value)
             continue;
        }
        
        // Try length (thickness)
        if(cssParser.parseLength(parser, builder, data.thickness)) {
            continue;
        }

        // Try color
        if(cssParser.parseCSSColor(parser, builder, data.color)) {
            continue;
        }

        // Something else?
        break;
    }
}
