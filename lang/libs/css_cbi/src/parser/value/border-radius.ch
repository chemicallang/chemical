
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
    if(cssParser.parseLength(parser, builder, ptr.first)) {

        cssParser.parseLength(parser, builder, ptr.second)
        cssParser.parseLength(parser, builder, ptr.third)
        cssParser.parseLength(parser, builder, ptr.fourth)

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

            if(cssParser.parseLength(parser, builder, next.first)) {

                cssParser.parseLength(parser, builder, next.second)
                cssParser.parseLength(parser, builder, next.third)
                cssParser.parseLength(parser, builder, next.fourth)

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
