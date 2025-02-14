enum CSSKeywordKind {

    Unknown,

    // Global Values
    Inherit,
    Initial,
    Unset,

    // Sizing Keywords
    Auto,

    // Font Weights
    Normal,
    Bold,
    Bolder,
    Lighter,

    // Font Sizes
    XXSmall,
    XSmall,
    Small,
    Medium,
    Large,
    XLarge,
    XXLarge,
    Smaller,
    Larger,

    // Text Alignment, Float or Clear
    Left,
    Right,
    Center,
    Justify,
    Both,

    // Display Keywords
    None,
    Inline,
    Block,
    InlineBlock,
    Flex,
    Grid,

    // Position Keywords
    Static,
    Relative,
    Absolute,
    Fixed,
    Sticky,

    // Overflow Keywords
    Visible,
    Hidden,
    Scroll,

    // Vertical Align
    Baseline,
    Sub,
    Super,
    TextTop,
    TextBottom,
    Middle,
    Top,
    Bottom,

    // Whitespace
    Nowrap,
    Pre,
    PreWrap,
    PreLine,

    // Text Transform
    Capitalize,
    Uppercase,
    Lowercase,

    // Visibility
    Collapse,

    // Cursor
    Default,
    Pointer,
    Move,
    Text,
    Wait,
    Help,
    NotAllowed,

    // Direction
    Ltr,
    Rtl,

    // Resize
    Horizontal,
    Vertical,

    // Border Collapse
    Separate,

    // Text Overflow
    Clip,
    Ellipsis,

    // Overflow Wrap
    BreakWord,

    // Word Break
    BreakAll,
    KeepAll,

    // Object Fit
    Fill,
    Contain,
    Cover,
    ScaleDown,

    // Image Rendering
    CrispEdges,
    Pixelated

}