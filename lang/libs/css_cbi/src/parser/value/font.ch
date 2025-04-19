
func parseFontValueKeywordKind(builder : *mut ASTBuilder, font : &mut CSSFontValueData, value : &std::string_view, hash : size_t) : CSSKeywordKind {
    const fontStyleKw = getFontStyleKeywordKind(hash)
    if(fontStyleKw != CSSKeywordKind.Unknown) {
        font.style = CSSFontStyle.Keyword(CSSKeywordValueData { kind : fontStyleKw, value : builder.allocate_view(value) })
        return fontStyleKw;
    } else {
        const fontVariantKw = getFontVariantKeywordKind(hash)
        if(fontVariantKw != CSSKeywordKind.Unknown) {
            font.fontVariant = CSSKeywordValueData { kind : fontVariantKw, value : builder.allocate_view(value) }
            return fontVariantKw;
        } else {
            const fontWeightKw = getFontWeightKeywordKind(hash)
            if(fontWeightKw != CSSKeywordKind.Unknown) {
                font.weight = CSSFontWeight.Keyword(CSSKeywordValueData { kind : fontWeightKw, value : builder.allocate_view(value) })
                return fontWeightKw;
            } else {
                const stretchKw = getFontStretchKeywordKind(hash)
                if(stretchKw != CSSKeywordKind.Unknown) {
                    font.stretch = CSSKeywordValueData { kind : stretchKw, value : builder.allocate_view(value) }
                    return stretchKw;
                } else {
                    return CSSKeywordKind.Unknown;
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
    first_val_hash : size_t
) {
    var i = -1;
    while(true) {
        i++;
        const token = parser.getToken()
        if(token.type != TokenType.Identifier) {
            return;
        }
        var hash : size_t = 0
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

    var first_val_hash : size_t = 0;
    const firstTok = parser.getToken()
    if(firstTok.type == TokenType.Identifier) {
        first_val_hash = fnv1_hash(firstTok.value.data())
        const sysKw = getSystemFamilyNameKeywordKind(first_val_hash)
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
        if(next.type == TokenType.Divide) {
            parser.increment()
            // Line Height
            if(!cssParser.parseNumberOrLength(parser, builder, font.lineHeight)) {
                parser.error("expected line height value after the length");
            }
        }
    } else if(token.type == TokenType.Identifier) {
        const fontSizeKind = getFontSizeKeywordKind(token.fnv1())
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