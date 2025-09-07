func (cssParser : &mut CSSParser) parseValuePair(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const pair = builder.allocate<CSSValuePair>()
    new (pair) CSSValuePair()

    value.kind = CSSValueKind.Pair
    value.data = pair

    if(cssParser.parseLength(parser, builder, pair.first)) {

        // optional
        cssParser.parseLength(parser, builder, pair.second)

    }
}

func (cssParser : &mut CSSParser) parseGap(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseValuePair(parser, builder, value)
}