
func getCSSGlobalKeywordKind(view : &std::string_view) : CSSKeywordKind {
    switch(fnv1_hash_view(view)) {
        comptime_fnv1_hash("inherit") => {
            return CSSKeywordKind.Inherit;
        }
        comptime_fnv1_hash("initial") => {
            return CSSKeywordKind.Initial
        }
        comptime_fnv1_hash("unset") => {
            return CSSKeywordKind.Unset
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func is_custom_property_name(view : &std::string_view) : bool {
    return view.size() >= 2 && view.get(0) == '-' && view.get(1) == '-';
}

func token_needs_space_before(type : TokenType) : bool {
    switch(type) {
        TokenType.Identifier,
        TokenType.PropertyName,
        TokenType.Number,
        TokenType.HexColor,
        TokenType.SingleQuotedValue,
        TokenType.DoubleQuotedValue,
        TokenType.Important,
        TokenType.Id,
        TokenType.ClassName => {
            return true;
        }
        default => {
            return false;
        }
    }
}

func token_needs_space_after(type : TokenType) : bool {
    switch(type) {
        TokenType.Identifier,
        TokenType.PropertyName,
        TokenType.Number,
        TokenType.HexColor,
        TokenType.SingleQuotedValue,
        TokenType.DoubleQuotedValue,
        TokenType.Important,
        TokenType.Id,
        TokenType.ClassName,
        TokenType.Comma,
        TokenType.RParen => {
            return true;
        }
        default => {
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseCustomPropertyValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var raw = std::string()
    var prev_type = TokenType.Unexpected;
    var prev_val = std::string_view("");
    var first = true;

    while(true) {
        const token = parser.getToken();
        if(token.type == TokenType.Semicolon || token.type == TokenType.EndOfFile) {
            break;
        }
        if(token.type == TokenType.Important) {
            break;
        }

        if(!first) {
            const prev_end = prev_val.data() + prev_val.size();
            if(prev_end != token.value.data()) {
                raw.append(' ')
            }
        }

        raw.append_view(&token.value)
        prev_type = token.type as TokenType
        prev_val = token.value
        first = false
        parser.increment()
    }

    var raw_data = builder.allocate<CSSRawValueData>();
    new (raw_data) CSSRawValueData {
        value : builder.allocate_view(raw.to_view())
    }
    value.kind = CSSValueKind.Raw
    value.data = raw_data
}

func (cssParser : &mut CSSParser) parseRawPropertyValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var raw = std::string()
    var prev_type = TokenType.Unexpected;
    var prev_val = std::string_view("");
    var first = true;

    while(true) {
        const token = parser.getToken();
        if(token.type == TokenType.Semicolon || token.type == TokenType.EndOfFile) {
            break;
        }
        if(token.type == TokenType.Important) {
            break;
        }

        if(!first) {
            const prev_end = prev_val.data() + prev_val.size();
            if(prev_end != token.value.data()) {
                raw.append(' ')
            }
        }

        raw.append_view(&token.value)
        prev_type = token.type as TokenType
        prev_val = token.value
        first = false
        parser.increment()
    }

    var raw_data = builder.allocate<CSSRawValueData>();
    new (raw_data) CSSRawValueData {
        value : builder.allocate_view(raw.to_view())
    }
    value.kind = CSSValueKind.Raw
    value.data = raw_data
}

func (cssParser : &mut CSSParser) parseRawFunctionValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    var raw = std::string()
    var prev_type = TokenType.Unexpected;
    var prev_val = std::string_view("");
    var first = true;
    var depth = 0;
    var seen_lparen = false;

    while(true) {
        const token = parser.getToken();
        if(token.type == TokenType.EndOfFile) {
            parser.error("expected a ')' to end the function call");
            break;
        }

        if(!first) {
            const prev_end = prev_val.data() + prev_val.size();
            if(prev_end != token.value.data()) {
                raw.append(' ')
            }
        }

        raw.append_view(&token.value)

        if(token.type == TokenType.LParen) {
            depth++;
            seen_lparen = true;
        } else if(token.type == TokenType.RParen) {
            depth--;
        }

        prev_type = token.type as TokenType
        prev_val = token.value
        first = false
        parser.increment()

        if(seen_lparen && depth == 0) {
            break;
        }
    }

    var raw_data = builder.allocate<CSSRawValueData>();
    new (raw_data) CSSRawValueData {
        value : builder.allocate_view(raw.to_view())
    }
    value.kind = CSSValueKind.Raw
    value.data = raw_data
}

func (cssParser : &mut CSSParser) parseRandomIdentifierValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    token : *mut Token
) : bool {
    const next = token + 1;
    const hash = token.fnv1();

    if(next.type == TokenType.LParen) {
        switch(hash) {
            comptime_fnv1_hash("calc"),
            comptime_fnv1_hash("calc-size") => {
                parser.increment();
                cssParser.parseCalc(parser, builder, value);
                return true;
            }
            comptime_fnv1_hash("var"),
            comptime_fnv1_hash("rgb"),
            comptime_fnv1_hash("rgba"),
            comptime_fnv1_hash("hsl"),
            comptime_fnv1_hash("hsla"),
            comptime_fnv1_hash("hwb"),
            comptime_fnv1_hash("lab"),
            comptime_fnv1_hash("lch"),
            comptime_fnv1_hash("oklab"),
            comptime_fnv1_hash("oklch"),
            comptime_fnv1_hash("color") => {
                return cssParser.parseIdentifierCSSColor(parser, builder, value, token);
            }
            comptime_fnv1_hash("fit-content") => {
                parser.increment();
                cssParser.parseFitContentCall(parser, builder, value);
                return true;
            }
            comptime_fnv1_hash("url"),
            comptime_fnv1_hash("src"),
            comptime_fnv1_hash("linear-gradient"),
            comptime_fnv1_hash("repeating-linear-gradient"),
            comptime_fnv1_hash("radial-gradient"),
            comptime_fnv1_hash("repeating-radial-gradient"),
            comptime_fnv1_hash("conic-gradient"),
            comptime_fnv1_hash("repeating-conic-gradient") => {
                parser.increment();
                const image = builder.allocate<BackgroundImageData>();
                new (image) BackgroundImageData {
                    url = UrlData()
                    gradient = GradientData()
                };
                image.is_url = true;
                if(hash == comptime_fnv1_hash("src")) {
                    image.url.is_source = true;
                    cssParser.parseUrlValue(parser, builder, &mut image.url)
                } else if(hash == comptime_fnv1_hash("url")) {
                    cssParser.parseUrlValue(parser, builder, &mut image.url)
                } else {
                    image.is_url = false;
                    switch(hash) {
                        comptime_fnv1_hash("linear-gradient") => {
                            cssParser.parseLinearGradient(parser, builder, &mut image.gradient, false)
                        }
                        comptime_fnv1_hash("repeating-linear-gradient") => {
                            cssParser.parseLinearGradient(parser, builder, &mut image.gradient, true)
                        }
                        comptime_fnv1_hash("radial-gradient") => {
                            cssParser.parseRadialGradient(parser, builder, &mut image.gradient, false)
                        }
                        comptime_fnv1_hash("repeating-radial-gradient") => {
                            cssParser.parseRadialGradient(parser, builder, &mut image.gradient, true)
                        }
                        comptime_fnv1_hash("conic-gradient") => {
                            cssParser.parseConicGradient(parser, builder, &mut image.gradient, false)
                        }
                        default => {
                            cssParser.parseConicGradient(parser, builder, &mut image.gradient, true)
                        }
                    }
                }
                value.kind = CSSValueKind.BackgroundImage
                value.data = image
                return true;
            }
            default => {
                cssParser.parseRawFunctionValue(parser, builder, value);
                return true;
            }
        }
    }

    if(cssParser.parseIdentifierCSSColor(parser, builder, value, token)) {
        return true;
    }

    parser.increment();
    alloc_value_keyword(builder, value, CSSKeywordKind.Unknown, &token.value);
    return true;
}

func (cssParser : &mut CSSParser) parseRandomValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, &token.value, false);
            return true;
        }
        TokenType.Identifier, TokenType.PropertyName => {
            return cssParser.parseRandomIdentifierValue(parser, builder, value, token);
        }
        TokenType.HexColor => {
            cssParser.parseHexColor(parser, builder, &token.value, value);
            parser.increment();
            return true;
        }
        TokenType.LBrace, TokenType.DollarLBrace => {
            cssParser.parseChemValueAfterLBrace(parser, builder, value)
            return true;
        }
        TokenType.SingleQuotedValue, TokenType.DoubleQuotedValue => {
            parser.increment();
            const val = std::string_view(token.value.data() + 1, token.value.size() - 2);
            var str_data = builder.allocate<CSSStringValueData>();
            new (str_data) CSSStringValueData {
                value : builder.allocate_view(&val)
            }
            value.kind = CSSValueKind.String;
            value.data = str_data;
            return true;
        }
        default => {
            parser.error("unknown value token with data");
            return false;
        }
    }
}

