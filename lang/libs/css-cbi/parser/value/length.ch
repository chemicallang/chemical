import "@std/hashing/fnv1.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "../ast/CSSValueKind.ch"

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