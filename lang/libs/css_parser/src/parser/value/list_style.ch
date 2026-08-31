func (cssParser : &mut CSSParser) parseListStyle(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseRawPropertyValue(parser, builder, value)
}
