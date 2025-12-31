
func (cssParser : &mut CSSParser) parseOutline(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    var outline = builder.allocate<CSSOutlineValueData>()
    new (outline) CSSOutlineValueData {
        width : CSSValue(),
        style : CSSValue(),
        color : CSSValue()
    }

    value.kind = CSSValueKind.Outline
    value.data = outline

    var has_length = false;
    var has_style = false;

    var i = -1;
    while(i < 3) {
        i++;
        const token = parser.getToken();
        switch(token.type) {
            TokenType.Number => {
                if(!cssParser.parseLength(parser, builder, outline.width)) {
                    parser.error("expected a length in outline");
                }
                has_length = true;
            }
            TokenType.Identifier => {
                if(token.value.equals("var")) {
                    parser.increment();
                    const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                    var valueRef : *mut CSSValue
                    if(has_length) {
                        if(has_style) {
                            valueRef = &mut outline.color
                        } else {
                            valueRef = &mut outline.style
                        }
                    } else {
                        valueRef = &mut outline.width
                    }
                    alloc_value_length_var(parser, builder, *valueRef, colorValue)
                } else {
                    const style = getOutlineStyleKeywordKind(token.fnv1())
                    if(style != CSSKeywordKind.Unknown) {
                        parser.increment()
                        alloc_value_keyword(builder, outline.style, style, token.value)
                        if(has_style) {
                            parser.error("there should be a single style in outline")
                        }
                        has_style = true;
                        continue;
                    } else {
                        const width = getLineWidthKeyword(token.value);
                        if(width != CSSKeywordKind.Unknown) {
                            parser.increment()
                            alloc_value_keyword(builder, outline.width, width, token.value)
                            continue;
                        }
                    }
                    if(cssParser.parseCSSColor(parser, builder, outline.color)) {
                        return;
                    } else {
                        parser.error("expected a css color");
                    }
                }
            }
            TokenType.Semicolon => {
                return;
            }
            default => {
                if(!cssParser.parseCSSColor(parser, builder, outline.color)) {
                    parser.error("unknown value token in outline");
                }
            }
        }
    }

}
