import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"

func getLineStyleKeyword(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("dotted") => { return CSSKeywordKind.Dotted }
        comptime_fnv1_hash("dashed") => { return CSSKeywordKind.Dashed }
        comptime_fnv1_hash("solid") => { return CSSKeywordKind.Solid }
        comptime_fnv1_hash("double") => { return CSSKeywordKind.Double }
        comptime_fnv1_hash("groove") => { return CSSKeywordKind.Groove }
        comptime_fnv1_hash("ridge") => { return CSSKeywordKind.Ridge }
        comptime_fnv1_hash("inset") => { return CSSKeywordKind.Inset }
        comptime_fnv1_hash("outset") => { return CSSKeywordKind.Outset }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getLineWidthKeyword(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("thin") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("medium") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("thick") => { return CSSKeywordKind.Dotted }
        default => { return CSSKeywordKind.Unknown }
    }
}

func (cssParser : &mut CSSParser) parseBorder(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    var border = builder.allocate<CSSBorderValueData>()
    new (border) CSSBorderValueData {
        width : CSSValue(),
        style : CSSValue(),
        color : CSSValue()
    }

    value.kind = CSSValueKind.Border
    value.data = border


    var has_style = false;

    var i = -1;
    while(i < 3) {
        i++;
        const token = parser.getToken();
        switch(token.type) {
            TokenType.Number => {
                if(!cssParser.parseLength(parser, builder, border.width)) {
                    parser.error("expected a length in border");
                }
            }
            TokenType.Identifier => {
                const style = getLineStyleKeyword(token.value.data())
                if(style != CSSKeywordKind.Unknown) {
                    parser.increment()
                    alloc_value_keyword(builder, border.style, style, token.value)
                    if(has_style) {
                        parser.error("there should be a single style in border")
                    }
                    has_style = true;
                    continue;
                } else {
                    const width = getLineWidthKeyword(token.value.data());
                    if(width != CSSKeywordKind.Unknown) {
                        parser.increment()
                        alloc_value_keyword(builder, border.width, width, token.value)
                        continue;
                    }
                }
                if(cssParser.isColor(token.value)) {
                    parser.increment();
                    alloc_named_color(builder, border.color, token.value);
                    return;
                } else {
                    parser.error("unknown value given");
                }
            }
            TokenType.Semicolon => {
                return;
            }
            default => {
                if(!cssParser.parseCSSColor(parser, builder, border.color)) {
                    parser.error("unknown value token in border");
                }
            }
        }
    }

}