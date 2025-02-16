func getHashedFontStyleKeyword(hashed : uint32_t) : CSSKeywordKind {
    switch(hashed) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("italic") => { return CSSKeywordKind.Italic }
        comptime_fnv1_hash("oblique") => { return CSSKeywordKind.Oblique }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHashedFontVariantKeyword(hashed : uint32_t) : CSSKeywordKind {
    switch(hashed) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("small-caps") => { return CSSKeywordKind.SmallCaps }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHashedFontWeightKeyword(hashed : uint32_t) : CSSKeywordKind {
    switch(hashed) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("bold") => { return CSSKeywordKind.Bold }
        comptime_fnv1_hash("bolder") => { return CSSKeywordKind.Bolder }
        comptime_fnv1_hash("lighter") => { return CSSKeywordKind.Lighter }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHashedFontWidthKeyword(hashed : uint32_t) : CSSKeywordKind {
    switch(hashed) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("ultra-condensed") => { return CSSKeywordKind.UltraCondensed }
        comptime_fnv1_hash("extra-condensed") => { return CSSKeywordKind.ExtraCondensed }
        comptime_fnv1_hash("condensed") => { return CSSKeywordKind.Condensed }
        comptime_fnv1_hash("semi-condensed") => { return CSSKeywordKind.SemiCondensed }
        comptime_fnv1_hash("semi-expanded") => { return CSSKeywordKind.SemiExpanded }
        comptime_fnv1_hash("expanded") => { return CSSKeywordKind.Expanded }
        comptime_fnv1_hash("extra-expanded") => { return CSSKeywordKind.ExtraExpanded }
        comptime_fnv1_hash("ultra-expanded") => { return CSSKeywordKind.UltraExpanded }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHashedSystemFamilyNameKeyword(hashed : uint32_t) : CSSKeywordKind {
    switch(hashed) {
        comptime_fnv1_hash("caption") => { return CSSKeywordKind.Caption }
        comptime_fnv1_hash("icon") => { return CSSKeywordKind.Icon }
        comptime_fnv1_hash("menu") => { return CSSKeywordKind.Menu }
        comptime_fnv1_hash("message-box") => { return CSSKeywordKind.MessageBox }
        comptime_fnv1_hash("small-caption") => { return CSSKeywordKind.SmallCaption }
        comptime_fnv1_hash("status-bar") => { return CSSKeywordKind.StatusBar }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHashedFontStretchKeyword(hashed : uint32_t) : CSSKeywordKind {
    return getHashedFontWidthKeyword(hashed)
}

func parseFontValueKeywordKind(builder : *mut ASTBuilder, font : &mut CSSFontValueData, value : &std::string_view, hash : uint32_t) : CSSKeywordKind {
    const fontStyleKw = getHashedFontStyleKeyword(hash)
    if(fontStyleKw != CSSKeywordKind.Unknown) {
        font.style = CSSFontStyle.Keyword(CSSKeywordValueData { kind : fontStyleKw, value : builder.allocate_view(value) })
        return fontStyleKw;
    } else {
        const fontVariantKw = getHashedFontVariantKeyword(hash)
        if(fontVariantKw != CSSKeywordKind.Unknown) {
            font.fontVariant = CSSKeywordValueData { kind : fontVariantKw, value : builder.allocate_view(value) }
            return fontVariantKw;
        } else {
            const fontWeightKw = getHashedFontWeightKeyword(hash)
            if(fontWeightKw != CSSKeywordKind.Unknown) {
                font.weight = CSSFontWeight.Keyword(CSSKeywordValueData { kind : fontWeightKw, value : builder.allocate_view(value) })
                return fontWeightKw;
            } else {
                const stretchKw = getHashedFontStretchKeyword(hash)
                if(stretchKw != CSSKeywordKind.Unknown) {
                    font.stretch = CSSKeywordValueData { kind : stretchKw, value : builder.allocate_view(value) }
                    return stretchKw;
                } else {
                    // TODO pass value as hash
                    // const sizeKw = getFontSizeKeywordKind(value.data())
                    // if(sizeKw != CSSKeywordKind.Unknown) {
                    //     font.size.kind = CSSValueKind.Keyword
                    //     const kw = builder.allocate<CSSKeywordValueData>()
                    //     new (kw) CSSKeywordValueData { kind : sizeKw, value : builder.allocate_view(value) }
                    //     font.size.data = kw
                    //     return sizeKw
                    // } else {
                        return CSSKeywordKind.Unknown;
                    // }
                }
            }
        }
    }
}

func (parser : &mut Parser) incrementToken(type : TokenType) : bool {
    const token = parser.getToken()
    if(token.type == type) {
        parser.increment()
        return true;
    } else {
        return false;
    }
}

func parseFontKeywordValues(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    font : &mut CSSFontValueData,
    first_val_hash : uint32_t
) {
    var i = -1;
    while(true) {
        i++;
        const token = parser.getToken()
        if(token.type != TokenType.Identifier) {
            return;
        }
        var hash : uint32_t = 0
        if(i == 0) {
            hash = first_val_hash
        } else {
            hash = fnv1_hash(token.value.data())
        }
        const kind = parseFontValueKeywordKind(builder, font, token.value, hash)
        if(kind == CSSKeywordKind.Oblique) {
            // there's an angle after this
            parser.increment()
            const angleTok = parser.getToken()
            if(angleTok.type == TokenType.Number) {
                parser.increment()
                font.style = CSSFontStyle.Oblique(angleTok.value)
                const degTok = parser.getToken()
                if(degTok.type == TokenType.Identifier && degTok.value.equals("deg")) {
                    parser.increment()
                } else {
                    parser.error("expected 'deg' for angle unit");
                }
            } else {
                parser.error("expected a number for oblique angle");
            }
        } else if(kind == CSSKeywordKind.Unknown) {
            return;
        } else {
            parser.increment()
        }
    }
}

func (parser : &mut Parser) parseFontFamiliesList(builder : *mut ASTBuilder, font : &mut CSSFontValueData) {
    // parsing the font family
    while(true) {
        const token = parser.getToken()
        switch(token.type) {
            TokenType.Identifier, TokenType.DoubleQuotedValue => {

                parser.increment()

                font.family.families.push(builder.allocate_view(token.value))

                // optionally increment the comma
                parser.incrementToken(TokenType.Comma)

            }
            default => {
                return;
            }
        }
    }
}

func (cssParser : &mut CSSParser) parseFont(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    var font = builder.allocate<CSSFontValueData>();
    new (font) CSSFontValueData {
         style       : CSSFontStyle.None()
         fontVariant : CSSKeywordValueData()
         weight      : CSSFontWeight.None()
         stretch     : CSSKeywordValueData()
         size        : CSSValue()
         lineHeight  : CSSValue()
         family      : CSSFontFamily()
    }

    value.kind = CSSValueKind.Font
    value.data = font

    var first_val_hash = 0;
    const firstTok = parser.getToken()
    if(firstTok.type == TokenType.Identifier) {
        first_val_hash = fnv1_hash(firstTok.value.data())
        const sysKw = getHashedSystemFamilyNameKeyword(first_val_hash)
        if(sysKw != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, value, sysKw, firstTok.value)
            return;
        }
    }

    parseFontKeywordValues(parser, builder, *font, first_val_hash)

    const token = parser.getToken()
    if(token.type == TokenType.Number) {
        // Font Size
        if(!cssParser.parseLength(parser, builder, font.size)) {
            parser.error("couldn't parse length");
        }
        const next = parser.getToken();
        switch(next.type) {
            TokenType.Divide => {
                parser.increment()
                // Line Height
                if(!cssParser.parseNumberOrLength(parser, builder, font.lineHeight)) {
                    parser.error("expected line height value after the length");
                }
            }
        }
    } else if(token.type == TokenType.Identifier) {
        // TODO do not pass the the data pointer, pass the hash
        const fontSizeKind = getFontSizeKeywordKind(token.value.data())
        if(fontSizeKind != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, font.size, fontSizeKind, token.value)
        } else {
            parser.error("unknown keyword for font size");
        }
    } else {
        parser.error("expected a font size after the font keyword values");
    }

    parser.parseFontFamiliesList(builder, *font)

}