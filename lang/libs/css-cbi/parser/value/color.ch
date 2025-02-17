func (cssParser : &mut CSSParser) parseHexColor(parser : *mut Parser, builder : *mut ASTBuilder, colorView : &std::string_view, value : &mut CSSValue) {
    // After the #, only 3, 4, 6, or 8 hexadecimal digits are allowed
    const hex_size = colorView.size() - 1;
    if(hex_size != 3 && hex_size != 4 && hex_size != 6 && hex_size != 8) {
        parser.error("hex color digits length must be 3,4,6 or 8");
        return;
    }
    var col_value = builder.allocate<CSSColorValueData>();
    new (col_value) CSSColorValueData {
        kind : CSSColorKind.HexColor,
        value : builder.allocate_view(colorView)
    }
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

func getSysColorSmallHash(view : &std::string_view) : size_t {

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

func getSysColorKind(hash : size_t) : CSSColorKind {
    if(isSystemColor(hash)) {
        return CSSColorKind.SystemColor
    } else if(hash == comptime_fnv1_hash("transparent")) {
        return CSSColorKind.Transparent
    } else if(hash == comptime_fnv1_hash("currentcolor")) {
        return CSSColorKind.CurrentColor
    } else {
        return CSSColorKind.Unknown;
    }
}

func (cssParser : &mut CSSParser) getIdentifierColorKind(view : &std::string_view) : CSSColorKind {
    if(cssParser.isNamedColor(view)) {
        return CSSColorKind.NamedColor
    } else {
        return getSysColorKind(getSysColorSmallHash(view))
    }
}

func isStrSystemColor(view : &std::string_view) : bool {
    // all system colors are less than 25 characters in length
    if(view.size() > 25) {
        return false;
    }
    return isSystemColor(getSysColorSmallHash(view))
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
            const kind = cssParser.getIdentifierColorKind(token.value)
            if(kind != CSSColorKind.Unknown) {
                parser.increment()
                alloc_color_val_data(builder, value, token.value, kind)
                return true;
            }
        }
        default => {
            return false;
        }
    }
}