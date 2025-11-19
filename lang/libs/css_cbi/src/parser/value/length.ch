
func getLengthKind(view : &std::string_view) : CSSLengthKind {
    switch(fnv1_hash_view(view)) {
        comptime_fnv1_hash("px") => {
            return CSSLengthKind.LengthPX
        }
        comptime_fnv1_hash("em") => {
            return CSSLengthKind.LengthEM
        }
        comptime_fnv1_hash("rem") => {
            return CSSLengthKind.LengthREM
        }
        comptime_fnv1_hash("vh") => {
            return CSSLengthKind.LengthVH
        }
        comptime_fnv1_hash("vw") => {
            return CSSLengthKind.LengthVW
        }
        comptime_fnv1_hash("vmin") => {
            return CSSLengthKind.LengthVMIN
        }
        comptime_fnv1_hash("vmax") => {
            return CSSLengthKind.LengthVMAX
        }
        comptime_fnv1_hash("cm") => {
            return CSSLengthKind.LengthCM
        }
        comptime_fnv1_hash("mm") => {
            return CSSLengthKind.LengthMM
        }
        comptime_fnv1_hash("in") => {
            return CSSLengthKind.LengthIN
        }
        comptime_fnv1_hash("pt") => {
            return CSSLengthKind.LengthPT
        }
        comptime_fnv1_hash("pc") => {
            return CSSLengthKind.LengthPC
        }
        comptime_fnv1_hash("ch") => {
            return CSSLengthKind.LengthCH
        }
        comptime_fnv1_hash("ex") => {
            return CSSLengthKind.LengthEX
        }
        comptime_fnv1_hash("s") => {
            return CSSLengthKind.LengthS
        }
        comptime_fnv1_hash("ms") => {
            return CSSLengthKind.LengthMS
        }
        comptime_fnv1_hash("Hz") => {
            return CSSLengthKind.LengthHZ
        }
        comptime_fnv1_hash("kHz") => {
            return CSSLengthKind.LengthKHZ
        }
        comptime_fnv1_hash("deg") => {
            return CSSLengthKind.LengthDEG
        }
        comptime_fnv1_hash("rad") => {
            return CSSLengthKind.LengthRAD
        }
        comptime_fnv1_hash("grad") => {
            return CSSLengthKind.LengthGRAD
        }
        comptime_fnv1_hash("turn") => {
            return CSSLengthKind.LengthTURN
        }
        default => {
            return CSSLengthKind.Unknown
        }
    }
}

func parseLengthKindSafe(parser : *mut Parser, builder : *mut ASTBuilder) : CSSLengthKind {
    const token = parser.getToken();
    if(token.type == TokenType.Percentage) {
        parser.increment();
        return CSSLengthKind.LengthPERCENTAGE
    } else if(token.type == TokenType.Identifier) {
        const k = getLengthKind(token.value)
        if(k != CSSLengthKind.Unknown) {
            parser.increment();
        }
        return k;
    } else {
        return CSSLengthKind.Unknown
    }
}

func parseLengthKind(parser : *mut Parser, builder : *mut ASTBuilder) : CSSLengthKind {
    const token = parser.getToken();
    if(token.type == TokenType.Percentage) {
        parser.increment();
        return CSSLengthKind.LengthPERCENTAGE
    } else if(token.type == TokenType.Identifier) {
        parser.increment();
        const kind = getLengthKind(token.value)
        if(kind != CSSLengthKind.Unknown) {
            return kind;
        } else {
            parser.error("unknown length unit");
            return CSSLengthKind.LengthPX
        }
    } else {
        parser.error("unknown unit token found");
        parser.increment()
        return CSSLengthKind.LengthPX
    }
}

