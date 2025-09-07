enum CSSGradientKind {
    None,
    Linear,
    Radial,
    Conic,
    RepeatingLinear,
    RepeatingRadial,
    RepeatingConic
}

struct LinearColorStop {

    var color : CSSValue

    var length : CSSValue

    var optSecLength : CSSValue

}

struct LinearColorStopWHint {

    var hint : CSSValue

    var stop : LinearColorStop

}

struct LinearGradientData {

    var angle : CSSLengthValueData

    var to : CSSKeywordValueData

    var color_stop_list : std::vector<LinearColorStopWHint>

}

struct RadialSizeData {

    var extent : CSSKeywordValueData

    var length : CSSValue

}

struct RadialGradientData {

    var shape : CSSKeywordValueData

    var size : RadialSizeData

    var position : CSSValue

}

enum RectangularColorSpaceKind {

    None,
    Srgb,
    SrgbLinear,
    DisplayP3,
    DisplayP3Linear,
    A98Rgb,
    ProphotoRgb,
    Rec2020,
    Lab,
    Oklab,
    XYZ,
    XYZd50,
    XYZd65

}

struct RectangularColorSpace {

    var kind : RectangularColorSpaceKind

    var value : std::string_view

}

enum PolarColorSpaceKind {
    None,
    HSl,
    HWB,
    LCH,
    OKLCH
}

struct PolarColorSpace {

    var kind : PolarColorSpaceKind

    var value : std::string_view

}

struct PolarColorSpaceWInterpolation {

    var space : PolarColorSpace

    var interpolation : CSSKeywordValueData

}

struct ColorInterpolationMethod {

    var rect_color_space : RectangularColorSpace

    var polar_color_space_and_inter : PolarColorSpaceWInterpolation

}

struct ConicGradientData {

    var from : CSSValue

    var at : CSSValue

    var interpolation : ColorInterpolationMethod

}

struct GradientData {

    var kind : CSSGradientKind = CSSGradientKind.None

    var data : *mut void = null

}