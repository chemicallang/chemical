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

    // Font Style Keywords
    Italic,
    Oblique,

    // Font Variant
    SmallCaps,

    // List Style Type
    Disc,
    Circle,
    Square,
    Decimal,
    DecimalLeadingZero,
    LowerRoman,
    UpperRoman,

    // List Style Position
    Inside,
    Outside,

    // Align Items
    Stretch,
    FlexStart,
    FlexEnd,
    Start,
    End,
    SelfStart,
    SelfEnd,

    // Align Content
    SpaceBetween,
    SpaceAround,

    // Flex Direction
    Row,
    RowReverse,
    Column,
    ColumnReverse,

    // Flex Wrap
    Wrap,
    WrapReverse,

    // Empty Cells
    Show,
    Hide,

    // Page Break before, after
    Always,
    Avoid,

    // Isolation
    Isolate,

    // Mix Blend Mode
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity,

    // User Select
    All,

    // Scroll Behavior
    Smooth,

    // Writing Mode
    HorizontalTB,
    VerticalRL,
    VerticalLR,

    // Animation Direction
    Reverse,
    Alternate,
    AlternateReverse,

    // Animation Fill Mode
    Forwards,
    Backwards,

    // Animation Play State
    Running,
    Paused,

    // Clip Rule
    NonZero,
    EvenOdd,

    // Shape Rendering
    OptimizeSpeed,
    CrispEdges,
    GeometricPrecision,

    // Text Rendering
    OptimizeLegibility,

    // Transform Style
    Flat,
    Preserve3d,

    // Unicode Bidi
    Embed,
    BidiOverride,
    IsolateOverride,

    /// Justify Content
    SpaceEvenly,

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
    Pixelated

}