import "@std/string_view.ch"
import "./CSSValueKind.ch"

enum CSSPropertyKind {

    Unknown,

}

struct CSSProperty {

    var kind : CSSPropertyKind

    var name : std::string_view;

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