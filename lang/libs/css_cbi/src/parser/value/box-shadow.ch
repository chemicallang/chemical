
func (cssParser : &mut CSSParser) parseBoxShadow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    // Allocate a new box-shadow value.
    var shadow = builder.allocate<CSSBoxShadowValueData>();
    new (shadow) CSSBoxShadowValueData()

    value.kind = CSSValueKind.BoxShadow;
    value.data = shadow;

    // Special case: "none" means no shadow.
    const token = parser.getToken();
    if(token.type == TokenType.Identifier && token.value.equals("none")) {
        parser.increment();
        // boxShadow.shadows remains empty.
        return;
    }

    while(true) {

        var lengths : CSSValue[] = [ CSSValue(), CSSValue(), CSSValue(), CSSValue() ]

        var lenInd = 0;

        while(true) {

            const currTok = parser.getToken();

            // Break out if we hit a comma or semicolon (end of this shadow).
            if(currTok.type == TokenType.Comma || currTok.type == TokenType.Semicolon) {
                break;
            }

            // Check if this token is a length.
            if(cssParser.parseLength(parser, builder, lengths[lenInd])) {
                lenInd++;
                continue; // Process next token.
            }

            // We allow only one color per shadow.
            if(shadow.color.kind == CSSValueKind.Unknown && cssParser.parseCSSColor(parser, builder, shadow.color)) {
                continue;
            }

            // Check if this token is a position keyword.
            if(currTok.type == TokenType.Identifier) {
                if(currTok.value.equals("inset")) {
                    shadow.inset = true;
                    parser.increment();
                    continue;
                }
                if(currTok.value.equals("outset")) {
                    // 'outset' is allowed but typically treated as default (non-inset)
                    shadow.inset = false;
                    parser.increment();
                    continue;
                }
            }

            break;

        }

        // Per the syntax, the <box-shadow-offset> is required and consists of two lengths.
        if(lengths[0].kind == CSSValueKind.Unknown && lengths[1].kind == CSSValueKind.Unknown) {
            parser.error("expected at least two lengths (offset-x and offset-y) for box-shadow");
        }
        // The first two lengths are always the offsets.
        shadow.offsetX = lengths[0];
        shadow.offsetY = lengths[1];
        // If a third length is provided, it is the blur radius.
        if(lengths[2].kind != CSSValueKind.Unknown) {
            shadow.blurRadius = lengths[2];
        }
        // If a fourth length is provided, it is the spread radius.
        if(lengths[3].kind != CSSValueKind.Unknown) {
            shadow.spreadRadius = lengths[3];
        }

        // If the next token is a comma, then there is another shadow.
        const nextTok = parser.getToken();
        if(nextTok.type == TokenType.Comma) {

            parser.increment(); // skip the comma and continue

            // create another shadow
            var nextShadow = builder.allocate<CSSBoxShadowValueData>();
            new (nextShadow) CSSBoxShadowValueData()

            shadow.next = nextShadow
            shadow = nextShadow

        } else {
            break;
        }

    }

}