func (cssParser : &mut CSSParser) parseChemValueAfterLBrace(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
) {
    parser.increment();
    const chem_value = parser.parseExpression(builder)
    if(chem_value != null) {
        value.kind = CSSValueKind.ChemicalValue
        value.data = chem_value
        cssParser.dyn_values.push(chem_value)
    } else {
        parser.error("no expression found in braces");
    }
    const next = parser.getToken()
    if(next.type == ChemicalTokenType.RBrace) {
        parser.increment();
    } else {
        parser.error("expected a '}' after the chemical expression");
    }
}

func (cssParser : &mut CSSParser) parseValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    propertyName : &std::string_view
) {

    const valueTok = parser.getToken();
    if(valueTok.type == TokenType.DollarLBrace) {
        cssParser.parseChemValueAfterLBrace(parser, builder, value);
        return;
    }
    if(valueTok.type == TokenType.Identifier) {
        const globalKind = getCSSGlobalKeywordKind(&valueTok.value);
        if(globalKind != CSSKeywordKind.Unknown) {
            parser.increment();
            var kw_value = builder.allocate<CSSKeywordValueData>();
            new (kw_value) CSSKeywordValueData {
                kind : globalKind,
                value : builder.allocate_view(&valueTok.value)
            }
            value.kind = CSSValueKind.Keyword;
            value.data = kw_value;
            return;
        }
    }

    const parserFn = cssParser.getParserFor(propertyName);

    if(parserFn == null) {

        cssParser.parseRandomValue(parser, builder, value);

    } else {

        parserFn(cssParser, parser, builder, value);

    }

}

