func (cssParser : &mut CSSParser) parseValuePair(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
        unsafe {
        const pair = builder.allocate<CSSValuePair>()
        new (pair) CSSValuePair()

        value.kind = CSSValueKind.Pair
        value.data = pair

        if(cssParser.parseLength(parser, builder, &mut pair.first)) {

            // optional
            cssParser.parseLength(parser, builder, &mut pair.second)

        }

        }
    }

func (cssParser : &mut CSSParser) parseGap(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
        unsafe {
        cssParser.parseValuePair(parser, builder, value)

        }
    }