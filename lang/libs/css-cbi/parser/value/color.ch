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

func (parser : &mut Parser) parseNumberOrPercentage(builder : *mut ASTBuilder) : CSSNumberOrPercentage {
    const token = parser.getToken()
    if(token.type == TokenType.Number) {
        parser.increment();
        const next = parser.getToken()
        const is_percentage = next.type == TokenType.Percentage;
        if(is_percentage) {
            parser.increment();
        }
        return CSSNumberOrPercentage { number : builder.allocate_view(token.value), is_percentage : is_percentage }
    } else {
        return CSSNumberOrPercentage()
    }
}

// this should be called after incrementing the 'rgb' token
func (cssParser : &mut CSSParser) parseRGBColor(parser : *mut Parser, builder : *mut ASTBuilder, data : &mut CSSRGBColorData) {
    const next = parser.getToken()
    if(next.type == TokenType.LParen) {
        parser.increment()
    } else {
        parser.error("expected a '(' after 'rgb'")
    }

    data.red = parser.parseNumberOrPercentage(builder)
    parser.incrementToken(TokenType.Comma)
    data.green = parser.parseNumberOrPercentage(builder)
    parser.incrementToken(TokenType.Comma)
    data.blue = parser.parseNumberOrPercentage(builder)
    const sep = parser.getToken()
    if(sep.type == TokenType.Divide || sep.type == TokenType.Comma) {
        parser.increment()
    }
    data.alpha = parser.parseNumberOrPercentage(builder)

    const last = parser.getToken()
    if(last.type == TokenType.RParen) {
        parser.increment()
    } else {
        parser.error("expected a ')' after 'rgb' arguments")
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