
func (cssParser : &mut CSSParser) parseHexColor(parser : *mut Parser, builder : *mut ASTBuilder, colorView : &std::string_view, value : &mut CSSValue) {
    // After the #, only 3, 4, 6, or 8 hexadecimal digits are allowed
    const hex_size = colorView.size() - 1;
    if(hex_size != 3 && hex_size != 4 && hex_size != 6 && hex_size != 8) {
        parser.error("hex color digits length must be 3,4,6 or 8");
        return;
    }
    var col_value = builder.allocate<CSSColorValueData>();
    col_value.kind = CSSColorKind.HexColor
    col_value.value.view = builder.allocate_view(colorView)
    value.kind = CSSValueKind.Color
    value.data = col_value
    var out : uint32_t = 0
    if(!parse_css_hex_color(colorView.data() + 1, colorView.size() - 1, &out)) {
        parser.error("hash color is not valid");
    }
}

func isSystemColor(hash : size_t) : bool {
    switch(hash) {
        comptime_fnv1_hash("accentcolor") => { return true; }
        comptime_fnv1_hash("accentcolortext") => { return true; }
        comptime_fnv1_hash("activetext") => { return true; }
        comptime_fnv1_hash("buttonborder") => { return true; }
        comptime_fnv1_hash("buttonface") => { return true; }
        comptime_fnv1_hash("buttontext") => { return true; }
        comptime_fnv1_hash("canvas") => { return true; }
        comptime_fnv1_hash("canvastext") => { return true; }
        comptime_fnv1_hash("field") => { return true; }
        comptime_fnv1_hash("fieldtext") => { return true; }
        comptime_fnv1_hash("graytext") => { return true; }
        comptime_fnv1_hash("highlight") => { return true; }
        comptime_fnv1_hash("highlighttext") => { return true; }
        comptime_fnv1_hash("linktext") => { return true; }
        comptime_fnv1_hash("mark") => { return true; }
        comptime_fnv1_hash("marktext") => { return true; }
        comptime_fnv1_hash("selecteditem") => { return true; }
        comptime_fnv1_hash("selecteditemtext") => { return true; }
        comptime_fnv1_hash("visitedtext") => { return true; }
        default => { return false; }
    }
}

func hashSmallColorValue(view : &std::string_view) : size_t {

    const size = view.size();
    if(size > 25) {
        return 0;
    }

    // copy the token value
    var arr : char[27] = {}
    strncpy(&arr[0], view.data(), size);
    arr[size] = '\0'

    // lowercase it
    var i = 0;
    while(i < size) {
        arr[i] = tolower(arr[i] as int)
        i++;
    }

    return fnv1_hash(&arr[0]);

}

func getHashColorKind(hash : size_t) : CSSColorKind {
    if(isSystemColor(hash)) {
        return CSSColorKind.SystemColor
    } else {
        switch(hash) {
            comptime_fnv1_hash("transparent") => {
                return CSSColorKind.Transparent
            }
            comptime_fnv1_hash("currentcolor") => {
                return CSSColorKind.CurrentColor
            }
            comptime_fnv1_hash("rgb") => {
                return CSSColorKind.RGB
            }
            comptime_fnv1_hash("rgba") => {
                return CSSColorKind.RGBA
            }
            comptime_fnv1_hash("hsl") => {
                return CSSColorKind.HSL
            }
            comptime_fnv1_hash("hsla") => {
                return CSSColorKind.HSLA
            }
            comptime_fnv1_hash("hwb") => {
                return CSSColorKind.HWB
            }
            comptime_fnv1_hash("lab") => {
                return CSSColorKind.LAB
            }
            comptime_fnv1_hash("lch") => {
                return CSSColorKind.LCH
            }
            comptime_fnv1_hash("oklab") => {
                return CSSColorKind.OKLAB
            }
            comptime_fnv1_hash("oklch") => {
                return CSSColorKind.OKLCH
            }
            comptime_fnv1_hash("color") => {
                return CSSColorKind.COLOR
            }
            default => {
                return CSSColorKind.Unknown;
            }
        }
    }
}

func (cssParser : &mut CSSParser) getIdentifierColorKind(view : &std::string_view) : CSSColorKind {
    if(cssParser.isNamedColor(view)) {
        return CSSColorKind.NamedColor
    } else {
        return getHashColorKind(hashSmallColorValue(view))
    }
}

func isStrSystemColor(view : &std::string_view) : bool {
    // all system colors are less than 25 characters in length
    if(view.size() > 25) {
        return false;
    }
    return isSystemColor(hashSmallColorValue(view))
}

func (parser : &mut Parser) parseNumberOrAngleOrNone(builder : *mut ASTBuilder) : CSSLengthValueData {
    const token = parser.getToken()
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            const lenKind = parseLengthKindSafe(&parser, builder)
            if(lenKind == CSSLengthKind.Unknown) {
                return CSSLengthValueData { kind : CSSLengthKind.None, value : builder.allocate_view(token.value) }
            } else {
                return CSSLengthValueData { kind : lenKind, value : builder.allocate_view(token.value) }
            }
        }
        TokenType.Identifier => {
            if(token.value.equals("none")) {
                parser.increment()
                return CSSLengthValueData { kind : CSSLengthKind.None, value : "none" }
            } else {
                break;
            }
        }
        default => {
            break;
        }
    }
    return CSSLengthValueData { kind : CSSLengthKind.Unknown, value : std::string_view() }
}

