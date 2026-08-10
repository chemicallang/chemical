public enum CSSGradientKind {
    None,
    Linear,
    Radial,
    Conic,
    RepeatingLinear,
    RepeatingRadial,
    RepeatingConic
}

public struct LinearColorStop {

    var color : CSSValue

    var length : CSSValue

    var optSecLength : CSSValue

}

public struct LinearColorStopWHint {

    var hint : CSSValue

    var stop : LinearColorStop

}

public struct LinearGradientData {

    var angle : CSSLengthValueData

    var to1 : CSSKeywordValueData

    var to2 : CSSKeywordValueData

    var color_stop_list : std::vector<LinearColorStopWHint>

}

public struct RadialSizeData {

    var extent : CSSKeywordValueData

    var length : CSSValue

}

public struct RadialGradientData {

    var shape : CSSKeywordValueData

    var size : RadialSizeData

    var position : CSSValue

    var color_stop_list : std::vector<LinearColorStopWHint>

}

public enum RectangularColorSpaceKind {

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

public struct RectangularColorSpace {

    var kind : RectangularColorSpaceKind = RectangularColorSpaceKind.None

    var value : std::string_view

}

public enum PolarColorSpaceKind {
    None,
    HSl,
    HWB,
    LCH,
    OKLCH
}

public struct PolarColorSpace {

    var kind : PolarColorSpaceKind = PolarColorSpaceKind.None

    var value : std::string_view

}

public struct PolarColorSpaceWInterpolation {

    var space : PolarColorSpace

    var interpolation : CSSKeywordValueData

}

public struct ColorInterpolationMethod {

    var rect_color_space : RectangularColorSpace

    var polar_color_space_and_inter : PolarColorSpaceWInterpolation

}

public struct ConicGradientData {

    var from : CSSValue

    var at : CSSValue

    var interpolation : ColorInterpolationMethod

    var color_stop_list : std::vector<LinearColorStopWHint>

}

public struct GradientData {

    var kind : CSSGradientKind = CSSGradientKind.None

    var data : *mut void = null

}