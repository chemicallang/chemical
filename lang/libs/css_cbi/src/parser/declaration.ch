
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
    var first = true;

    while(true) {
        const token = parser.getToken();
        if(token.type == TokenType.Semicolon || token.type == TokenType.EndOfFile) {
            break;
        }
        if(token.type == TokenType.Important) {
            break;
        }

        if(!first && token_needs_space_before(token.type as TokenType) && token_needs_space_after(prev_type as TokenType)) {
            raw.append(' ')
        }

        raw.append_view(token.value)
        prev_type = token.type as TokenType
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

func (cssParser : &mut CSSParser) parseRandomValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) : bool {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, token.value, false);
            return true;
        }
        TokenType.Identifier => {
            cssParser.parseIdentifierCSSColor(parser, builder, value, token)
            return true;
        }
        TokenType.HexColor => {
            cssParser.parseHexColor(parser, builder, token.value, value);
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
                value : builder.allocate_view(val)
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
        const globalKind = getCSSGlobalKeywordKind(valueTok.value);
        if(globalKind != CSSKeywordKind.Unknown) {
            parser.increment();
            var kw_value = builder.allocate<CSSKeywordValueData>();
            new (kw_value) CSSKeywordValueData {
                kind : globalKind,
                value : builder.allocate_view(valueTok.value)
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
            name : builder.allocate_view(token.value)
        },
        value : CSSValue {
            kind : CSSValueKind.Unknown,
            data : null
        },
        important : false
    }

    if(is_custom_property_name(token.value)) {
        cssParser.parseCustomPropertyValue(parser, builder, decl.value);
    } else {
        cssParser.parseValue(parser, builder, decl.value, token.value);
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
