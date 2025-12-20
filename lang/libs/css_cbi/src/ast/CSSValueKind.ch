enum CSSValueKind {

    Unknown,

    Multiple,
    Pair,
    ChemicalValue,
    Variable, // var(--)

    // keywords like auto
    Keyword,
    Length,
    Color,

    // used for width and height (fit-content call)
    SingleLengthFunctionCall,

    Border,
    BorderRadius,
    Font,
    FontFamily,
    BoxShadow,
    TextShadow,
    Transition,
    TransitionTimingFunction,
    Transform,
    Background,
    BackgroundImage,
    MultipleBackgroundImage,
    TextDecoration,


}