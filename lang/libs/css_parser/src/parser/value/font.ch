
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
            hash = fnv1_hash_view(&token.value)
        }
        const kind = parseFontValueKeywordKind(builder, font, &token.value, hash)
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

func (parser : &mut Parser) parseFontFamiliesList(builder : *mut ASTBuilder, family : &mut CSSFontFamily) {
    // parsing the font family
    while(true) {
        const token = parser.getToken()
        switch(token.type) {
            TokenType.Identifier, TokenType.PropertyName, TokenType.DoubleQuotedValue, TokenType.SingleQuotedValue => {

                if((token.type == TokenType.Identifier || token.type == TokenType.PropertyName) && token.value.equals("var")) {
                    return;
                }

                parser.increment()

                family.families.push(builder.allocate_view(&token.value))

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
        first_val_hash = fnv1_hash_view(&firstTok.value)
        const sysKw = getSystemFamilyNameKeywordKind(first_val_hash)
        if(sysKw != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, value, sysKw, &firstTok.value)
            return;
        }
    }

    parseFontKeywordValues(parser, builder, &mut *font, first_val_hash)

    const token = parser.getToken()
    if(token.type == TokenType.Number) {
        // Check if this is a numeric font weight (100-900) or font size
        // Look ahead: if the next token is a number or identifier that looks like
        // a font-size keyword, then current token is a weight
        var isWeight = false
        if(token.value.size() == 3) {
            const c0 = token.value.data()[0] as uint
            const c1 = token.value.data()[1] as uint
            const c2 = token.value.data()[2] as uint
            if(c0 >= '1' as uint && c0 <= '9' as uint && c1 == '0' as uint && c2 == '0' as uint) {
                // Looks like 100-900, check if what follows is a font size
                const nextTok = token + 1
                if(nextTok.type == TokenType.Number) {
                    // Another number follows, so this is weight: "600 18px"
                    isWeight = true
                } else if(nextTok.type == TokenType.Identifier) {
                    // Check if it's NOT a unit (px, em, rem, etc.)
                    if(!nextTok.value.equals("px") && !nextTok.value.equals("em") &&
                       !nextTok.value.equals("rem") && !nextTok.value.equals("pt") &&
                       !nextTok.value.equals("%") && !nextTok.value.equals("/")) {
                        isWeight = true
                    }
                }
            }
        }
        if(isWeight) {
            font.weight = CSSFontWeight.Absolute(builder.allocate_view(&token.value))
            parser.increment()
            // Now parse the font size that follows the weight
            const sizeToken = parser.getToken()
            if(sizeToken.type == TokenType.Number) {
                if(!cssParser.parseLength(parser, builder, &mut font.size)) {
                    parser.error("couldn't parse font size after weight");
                }
            } else if(sizeToken.type == TokenType.Identifier) {
                const fontSizeKind = getFontSizeKeywordKind(sizeToken.fnv1())
                if(fontSizeKind != CSSKeywordKind.Unknown) {
                    parser.increment()
                    alloc_value_keyword(builder, &mut font.size, fontSizeKind, &sizeToken.value)
                }
            }
        } else {
            // Font Size
            if(!cssParser.parseLength(parser, builder, &mut font.size)) {
                parser.error("couldn't parse length");
            }
        }
        const next = parser.getToken();
        if(next.type == TokenType.Divide) {
            parser.increment()
            // Line Height
            if(!cssParser.parseNumberOrLength(parser, builder, &mut font.lineHeight)) {
                parser.error("expected line height value after the length");
            }
        }
    } else if(token.type == TokenType.Identifier) {
        const fontSizeKind = getFontSizeKeywordKind(token.fnv1())
        if(fontSizeKind != CSSKeywordKind.Unknown) {
            parser.increment()
            alloc_value_keyword(builder, &mut font.size, fontSizeKind, &token.value)
        } else {
            parser.error("unknown keyword for font size");
        }
    } else {
        parser.error("expected a font size after the font keyword values");
    }

    parser.parseFontFamiliesList(builder, &mut font.family)

}

func (cssParser : &mut CSSParser) parseFontFamily(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    const token = parser.getToken()
    if(token.type == TokenType.LBrace || token.type == TokenType.DollarLBrace) {
        cssParser.parseChemValueAfterLBrace(parser, builder, value)
        return;
    }
    if((token.type == TokenType.Identifier || token.type == TokenType.PropertyName) && token.value.equals("var")) {
        parser.increment()
        const fontVar = cssParser.parseCSSVariableFunc(parser, builder)
        alloc_value_length_var(parser, builder, value, &fontVar)
        return;
    }

    var family = builder.allocate<CSSFontFamily>();
    new (family) CSSFontFamily()

    value.kind = CSSValueKind.FontFamily
    value.data = family

    parser.parseFontFamiliesList(builder, &mut *family)

}