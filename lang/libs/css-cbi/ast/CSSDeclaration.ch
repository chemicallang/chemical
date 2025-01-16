import "@std/string_view.ch"

enum CSSPropertyKind {

    Unknown,

}

struct CSSProperty {

    var kind : CSSPropertyKind

    var name : std::string_view;

}

enum CSSValueKind {

    LengthPX,
    LengthEM,
    LengthREM,
    LengthVH,
    LengthVW,
    LengthVMIN,
    LengthVMAX,
    LengthPERCENTAGE,
    LengthCM,
    LengthMM,
    LengthIN,
    LengthPT,
    LengthPC,
    LengthCH,
    LengthEX,
    LengthS,
    LengthMS,
    LengthHZ,
    LengthKHZ,
    LengthDEG,
    LengthRAD,
    LengthGRAD,
    LengthTURN

}

struct CSSValueData {

}

// we store length value as string_view
struct CSSNumberValueData : CSSValueData {

    var value : std::string_view

}

struct CSSValue {

    var kind : CSSValueKind

    var data : *mut void

}

struct CSSDeclaration {

    var property : CSSProperty

    var value : CSSValue

}