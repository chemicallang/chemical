
func (cssParser : &mut CSSParser) parseBorderRadius(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const ptr = builder.allocate<CSSBorderRadiusValueData>();
    if(parser.parseLengthInto(builder, ptr.first)) {

        parser.parseLengthInto(builder, ptr.second)
        parser.parseLengthInto(builder, ptr.third)
        parser.parseLengthInto(builder, ptr.fourth)

        ptr.next = null

        const tok = parser.getToken()
        if(tok.type == TokenType.Divide) {
            parser.increment();

            const next = builder.allocate<CSSBorderRadiusValueData>();
            next.next = null;

            if(parser.parseLengthInto(builder, next.first)) {

                parser.parseLengthInto(builder, next.second)
                parser.parseLengthInto(builder, next.third)
                parser.parseLengthInto(builder, next.fourth)

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