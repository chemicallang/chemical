func (cssParser : &mut CSSParser) parseTextShadow(
    parser  : *mut Parser,
    builder : *mut ASTBuilder,
    value   : &mut CSSValue
) {
    // Allocate a new text-shadow value.
    var shadow = builder.allocate<CSSTextShadowValueData>();
    new (shadow) CSSTextShadowValueData();

    value.kind = CSSValueKind.TextShadow;
    value.data = shadow;

    // Special case: "none" means no shadow.
    const token = parser.getToken();
    if (token.type == TokenType.Identifier && token.value.equals("none")) {
        parser.increment();
        return;
    }

    // Otherwise, text-shadow is a comma-separated list of shadows.
    while (true) {

        // We'll collect length values in an array. The spec requires 2 or 3 lengths.
        var lengths : CSSValue[] = { CSSValue(), CSSValue(), CSSValue() };  // dynamic array of CSSValue

        var lenInd = 0;

        // For one shadow, process tokens until a comma or semicolon is reached.
        while (true) {

            const currTok = parser.getToken();

            // End of this shadow's tokens.
            if (currTok.type == TokenType.Comma || currTok.type == TokenType.Semicolon) {
                break;
            }

            // If the token is a length, parse it and add it.
            if (cssParser.parseLength(parser, builder, lengths[lenInd])) {
                lenInd++
                continue;
            }

            // Only parse one color per shadow.
            if (shadow.color.kind == CSSValueKind.Unknown) {
                cssParser.parseCSSColor(parser, builder, shadow.color);
                continue;
            }

            // If token isn't recognized as length or color, break out.
            break;

        }

        // Validate that we have at least 2 lengths (offset-x and offset-y).
        if (lengths[0].kind == CSSValueKind.Unknown && lengths[1].kind == CSSValueKind.Unknown) {
            parser.error("expected at least two length values for text-shadow");
        }

        // The first two lengths are always the offsets.
        shadow.offsetX = lengths[0];
        shadow.offsetY = lengths[1];

        // If a third length exists, that's the blur radius.
        if (lengths[2].kind != CSSValueKind.Unknown) {
            shadow.blurRadius = lengths[2];
        }

        // Check if there's a comma separator before the next shadow.
        const nextTok = parser.getToken();
        if (nextTok.type == TokenType.Comma) {

            parser.increment(); // skip the comma and continue with next shadow

            const nextShadow = builder.allocate<CSSTextShadowValueData>()
            new (nextShadow) CSSTextShadowValueData()

            shadow.next = nextShadow
            shadow = nextShadow

        } else {
            break;
        }
    }
}