func (cssParser : &mut CSSParser) parseLengthInto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    length : &mut CSSLengthValueData,
    required_unit : bool = true
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            length.value = builder.allocate_view(token.value)
            const lenKind = parseLengthKindSafe(parser, builder)
            if(lenKind != CSSLengthKind.Unknown) {
                length.kind = lenKind
            } else {
                if(token.value.equals("0")) {
                    length.kind = CSSLengthKind.None
                } else {
                    if(required_unit) {
                        parser.error("expected a unit after number");
                    } else {
                        length.kind = CSSLengthKind.None
                    }
                }
            }
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                length.value = cssParser.parseCSSVariableFunc(parser, builder)
                return true;
            } else {
                length.value = std::string_view("")
                length.kind = CSSLengthKind.Unknown
                return false;
            }
        }
        default => {
            length.value = std::string_view("")
            length.kind = CSSLengthKind.Unknown
            return false;
        }
    }
}

func (cssParser: &mut CSSParser) parseNumberInto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    length : &mut CSSLengthValueData
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            length.value = builder.allocate_view(token.value)
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                length.value = cssParser.parseCSSVariableFunc(parser, builder)
                return true;
            } else {
                length.value = std::string_view("")
                length.kind = CSSLengthKind.Unknown
                return false;
            }
        }
        default => {
            length.value = std::string_view("")
            length.kind = CSSLengthKind.Unknown
            return false;
        }
    }
}

func (cssParser: &mut CSSParser) parseNumberOrLengthInto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    length : &mut CSSLengthValueData
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            length.value = builder.allocate_view(token.value)
            const lengthKind = parseLengthKindSafe(parser, builder);
            if(lengthKind == CSSLengthKind.Unknown) {
                length.kind = CSSLengthKind.None
            } else {
                length.kind = lengthKind
            }
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                length.value = cssParser.parseCSSVariableFunc(parser, builder)
                return true;
            } else {
                length.value = std::string_view("")
                length.kind = CSSLengthKind.Unknown
                return false;
            }
        }
        default => {
            length.value = std::string_view("")
            length.kind = CSSLengthKind.Unknown
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseLength(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {

    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, token.value)
            return true;
        }
        TokenType.LBrace => {
            cssParser.parseChemValueAfterLBrace(parser, builder, value)
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                alloc_value_length_var(parser, builder, value, colorValue)
                return true;
            } else {
                return false;
            }
        }
        default => {
            return false;
        }
    }

}

func (cssParser : &mut CSSParser) parseNumberOrLength(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_num_length(parser, builder, value, token.value)
            return true;
        }
        TokenType.LBrace => {
            cssParser.parseChemValueAfterLBrace(parser, builder, value)
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                alloc_value_length_var(parser, builder, value, colorValue)
                return true;
            } else {
                return false;
            }
        }
        default => {
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseNumberOrAuto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_number(builder, value, token.value)
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                alloc_value_length_var(parser, builder, value, colorValue)
                return true;
            } else if(token.value.equals("auto")) {
                parser.increment();
                alloc_value_keyword(builder, value, CSSKeywordKind.Auto, token.value);
                return true;
            } else {
                return false;
            }
        }
        TokenType.LBrace => {
            cssParser.parseChemValueAfterLBrace(parser, builder, value)
            return true;
        }
        default => {
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseLengthOrAuto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, token.value)
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("var")) {
                parser.increment()
                const colorValue = cssParser.parseCSSVariableFunc(parser, builder)
                alloc_value_length_var(parser, builder, value, colorValue)
                return true;
            } else if(token.value.equals("auto")) {
                parser.increment();
                alloc_value_keyword(builder, value, CSSKeywordKind.Auto, token.value);
                return true;
            } else {
                return false;
            }
        }
        TokenType.LBrace => {
            cssParser.parseChemValueAfterLBrace(parser, builder, value)
            return true;
        }
        default => {
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseMarginSingle(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if(!cssParser.parseLengthOrAuto(parser, builder, value)) {
        parser.error("unknown value for margin");
    }
}

func (cssParser : &mut CSSParser) parseZIndex(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if(!cssParser.parseNumberOrAuto(parser, builder, value)) {
        parser.error("unknown value for z-index");
    }
}