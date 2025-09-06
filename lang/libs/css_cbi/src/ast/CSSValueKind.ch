enum CSSValueKind {

    Unknown,

    Multiple,

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
    Transform,

    BackgroundImage,


}