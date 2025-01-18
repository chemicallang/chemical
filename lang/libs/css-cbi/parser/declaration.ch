import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"

func parseLengthKind(parser : *mut Parser, builder : *mut ASTBuilder) : CSSValueKind {
    const token = parser.getToken();
    if(token.type == TokenType.Percentage) {
        parser.increment();
        return CSSValueKind.LengthPERCENTAGE
    } else if(token.type == TokenType.Identifier) {
        parser.increment();
        switch(fnv1_hash(token.value.data())) {
            comptime_fnv1_hash("px") => {
                return CSSValueKind.LengthPX
            }
            comptime_fnv1_hash("em") => {
                return CSSValueKind.LengthEM
            }
            comptime_fnv1_hash("rem") => {
                return CSSValueKind.LengthREM
            }
            comptime_fnv1_hash("vh") => {
                return CSSValueKind.LengthVH
            }
            comptime_fnv1_hash("vw") => {
                return CSSValueKind.LengthVW
            }
            comptime_fnv1_hash("vmin") => {
                return CSSValueKind.LengthVMIN
            }
            comptime_fnv1_hash("vmax") => {
                return CSSValueKind.LengthVMAX
            }
            comptime_fnv1_hash("cm") => {
                return CSSValueKind.LengthCM
            }
            comptime_fnv1_hash("mm") => {
                return CSSValueKind.LengthMM
            }
            comptime_fnv1_hash("in") => {
                return CSSValueKind.LengthIN
            }
            comptime_fnv1_hash("pt") => {
                return CSSValueKind.LengthPT
            }
            comptime_fnv1_hash("pc") => {
                return CSSValueKind.LengthPC
            }
            comptime_fnv1_hash("ch") => {
                return CSSValueKind.LengthCH
            }
            comptime_fnv1_hash("ex") => {
                return CSSValueKind.LengthEX
            }
            comptime_fnv1_hash("s") => {
                return CSSValueKind.LengthS
            }
            comptime_fnv1_hash("ms") => {
                return CSSValueKind.LengthMS
            }
            comptime_fnv1_hash("Hz") => {
                return CSSValueKind.LengthHZ
            }
            comptime_fnv1_hash("kHz") => {
                return CSSValueKind.LengthKHZ
            }
            comptime_fnv1_hash("deg") => {
                return CSSValueKind.LengthDEG
            }
            comptime_fnv1_hash("rad") => {
                return CSSValueKind.LengthRAD
            }
            comptime_fnv1_hash("grad") => {
                return CSSValueKind.LengthGRAD
            }
            comptime_fnv1_hash("turn") => {
                return CSSValueKind.LengthTURN
            }
            default => {
                parser.error("unknown unit found");
                return CSSValueKind.LengthPX
            }
        }
    } else {
        parser.error("unknown unit token found");
        parser.increment()
        return CSSValueKind.LengthPX
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