
enum CSSPropertyKind {

    Unknown,

}

struct CSSProperty {

    var kind : CSSPropertyKind

    var name : std::string_view;

}

// any keyword that is being stored as a value like 'auto'
@direct_init
struct CSSKeywordValueData {

    var kind : CSSKeywordKind

    var value : std::string_view

    @make
    func make() {
        kind = CSSKeywordKind.Unknown
    }

}

@direct_init
struct CSSLengthValueData {

    var kind : CSSLengthKind

    // we store length value as string_view for printing it fast
    // since we don't need to check the value
    var value : std::string_view

    @make
    func make() {
        kind = CSSLengthKind.Unknown
    }

}

struct SingleLengthFuncCall {

    var name : CSSKeywordValueData

    var length : CSSLengthValueData

}

struct CSSRGBColorData {

    var red : CSSLengthValueData

    var green : CSSLengthValueData

    var blue : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSHSLColorData {

    var hue : CSSLengthValueData

    var saturation : CSSLengthValueData

    var lightness : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSHWBColorData {

    var hue : CSSLengthValueData

    var whiteness : CSSLengthValueData

    var blackness : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSLABColorData {

    var lightness : CSSLengthValueData

    var rgAxis : CSSLengthValueData

    var byAxis : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSLCHColorData {

    var lightness : CSSLengthValueData

    var chroma : CSSLengthValueData

    var hue : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSOKLABColorData {

    var lightness : CSSLengthValueData

    var aAxis : CSSLengthValueData

    var bAxis : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSOKLCHColorData {

    var lightness : CSSLengthValueData

    var pChroma : CSSLengthValueData

    var hue : CSSLengthValueData

    var alpha : CSSLengthValueData

}

struct CSSColorValueData {

    var kind : CSSColorKind

    union {

        var view : std::string_view

        var rgbData : *CSSRGBColorData

        var hslData : *CSSHSLColorData

        var hwbData : *CSSHWBColorData

        var labData : *CSSLABColorData

        var lchData : *CSSLCHColorData

        var oklabData : *CSSOKLABColorData

        var oklchData : *CSSOKLCHColorData

    } value;

}

struct CSSValuePair {

    var first : CSSValue

    var second : CSSValue

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

struct UrlData {

    var value : std::string_view

    var is_source : bool = false;

}

struct BackgroundImageData {

    var is_url : bool = true;

    var url : UrlData

    var gradient : GradientData

    @make
    func make() {
        url = UrlData()
        gradient = GradientData()
    }

}

struct MultipleBackgroundImageData {

    var images : std::vector<BackgroundImageData>

}

struct CSSBackgroundLayerData {

    var image : CSSValue

    var positionX : CSSValue

    var positionY : CSSValue

    var size : CSSValue

    var repeat : CSSValue

    var attachment : CSSValue

    var origin : CSSValue

    var clip : CSSValue

}

struct CSSBackgroundValueData {

    var color : CSSValue

    var layers : std::vector<CSSBackgroundLayerData>

}

struct CSSLinearEasingPoint {

    // the duration point
    var point : CSSLengthValueData

    var start : CSSLengthValueData

    var stop : CSSLengthValueData

    var next : *CSSLinearEasingPoint

    @make
    func make() {
        next = null
    }

}

struct CSSCubicBezierEasingData {

    var x1 : CSSLengthValueData

    var y1 : CSSLengthValueData

    var x2 : CSSLengthValueData

    var y2 : CSSLengthValueData

}

struct CSSStepsEasingData {

    var step : CSSLengthValueData

    var position : CSSKeywordValueData

}

struct CSSEasingFunction {

    var kind : CSSKeywordKind

    union {

        var keyword : CSSKeywordValueData

        var linear : *CSSLinearEasingPoint

        var bezier : *CSSCubicBezierEasingData

        var steps : *CSSStepsEasingData

    } data;

    @make
    func make() {
        kind = CSSKeywordKind.Unknown
    }

}

struct CSSTransitionValueData {

    // can be 'all', 'none', a css property name or even empty
    var property : std::string_view

    var duration : CSSLengthValueData

    var easing : CSSEasingFunction

    var delay : CSSLengthValueData

    var behavior : CSSKeywordValueData

    var next : *mut CSSTransitionValueData

    @make
    func make() {
        next = null
    }

}

struct CSSTransformLengthNode {

    var length : CSSLengthValueData

    var next : *mut CSSTransformLengthNode

    @make
    func make() {
        next = null
    }

}

struct CSSTransformValueData {

    var transformFunction : CSSKeywordValueData

    var node : *mut CSSTransformLengthNode

    var next : *mut CSSTransformValueData

    @make
    func make() {
        node = null
        next = null
    }

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
 
struct CSSTextDecorationValueData {
 
    var line : CSSValue
 
    var style : CSSValue
 
    var color : CSSValue
 
    var thickness : CSSValue
 
};
 
enum CSSCalcExpressionKind {
    Literal,
    Operation,
    Group
}
 
struct CSSCalcExpression {
    var kind : CSSCalcExpressionKind
    var data : *mut void
}
 
struct CSSCalcOperationData {
    var left : CSSCalcExpression
    var right : CSSCalcExpression
    var op : char
}
 
struct CSSCalcValueData {
    var expression : CSSCalcExpression
};
 
@direct_init
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

    var important : bool

}