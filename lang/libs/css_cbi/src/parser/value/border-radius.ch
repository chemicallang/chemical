
func (cssParser : &mut CSSParser) parseBorderRadius(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const ptr = builder.allocate<CSSBorderRadiusValueData>();
    new (ptr) CSSBorderRadiusValueData {
        first : CSSValue()
        second : CSSValue()
        third : CSSValue()
        fourth : CSSValue()
        next : null
    }
    if(cssParser.parseLength(parser, builder, &mut ptr.first)) {

        cssParser.parseLength(parser, builder, &mut ptr.second)
        cssParser.parseLength(parser, builder, &mut ptr.third)
        cssParser.parseLength(parser, builder, &mut ptr.fourth)

        const tok = parser.getToken()
        if(tok.type == TokenType.Divide) {
            parser.increment();

            const next = builder.allocate<CSSBorderRadiusValueData>();
            new (next) CSSBorderRadiusValueData {
                first : CSSValue()
                second : CSSValue()
                third : CSSValue()
                fourth : CSSValue()
                next : null
            }

            if(cssParser.parseLength(parser, builder, &mut next.first)) {

                cssParser.parseLength(parser, builder, &mut next.second)
                cssParser.parseLength(parser, builder, &mut next.third)
                cssParser.parseLength(parser, builder, &mut next.fourth)

                ptr.next = next;

            } else {
                parser.error("expected a length value");
            }

        }

        value.kind = CSSValueKind.BorderRadius
        value.data = ptr;

    } else {
        parser.error("expected a length value");
    }

}
