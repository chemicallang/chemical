import "@std/hashing/fnv1.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "/ast/CSSLengthKind.ch"

func getLengthKind(str : *char) : CSSLengthKind {
    switch(fnv1_hash(str)) {
        comptime_fnv1_hash("px") => {
            return CSSLengthKind.LengthPX
        }
        comptime_fnv1_hash("em") => {
            return CSSLengthKind.LengthEM
        }
        comptime_fnv1_hash("rem") => {
            return CSSLengthKind.LengthREM
        }
        comptime_fnv1_hash("vh") => {
            return CSSLengthKind.LengthVH
        }
        comptime_fnv1_hash("vw") => {
            return CSSLengthKind.LengthVW
        }
        comptime_fnv1_hash("vmin") => {
            return CSSLengthKind.LengthVMIN
        }
        comptime_fnv1_hash("vmax") => {
            return CSSLengthKind.LengthVMAX
        }
        comptime_fnv1_hash("cm") => {
            return CSSLengthKind.LengthCM
        }
        comptime_fnv1_hash("mm") => {
            return CSSLengthKind.LengthMM
        }
        comptime_fnv1_hash("in") => {
            return CSSLengthKind.LengthIN
        }
        comptime_fnv1_hash("pt") => {
            return CSSLengthKind.LengthPT
        }
        comptime_fnv1_hash("pc") => {
            return CSSLengthKind.LengthPC
        }
        comptime_fnv1_hash("ch") => {
            return CSSLengthKind.LengthCH
        }
        comptime_fnv1_hash("ex") => {
            return CSSLengthKind.LengthEX
        }
        comptime_fnv1_hash("s") => {
            return CSSLengthKind.LengthS
        }
        comptime_fnv1_hash("ms") => {
            return CSSLengthKind.LengthMS
        }
        comptime_fnv1_hash("Hz") => {
            return CSSLengthKind.LengthHZ
        }
        comptime_fnv1_hash("kHz") => {
            return CSSLengthKind.LengthKHZ
        }
        comptime_fnv1_hash("deg") => {
            return CSSLengthKind.LengthDEG
        }
        comptime_fnv1_hash("rad") => {
            return CSSLengthKind.LengthRAD
        }
        comptime_fnv1_hash("grad") => {
            return CSSLengthKind.LengthGRAD
        }
        comptime_fnv1_hash("turn") => {
            return CSSLengthKind.LengthTURN
        }
        default => {
            return CSSLengthKind.Unknown
        }
    }
}

func parseLengthKind(parser : *mut Parser, builder : *mut ASTBuilder) : CSSLengthKind {
    const token = parser.getToken();
    if(token.type == TokenType.Percentage) {
        parser.increment();
        return CSSLengthKind.LengthPERCENTAGE
    } else if(token.type == TokenType.Identifier) {
        parser.increment();
        const kind = getLengthKind(token.value.data())
        if(kind != CSSLengthKind.Unknown) {
            return kind;
        } else {
            parser.error("unknown length unit");
            return CSSLengthKind.LengthPX
        }
    } else {
        parser.error("unknown unit token found");
        parser.increment()
        return CSSLengthKind.LengthPX
    }
}

func (cssParser : &mut CSSParser) parseLengthOrAuto(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {

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
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("auto")) {
                parser.increment();
                var kw_value = builder.allocate<CSSKeywordValueData>()
                new (kw_value) CSSKeywordValueData {
                    kind : CSSKeywordKind.Auto,
                    value : builder.allocate_view(token.value)
                }
                value.kind = CSSValueKind.Keyword
                value.data = kw_value
                return true;
            } else {
                return false;
            }
        }
        default => {
            return false;
        }
    }

}


func (cssParser : &mut CSSParser) parseWidth(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if(!cssParser.parseLengthOrAuto(parser, builder, value)) {
        parser.error("unknown value given for width");
    }
}

func (cssParser : &mut CSSParser) parseHeight(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    if(!cssParser.parseLengthOrAuto(parser, builder, value)) {
        parser.error("unknown value given for height");
    }
}