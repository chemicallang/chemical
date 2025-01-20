import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"
import "./value/length.ch"

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

func parseValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            var number_value = builder.allocate<CSSNumberValueData>()
            new (number_value) CSSNumberValueData {
                CSSValueData : CSSValueData { },
                value : builder.allocate_view(token.value)
            }
            parser.increment();
            value.kind = parseLengthKind(parser, builder);
            value.data = number_value
            return
        }
        TokenType.Identifier => {
            const global = getCSSGlobalValueKind(token.value.data());
            if(global != CSSValueKind.Unknown) {
                parser.increment();
                value.kind = global;
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

func parseDeclaration(parser : *mut Parser, builder : *mut ASTBuilder, has_dynamic_values : &mut bool) : *mut CSSDeclaration {

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
            kind : CSSValueKind.LengthPX,
            data : null
        }
    }

    const col = parser.getToken();
    if(col.type == TokenType.Colon) {
        parser.increment();
    } else {
        parser.error("expected colon after the css property name");
    }

    parseValue(parser, builder, decl.value);

    const sc = parser.getToken();
    if(sc.type == TokenType.Semicolon) {
        parser.increment();
    } else {
        parser.error("expected a semicolon after the property's value");
    }

    return decl;

}