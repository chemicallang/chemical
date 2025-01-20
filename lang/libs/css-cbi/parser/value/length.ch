import "@std/hashing/fnv1.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "../ast/CSSValueKind.ch"

func getLengthKind(str : *char) : CSSValueKind {
    switch(fnv1_hash(str)) {
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
            return CSSValueKind.Unknown
        }
    }
}

func parseLengthKind(parser : *mut Parser, builder : *mut ASTBuilder) : CSSValueKind {
    const token = parser.getToken();
    if(token.type == TokenType.Percentage) {
        parser.increment();
        return CSSValueKind.LengthPERCENTAGE
    } else if(token.type == TokenType.Identifier) {
        parser.increment();
        const kind = getLengthKind(token.value.data())
        if(kind != CSSValueKind.Unknown) {
            return kind;
        } else {
            parser.error("unknown length unit");
            return CSSValueKind.LengthPX
        }
    } else {
        parser.error("unknown unit token found");
        parser.increment()
        return CSSValueKind.LengthPX
    }
}