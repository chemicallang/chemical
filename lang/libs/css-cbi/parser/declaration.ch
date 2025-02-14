import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"
import "./value/length.ch"
import "./CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"

func getCSSGlobalKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
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

func (cssParser : &mut CSSParser) parseRandomValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            var number_value = builder.allocate<CSSLengthValueData>()
            new (number_value) CSSLengthValueData {
                kind : CSSLengthKind.Unknown,
                value : builder.allocate_view(token.value)
            }
            value.kind = CSSValueKind.Length
            number_value.kind = parseLengthKind(parser, builder);
            value.data = number_value
            return
        }
        TokenType.Identifier => {
            if(cssParser.isColor(token.value)) {
                parser.increment();
                var col_value = builder.allocate<CSSColorValueData>();
                new (col_value) CSSColorValueData {
                    kind : CSSColorKind.NamedColor,
                    value : builder.allocate_view(token.value)
                }
                value.kind = CSSValueKind.Color
                value.data = col_value
                return;
            } else {
                parser.error("unknown value given");
            }
        }
        TokenType.HexColor => {
            // After the #, only 3, 4, 6, or 8 hexadecimal digits are allowed
            const hex_size = token.value.size() - 1;
            if(hex_size != 3 && hex_size != 4 && hex_size != 6 && hex_size != 8) {
                parser.error("hex color digits length must be 3,4,6 or 8");
                return;
            }
            var col_value = builder.allocate<CSSColorValueData>();
            new (col_value) CSSColorValueData {
                kind : CSSColorKind.HexColor,
                value : builder.allocate_view(token.value)
            }
            value.kind = CSSValueKind.Color
            value.data = col_value
            var out : uint32_t = 0
            if(!parse_css_hex_color(token.value.data() + 1, token.value.size() - 1, &out)) {
                parser.error("hash color is not valid");
            }
            parser.increment();
            return;
        }
        default => {
            parser.error("unknown value token with data");
        }
    }
}

func (cssParser : &mut CSSParser) parseValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    propertyName : &std::string_view
) {

    const valueTok = parser.getToken();
    if(valueTok.type == TokenType.Identifier) {
        const globalKind = getCSSGlobalKeywordKind(valueTok.value.data());
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

func (cssParser : &mut CSSParser) parseDeclaration(parser : *mut Parser, builder : *mut ASTBuilder) : *mut CSSDeclaration {

    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        parser.increment();
    } else {
        return null;
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
        }
    }

    const col = parser.getToken();
    if(col.type == TokenType.Colon) {
        parser.increment();
    } else {
        parser.error("expected colon after the css property name");
    }

    cssParser.parseValue(parser, builder, decl.value, token.value);

    const sc = parser.getToken();
    if(sc.type == TokenType.Semicolon) {
        parser.increment();
    } else {
        parser.error("expected a semicolon after the property's value");
    }

    printf("parsed a declaration with key '%s'\n", token.value.data());
    fflush(null)

    return decl;

}