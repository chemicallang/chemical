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

struct CSSRGBColorData {

    var red : CSSLengthValueData

    var green : CSSLengthValueData

    var blue : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSColorValueData {

    var kind : CSSColorKind

    union {

        var view : std::string_view

        var rgbData : *CSSRGBColorData

    } value;

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

}

struct CSSBoxShadowValueData {

    var inset : bool;
    var offsetX : CSSValue;
    var offsetY : CSSValue;
    var blurRadius : CSSValue;   // optional, default to 0 if not provided
    var spreadRadius : CSSValue; // optional, default to 0 if not provided
    var color : CSSValue;        // optional, can be left undefined

    var next : *mut CSSBoxShadowValueData

    @make
    func empty() {
        inset = false;
        next = null
    }

    func isEmpty(&self) : bool {
        return inset == false && offsetX.isUnknown() && offsetY.isUnknown() && blurRadius.isUnknown() && spreadRadius.isUnknown() && color.isUnknown()
    }

};

struct CSSTextShadowValueData {

    var offsetX : CSSValue;
    var offsetY : CSSValue;
    var blurRadius : CSSValue; // optional; default to 0 if not provided
    var color : CSSValue;      // optional; if not provided, use current color

    var next : *mut CSSTextShadowValueData

    @make
    func empty() {
        next = null
    }

    func isEmpty(&self) : bool {
        return offsetX.isUnknown() && offsetY.isUnknown() && blurRadius.isUnknown() && color.isUnknown()
    }

};

struct CSSValue {

    var kind : CSSValueKind

    var data : *mut void

    @make
    func empty() {
        kind = CSSValueKind.Unknown
        data = null
    }

    func isUnknown(&self) : bool {
        return kind == CSSValueKind.Unknown;
    }

}

struct CSSDeclaration {

    var property : CSSProperty

    var value : CSSValue

}