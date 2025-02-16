func (cssParser : &mut CSSParser) parseBorderRadius(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const ptr = builder.allocate<CSSBorderRadiusValueData>();
    if(cssParser.parseLengthInto(parser, builder, ptr.first)) {

        cssParser.parseLengthInto(parser, builder, ptr.second)
        cssParser.parseLengthInto(parser, builder, ptr.third)
        cssParser.parseLengthInto(parser, builder, ptr.fourth)

        ptr.next = null

        const tok = parser.getToken()
        if(tok.type == TokenType.Divide) {
            parser.increment();

            const next = builder.allocate<CSSBorderRadiusValueData>();
            next.next = null;

            if(cssParser.parseLengthInto(parser, builder, next.first)) {

                cssParser.parseLengthInto(parser, builder, next.second)
                cssParser.parseLengthInto(parser, builder, next.third)
                cssParser.parseLengthInto(parser, builder, next.fourth)

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