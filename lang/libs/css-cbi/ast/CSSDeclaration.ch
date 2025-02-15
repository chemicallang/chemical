import "@std/string_view.ch"
import "@std/vector.ch"
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

    @make
    func make() {
        kind = CSSKeywordKind.Unknown
    }

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

struct CSSMultipleValues {

    var values : std::vector<CSSValue>

}

/**
 * the struct used for border value
 * value with kind Unknown is present if a single value (width / style) is NOT given
 */
struct CSSBorderValueData {

    var width : CSSValue

    var style : CSSValue

    var color : CSSValue

}

struct CSSBorderRadiusValueData {

    var first : CSSLengthValueData

    var second : CSSLengthValueData

    var third : CSSLengthValueData

    var fourth : CSSLengthValueData

    var next : *mut CSSBorderRadiusValueData

}

variant CSSFontStyle {
    None()
    Keyword(keyword : CSSKeywordValueData)
    Oblique(view : std::string_view)
}

variant CSSFontWeight {
    None();
    Keyword(keyword : CSSKeywordValueData)
    Absolute(view : std::string_view)
}

struct CSSFontFamily {

    var families : std::vector<std::string_view>

}

struct CSSFontValueData {

    var style       : CSSFontStyle

    var fontVariant : CSSKeywordValueData

    var weight      : CSSFontWeight

    var stretch     : CSSKeywordValueData

    var size        : CSSValue

    var lineHeight  : CSSValue

    var family      : CSSFontFamily

    @make
    func make() {
        style = CSSFontStyle.None()
        weight = CSSFontWeight.None()
    }

}

struct CSSValue {

    var kind : CSSValueKind

    var data : *mut void

    @make
    func empty() {
        kind = CSSValueKind.Unknown
        data = null
    }

}

struct CSSDeclaration {

    var property : CSSProperty

    var value : CSSValue

}