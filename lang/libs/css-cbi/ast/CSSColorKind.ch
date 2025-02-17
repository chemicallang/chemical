enum CSSColorKind {

    Unknown,

    NamedColor,
    SystemColor,
    Transparent,
    CurrentColor,
    HexColor,

    // Color Functions
    RGB,
    RGBA,
    HSL,
    HSLA,
    HWB,
    LAB,
    LCH,
    OKLAB,
    OKLCH,
    COLOR,

    // Indexes into the enum itself
    FunctionsStart = RGB,
    FunctionsEnd = COLOR

}