func (parser : &mut Parser) parseNumberOrPercentageOrNone(builder : *mut ASTBuilder) : CSSLengthValueData {
    const token = parser.getToken()
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            if(parser.getToken().type == TokenType.Percentage) {
                parser.increment();
                return CSSLengthValueData { kind : CSSLengthKind.LengthPERCENTAGE, value : builder.allocate_view(token.value) }
            } else {
                return CSSLengthValueData { kind : CSSLengthKind.None, value : builder.allocate_view(token.value) }
            }
        }
        TokenType.Identifier => {
            if(token.value.equals("none")) {
                parser.increment()
                return CSSLengthValueData { kind : CSSLengthKind.None, value : "none" }
            } else {
                break;
            }
        }
        default => {
            break;
        }
    }
    return CSSLengthValueData { kind : CSSLengthKind.Unknown, value : std::string_view() }
}

// this should be called after incrementing the 'rgb' or 'rgba' token
func (cssParser : &mut CSSParser) parseRGBColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSRGBColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'rgb'")
    }

    data.red = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.green = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.blue = parser.parseNumberOrPercentageOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'rgb' arguments")
    }
}

// this should be called after incrementing the 'hsl' or 'hsla' token
func (cssParser : &mut CSSParser) parseHSLColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSHSLColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'hsl'")
    }

    data.hue = parser.parseNumberOrAngleOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.saturation = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.lightness = parser.parseNumberOrPercentageOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'hsl' arguments")
    }

}

func (cssParser : &mut CSSParser) parseHWBColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSHWBColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'hwb'")
    }

    data.hue = parser.parseNumberOrAngleOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.whiteness = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.blackness = parser.parseNumberOrPercentageOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'hwb' arguments")
    }

}

// this should be called after incrementing the 'rgb' or 'rgba' token
func (cssParser : &mut CSSParser) parseLABColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSLABColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'lab'")
    }

    data.lightness = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.rgAxis = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.byAxis = parser.parseNumberOrPercentageOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'lab' arguments")
    }
}

func (cssParser : &mut CSSParser) parseLCHColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSLCHColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'lch'")
    }

    data.lightness = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.chroma = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.hue = parser.parseNumberOrAngleOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'lch' arguments")
    }

}

func (cssParser : &mut CSSParser) parseOKLABColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSOKLABColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'lch'")
    }

    data.lightness = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.aAxis = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.bAxis = parser.parseNumberOrPercentageOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'lch' arguments")
    }

}

func (cssParser : &mut CSSParser) parseOKLCHColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSOKLCHColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'oklch'")
    }

    data.lightness = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.pChroma = parser.parseNumberOrPercentageOrNone(builder)
    parser.incrementToken(TokenType.Comma)
    data.hue = parser.parseNumberOrAngleOrNone(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentageOrNone(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'oklch' arguments")
    }

}

func (cssParser : &mut CSSParser) parseIdentifierCSSColor(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    token : *mut Token
) : bool {
    const kind = cssParser.getIdentifierColorKind(token.value)
    if(kind == CSSColorKind.Unknown) {
        return false;
    } else if(kind >= CSSColorKind.FunctionsStart && kind <= CSSColorKind.FunctionsEnd) {
        // detected a function
        switch(kind) {
            CSSColorKind.RGB, CSSColorKind.RGBA => {

                parser.increment()

                const rgbData = builder.allocate<CSSRGBColorData>()
                new (rgbData) CSSRGBColorData();

                cssParser.parseRGBColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.rgbData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.HSL, CSSColorKind.HSLA => {

                parser.increment()

                const rgbData = builder.allocate<CSSHSLColorData>()
                new (rgbData) CSSHSLColorData();

                cssParser.parseHSLColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.rgbData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.HWB => {
                parser.increment()

                const rgbData = builder.allocate<CSSHWBColorData>()
                new (rgbData) CSSHWBColorData();

                cssParser.parseHWBColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.hwbData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.LAB => {
                parser.increment()

                const rgbData = builder.allocate<CSSLABColorData>()
                new (rgbData) CSSLABColorData();

                cssParser.parseLABColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.labData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.LCH => {
                parser.increment()

                const rgbData = builder.allocate<CSSLCHColorData>()
                new (rgbData) CSSLCHColorData();

                cssParser.parseLCHColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.lchData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.OKLAB => {
                parser.increment()

                const rgbData = builder.allocate<CSSOKLABColorData>()
                new (rgbData) CSSOKLABColorData();

                cssParser.parseOKLABColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.oklabData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            CSSColorKind.OKLCH => {
                parser.increment()

                const rgbData = builder.allocate<CSSOKLCHColorData>()
                new (rgbData) CSSOKLCHColorData();

                cssParser.parseOKLCHColor(parser, builder, *rgbData)

                var col_value = builder.allocate<CSSColorValueData>();
                col_value.kind = kind;
                col_value.value.oklchData = rgbData

                value.kind = CSSValueKind.Color
                value.data = col_value

                return true;
            }
            default => {
                parser.error("color function kind not handled in css color parser");
                return false;
            }
        }
    } else {
        parser.increment()
        alloc_color_val_data(builder, value, token.value, kind)
        return true;
    }
}

func (cssParser : &mut CSSParser) parseCSSColor(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.HexColor => {
            cssParser.parseHexColor(parser, builder, token.value, value);
            parser.increment();
            return true;
        }
        TokenType.Identifier => {
            return cssParser.parseIdentifierCSSColor(parser, builder, value, token)
        }
        default => {
            return false;
        }
    }
}