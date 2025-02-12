import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"
import "./value/length.ch"
import "./CSSParser.ch"

func getCSSGlobalValueKind(ptr : *char) : CSSValueKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("inherit") => {
            return CSSValueKind.Inherit;
        }
        comptime_fnv1_hash("initial") => {
            return CSSValueKind.Initial
        }
        comptime_fnv1_hash("unset") => {
            return CSSValueKind.Unset
        }
        default => {
            return CSSValueKind.Unknown
        }
    }
}

func (cssParser : &mut CSSParser) parseValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) {
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
            const global = getCSSGlobalValueKind(token.value.data());
            if(global != CSSValueKind.Unknown) {
                parser.increment();
                value.kind = global;
                return;
            } else if(cssParser.isColor(token.value)) {
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
        default => {
            parser.error("unknown value token with data");
        }
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
            kind : CSSValueKind.Length,
            data : null
        }
    }

    const col = parser.getToken();
    if(col.type == TokenType.Colon) {
        parser.increment();
    } else {
        parser.error("expected colon after the css property name");
    }

    cssParser.parseValue(parser, builder, decl.value);

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