import "@std/string_view.ch"
import "./CSSValueKind.ch"
import "./CSSLengthKind.ch"
import "./CSSColorKind.ch"
import "./CSSKeywordKind.ch"

enum CSSPropertyKind {

    Unknown,

}

struct CSSProperty {

    var kind : CSSPropertyKind

    var name : std::string_view;

}

// any keyword that is being stored as a value like 'auto'
struct CSSKeywordValueData {

    var kind : CSSKeywordKind

    var value : std::string_view

}


struct CSSLengthValueData {

    var kind : CSSLengthKind

    // we store length value as string_view for printing it fast
    // since we don't need to check the value
    var value : std::string_view

}

struct CSSColorValueData {

    var kind : CSSColorKind

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