func (cssParser : &mut CSSParser) parseContent(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    cssParser.parseRandomValue(parser, builder, value);
}

func (cssParser : &mut CSSParser) parseDeclaration(parser : *mut Parser, builder : *mut ASTBuilder) : *mut CSSDeclaration {

    const token = parser.getToken();

    switch(token.type) {
        TokenType.PropertyName, TokenType.Identifier => {
            // peak the next token and return early if not declaration (without incrementing)
            // if there's no colon next to this token (it maybe a tag name)
            // like div {} instead of color : red;
            const nextTok = token + 1;
            if(nextTok.type != TokenType.Colon) {
                return null;
            }
            // Check if this is a pseudo-class selector: tag:hover{...} or tag::before{...}
            // Pattern: identifier : (identifier | :) { 
            const afterColon = token + 2;
            if(afterColon.type == TokenType.Identifier || afterColon.type == TokenType.PropertyName || afterColon.type == TokenType.Colon) {
                // tag:hover{...} -> check if identifier follows and then {
                // tag::before{...} -> check if ::identifier then {
                if(afterColon.type == TokenType.Colon) {
                    // ::pseudo-element pattern: check for identifier and then {
                    const pseudoIdent = token + 3;
                    if(pseudoIdent.type == TokenType.Identifier || pseudoIdent.type == TokenType.PropertyName) {
                        const afterPseudo = token + 4;
                        if(afterPseudo.type == TokenType.LBrace) {
                            return null; // tag::pseudo{} - selector
                        }
                    }
                } else {
                    // :pseudo-class pattern: check if the token after identifier is {
                    const afterIdent = token + 3;
                    if(afterIdent.type == TokenType.LBrace || afterIdent.type == TokenType.Colon) {
                        return null; // tag:hover{} or tag:hover::before{} - selector
                    }
                }
            }
            // increment both tokens
            parser.increment();
            parser.increment();
        }
        TokenType.Comment => {
            parser.increment();
            return cssParser.parseDeclaration(parser, builder);
        }
        default => {
            return null;
        }
    }

    const decl = builder.allocate<CSSDeclaration>();
    new (decl) CSSDeclaration {
        property : CSSProperty {
            kind : CSSPropertyKind.Unknown,
            name : builder.allocate_view(&token.value)
        },
        value : CSSValue {
            kind : CSSValueKind.Unknown,
            data : null
        },
        important : false
    }

    if(is_custom_property_name(&token.value)) {
        cssParser.parseCustomPropertyValue(parser, builder, &mut decl.value);
    } else {
        cssParser.parseValue(parser, builder, &mut decl.value, &token.value);
    }
    
    if(parser.increment_if(TokenType.Important as int)) {
        decl.important = true;
    } else {
        decl.important = false;
    }

    const sc = parser.getToken();
    if(sc.type == TokenType.Semicolon) {
        parser.increment();
    } else {
        parser.error("expected a semicolon after the property's value");
    }

    return decl;

}
