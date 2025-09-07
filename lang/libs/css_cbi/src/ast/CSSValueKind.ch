enum CSSValueKind {

    Unknown,

    Multiple,
    ChemicalValue,

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

    BackgroundImage,


}