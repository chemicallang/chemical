enum CSSKeywordKind {

    Unknown,

    // Global Values
    Inherit,
    Initial,
    Unset,

    // Sizing Keywords
    Auto,

    // var(--my-var)
    Var,

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
    Pixelated,

    // BackgroundRepeat
    Repeat,
    RepeatX,
    RepeatY,
    NoRepeat,

    // BackgroundAttachment
    Local,

    // BackgroundClip
    BorderBox,
    PaddingBox,
    ContentBox,

    // TextDecorationLine
    Underline,
    Overline,
    LineThrough,
    Blink,

    // TextDecorationStyle
    Solid,
    Double,
    Dotted,
    Dashed,
    Wavy,

    // TextJustify
    InterWord,
    InterIdeograph,
    Distribute,

    // GridAutoFlow
    Dense,

    // BorderImageRepeat
    Round,
    Space,

    // BreakAfter
    Page,

    // ColumnRuleStyle
    Groove,
    Ridge,
    Inset,
    Outset,

    // ColumnFill
    Balance,

    // BoxDecorationBreak
    Slice,
    Clone,

    // MaskMode
    Alpha,
    Luminance,

    // MaskComposite
    Add,
    Subtract,
    Intersect,
    Exclude,

    // ScrollbarWidth
    Thin,

    // TouchAction
    Manipulation,
    PanX,
    PanY,
    PinchZoom,

    // Hyphens
    Manual,

    // LineBreak
    Loose,
    Strict,

    // TextEmphasisStyle
    Dot,
    DoubleCircle,
    Triangle,
    Sesame,

    // TextEmphasisPosition
    Under,
    Over,

    // TextOrientation
    Mixed,
    Upright,
    Sideways,
    SidewaysRight,

    // Contain
    Content,
    Layout,
    Style,
    Paint,

    // PageOrientation
    Portrait,
    Landscape,

    // TransformBox
    FillBox,
    ViewBox,

    // FontVariantCaps
    AllSmallCaps,
    PetiteCaps,
    Unicase,
    TitlingCaps,

    // FontVariantNumeric
    LiningNums,
    OldstyleNums,
    ProportionalNums,
    TabularNums,
    DiagonalFractions,
    StackedFractions,

    // FontVariantEastAsian
    FullWidth,
    ProportionalWidth,
    Ruby,

    // ImageOrientation
    FromImage,

    // TransitionTimingFunction
    Ease,
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    StepStart,
    StepEnd,

    // Easing Function
    CubicBezier,
    Steps,

    // VectorEffect
    NonScalingStroke,

    // ColorScheme
    Light,
    Dark,

    // PrintColorAdjust
    Economy,
    Exact,

    // TextUnderlinePosition
    FromFont,

    // Scrollbar Gutter
    Stable,
    BothEdges,

    // System Font Keywords
    Caption,
    Icon,
    Menu,
    MessageBox,
    SmallCaption,
    StatusBar,

    // Font Width Keywords
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded,

    // Steps Function Call Step Position
    JumpStart,
    JumpEnd,
    JumpNone,
    JumpBoth,

    // Transition Behavior
    AllowDiscrete,

    // Transform Functions
    Matrix,
    Matrix3d,
    Perspective,
    Rotate,
    Rotate3d,
    RotateX,
    RotateY,
    RotateZ,
    Scale,
    Scale3d,
    ScaleX,
    ScaleY,
    ScaleZ,
    Skew,
    SkewX,
    SkewY,
    Translate,
    Translate3d,
    TranslateX,
    TranslateY,
    TranslateZ,

    // width or height keywords
    MinContent,
    MaxContent,
    FitContent,
    CalcSize,
    Width,
    Height,
    SelfBlock,
    SelfInline,

    // hue interpolation method
    Shorter,
    Longer,
    Increasing,
    Decreasing,

    Ellipse,

    ClosestSide,
    ClosestCorner,
    FarthestSide,
    FarthestCorner

}