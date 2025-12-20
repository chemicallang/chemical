
func getWidthCSSKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("var") => { return CSSKeywordKind.Var }
        comptime_fnv1_hash("min-content") => { return CSSKeywordKind.MinContent }
        comptime_fnv1_hash("max-content") => { return CSSKeywordKind.MaxContent }
        comptime_fnv1_hash("fit-content") => { return CSSKeywordKind.FitContent }
        comptime_fnv1_hash("calc") => { return CSSKeywordKind.Calc }
        comptime_fnv1_hash("calc-size") => { return CSSKeywordKind.CalcSize }
        comptime_fnv1_hash("width") => { return CSSKeywordKind.Width }
        comptime_fnv1_hash("height") => { return CSSKeywordKind.Height }
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch }
        comptime_fnv1_hash("block") => { return CSSKeywordKind.Block }
        comptime_fnv1_hash("self-block") => { return CSSKeywordKind.SelfBlock }
        comptime_fnv1_hash("self-inline") => { return CSSKeywordKind.SelfInline }
        default => { return CSSKeywordKind.Unknown }
    }
}

func (cssParser : &mut CSSParser) parseFitContentCall(
     parser : *mut Parser,
     builder : *mut ASTBuilder,
     value : &mut CSSValue
) {

    const lpTok = parser.getToken()
    if(lpTok.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' for function call");
    }

    const funcData = builder.allocate<SingleLengthFuncCall>()
    new (funcData) SingleLengthFuncCall()

    value.kind = CSSValueKind.SingleLengthFunctionCall
    value.data = funcData

    funcData.name = CSSKeywordValueData { value : std::string_view("fit-content"), kind : CSSKeywordKind.FitContent }
    if(!cssParser.parseLengthInto(parser, builder, funcData.length)) {
        parser.error("expected a length for fit-content");
    }

    const rpTok = parser.getToken()
    if(rpTok.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' for function call");
    }

}

func (cssParser : &mut CSSParser) parseWidthOrHeightValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, token.value)
            return;
        }
        TokenType.Identifier => {
            const hash = token.fnv1()
            const kind = getWidthCSSKeywordKind(hash)
            switch(kind) {
                CSSKeywordKind.Var => {
                    parser.increment()
                    const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                    alloc_value_length_var(parser, builder, value, colorValue)
                    return;
                }
                CSSKeywordKind.CalcSize => {
                    parser.increment();
                    cssParser.parseCalc(parser, builder, value);
                    return;
                }
                CSSKeywordKind.Calc => {
                    parser.increment();
                    cssParser.parseCalc(parser, builder, value);
                    return;
                }
                CSSKeywordKind.FitContent => {
                    parser.increment()
                    const next = parser.getToken()
                    if(next.type == TokenType.LParen) {
                        cssParser.parseFitContentCall(parser, builder, value)
                    } else {
                        alloc_value_keyword(builder, value, kind, token.value);
                        return;
                    }
                }
                CSSKeywordKind.Unknown => {
                    parser.error("unknown value given for width/height");
                }
                default => {
                    parser.increment();
                    alloc_value_keyword(builder, value, kind, token.value);
                    return;
                }
            }
        }
        default => {
            parser.error("unknown value given for width/height");
        }
    }

}

func (cssParser : &mut CSSParser) parseWidth(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseWidthOrHeightValue(parser, builder, value)
}

func (cssParser : &mut CSSParser) parseHeight(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseWidthOrHeightValue(parser, builder, value)
}