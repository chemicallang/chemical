
func getBorderStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("dotted") => { return CSSKeywordKind.Dotted }
        comptime_fnv1_hash("dashed") => { return CSSKeywordKind.Dashed }
        comptime_fnv1_hash("solid") => { return CSSKeywordKind.Solid }
        comptime_fnv1_hash("double") => { return CSSKeywordKind.Double }
        comptime_fnv1_hash("groove") => { return CSSKeywordKind.Groove }
        comptime_fnv1_hash("ridge") => { return CSSKeywordKind.Ridge }
        comptime_fnv1_hash("inset") => { return CSSKeywordKind.Inset }
        comptime_fnv1_hash("outset") => { return CSSKeywordKind.Outset }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTransformFunctionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("matrix") => { return CSSKeywordKind.Matrix }
        comptime_fnv1_hash("matrix3d") => { return CSSKeywordKind.Matrix3d }
        comptime_fnv1_hash("perspective") => { return CSSKeywordKind.Perspective }
        comptime_fnv1_hash("rotate") => { return CSSKeywordKind.Rotate }
        comptime_fnv1_hash("rotate3d") => { return CSSKeywordKind.Rotate3d }
        comptime_fnv1_hash("rotateX") => { return CSSKeywordKind.RotateX }
        comptime_fnv1_hash("rotateY") => { return CSSKeywordKind.RotateY }
        comptime_fnv1_hash("rotateZ") => { return CSSKeywordKind.RotateZ }
        comptime_fnv1_hash("scale") => { return CSSKeywordKind.Scale }
        comptime_fnv1_hash("scale3d") => { return CSSKeywordKind.Scale3d }
        comptime_fnv1_hash("scaleX") => { return CSSKeywordKind.ScaleX }
        comptime_fnv1_hash("scaleY") => { return CSSKeywordKind.ScaleY }
        comptime_fnv1_hash("scaleZ") => { return CSSKeywordKind.ScaleZ }
        comptime_fnv1_hash("skew") => { return CSSKeywordKind.Skew }
        comptime_fnv1_hash("skewX") => { return CSSKeywordKind.SkewX }
        comptime_fnv1_hash("skewY") => { return CSSKeywordKind.SkewY }
        comptime_fnv1_hash("translate") => { return CSSKeywordKind.Translate }
        comptime_fnv1_hash("translate3d") => { return CSSKeywordKind.Translate3d }
        comptime_fnv1_hash("translateX") => { return CSSKeywordKind.TranslateX }
        comptime_fnv1_hash("translateY") => { return CSSKeywordKind.TranslateY }
        comptime_fnv1_hash("translateZ") => { return CSSKeywordKind.TranslateZ }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAnimationTimingFunctionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("ease") => { return CSSKeywordKind.Ease }
        comptime_fnv1_hash("linear") => { return CSSKeywordKind.Linear }
        comptime_fnv1_hash("ease-in") => { return CSSKeywordKind.EaseIn }
        comptime_fnv1_hash("ease-out") => { return CSSKeywordKind.EaseOut }
        comptime_fnv1_hash("ease-in-out") => { return CSSKeywordKind.EaseInOut }
        comptime_fnv1_hash("step-start") => { return CSSKeywordKind.StepStart }
        comptime_fnv1_hash("step-end") => { return CSSKeywordKind.StepEnd }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTransitionBehaviorKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("allow-discrete") => { return CSSKeywordKind.AllowDiscrete }
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskBorderModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("alpha") => { return CSSKeywordKind.Alpha }
        comptime_fnv1_hash("luminance") => { return CSSKeywordKind.Luminance }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskBorderRepeatKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch }
        comptime_fnv1_hash("repeat") => { return CSSKeywordKind.Repeat }
        comptime_fnv1_hash("round") => { return CSSKeywordKind.Round }
        comptime_fnv1_hash("space") => { return CSSKeywordKind.Space }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextDecorationSkipInkKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextUnderlinePositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("under") => { return CSSKeywordKind.Under }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("from-font") => { return CSSKeywordKind.FromFont }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontOpticalSizingKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTransitionTimingFunctionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("ease") => { return CSSKeywordKind.Ease }
        comptime_fnv1_hash("linear") => { return CSSKeywordKind.Linear }
        comptime_fnv1_hash("ease-in") => { return CSSKeywordKind.EaseIn }
        comptime_fnv1_hash("ease-out") => { return CSSKeywordKind.EaseOut }
        comptime_fnv1_hash("ease-in-out") => { return CSSKeywordKind.EaseInOut }
        comptime_fnv1_hash("step-start") => { return CSSKeywordKind.StepStart }
        comptime_fnv1_hash("step-end") => { return CSSKeywordKind.StepEnd }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getVectorEffectKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("non-scaling-stroke") => { return CSSKeywordKind.NonScalingStroke }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getForcedColorAdjustKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getColorSchemeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("light") => { return CSSKeywordKind.Light }
        comptime_fnv1_hash("dark") => { return CSSKeywordKind.Dark }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPrintColorAdjustKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("economy") => { return CSSKeywordKind.Economy }
        comptime_fnv1_hash("exact") => { return CSSKeywordKind.Exact }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getOverscrollBehaviorKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("contain") => { return CSSKeywordKind.Contain }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPageOrientationKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("portrait") => { return CSSKeywordKind.Portrait }
        comptime_fnv1_hash("landscape") => { return CSSKeywordKind.Landscape }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextCombineUprightKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("all") => { return CSSKeywordKind.All }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontKerningKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getOutlineStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("dotted") => { return CSSKeywordKind.Dotted }
        comptime_fnv1_hash("dashed") => { return CSSKeywordKind.Dashed }
        comptime_fnv1_hash("solid") => { return CSSKeywordKind.Solid }
        comptime_fnv1_hash("double") => { return CSSKeywordKind.Double }
        comptime_fnv1_hash("groove") => { return CSSKeywordKind.Groove }
        comptime_fnv1_hash("ridge") => { return CSSKeywordKind.Ridge }
        comptime_fnv1_hash("inset") => { return CSSKeywordKind.Inset }
        comptime_fnv1_hash("outset") => { return CSSKeywordKind.Outset }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTransformBoxKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("border-box") => { return CSSKeywordKind.BorderBox }
        comptime_fnv1_hash("fill-box") => { return CSSKeywordKind.FillBox }
        comptime_fnv1_hash("view-box") => { return CSSKeywordKind.ViewBox }
        comptime_fnv1_hash("content-box") => { return CSSKeywordKind.ContentBox }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontVariantCapsKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("small-caps") => { return CSSKeywordKind.SmallCaps }
        comptime_fnv1_hash("all-small-caps") => { return CSSKeywordKind.AllSmallCaps }
        comptime_fnv1_hash("petite-caps") => { return CSSKeywordKind.PetiteCaps }
        comptime_fnv1_hash("unicase") => { return CSSKeywordKind.Unicase }
        comptime_fnv1_hash("titling-caps") => { return CSSKeywordKind.TitlingCaps }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontVariantNumericKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("lining-nums") => { return CSSKeywordKind.LiningNums }
        comptime_fnv1_hash("oldstyle-nums") => { return CSSKeywordKind.OldstyleNums }
        comptime_fnv1_hash("proportional-nums") => { return CSSKeywordKind.ProportionalNums }
        comptime_fnv1_hash("tabular-nums") => { return CSSKeywordKind.TabularNums }
        comptime_fnv1_hash("diagonal-fractions") => { return CSSKeywordKind.DiagonalFractions }
        comptime_fnv1_hash("stacked-fractions") => { return CSSKeywordKind.StackedFractions }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontVariantEastAsianKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("full-width") => { return CSSKeywordKind.FullWidth }
        comptime_fnv1_hash("proportional-width") => { return CSSKeywordKind.ProportionalWidth }
        comptime_fnv1_hash("ruby") => { return CSSKeywordKind.Ruby }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getImageOrientationKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("from-image") => { return CSSKeywordKind.FromImage }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontWeightKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("bold") => { return CSSKeywordKind.Bold }
        comptime_fnv1_hash("bolder") => { return CSSKeywordKind.Bolder }
        comptime_fnv1_hash("lighter") => { return CSSKeywordKind.Lighter }
        default => {  return CSSKeywordKind.Unknown }
    }
}

func getFontStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("italic") => { return CSSKeywordKind.Italic }
        comptime_fnv1_hash("oblique") => { return CSSKeywordKind.Oblique }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontVariantKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("small-caps") => { return CSSKeywordKind.SmallCaps }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getListStyleTypeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("disc") => { return CSSKeywordKind.Disc; }
        comptime_fnv1_hash("circle") => { return CSSKeywordKind.Circle }
        comptime_fnv1_hash("square") => { return CSSKeywordKind.Square }
        comptime_fnv1_hash("decimal") => { return CSSKeywordKind.Decimal }
        comptime_fnv1_hash("decimal-leading-zero") => { return CSSKeywordKind.DecimalLeadingZero }
        comptime_fnv1_hash("lower-roman") => { return CSSKeywordKind.LowerRoman }
        comptime_fnv1_hash("upper-roman") => { return CSSKeywordKind.UpperRoman }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getListStylePositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("inside") => { return CSSKeywordKind.Inside; }
        comptime_fnv1_hash("outside") => { return CSSKeywordKind.Outside }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAlignItemsKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch; }
        comptime_fnv1_hash("flex-start") => { return CSSKeywordKind.FlexStart }
        comptime_fnv1_hash("flex-end") => { return CSSKeywordKind.FlexEnd }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("baseline") => { return CSSKeywordKind.Baseline }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        comptime_fnv1_hash("self-start") => { return CSSKeywordKind.SelfStart }
        comptime_fnv1_hash("self-end") => { return CSSKeywordKind.SelfEnd }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAlignContentKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch; }
        comptime_fnv1_hash("flex-start") => { return CSSKeywordKind.FlexStart }
        comptime_fnv1_hash("flex-end") => { return CSSKeywordKind.FlexEnd }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("space-between") => { return CSSKeywordKind.SpaceBetween }
        comptime_fnv1_hash("space-around") => { return CSSKeywordKind.SpaceAround }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getJustifyContentKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("flex-start") => { return CSSKeywordKind.FlexStart }
        comptime_fnv1_hash("flex-end") => { return CSSKeywordKind.FlexEnd }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("space-between") => { return CSSKeywordKind.SpaceBetween }
        comptime_fnv1_hash("space-around") => { return CSSKeywordKind.SpaceAround }
        comptime_fnv1_hash("space-evenly") => { return CSSKeywordKind.SpaceEvenly }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontSizeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("xx-small") => { return CSSKeywordKind.XXSmall; }
        comptime_fnv1_hash("x-small") => { return CSSKeywordKind.XSmall }
        comptime_fnv1_hash("small") => { return CSSKeywordKind.Small }
        comptime_fnv1_hash("medium") => { return CSSKeywordKind.Medium }
        comptime_fnv1_hash("large") => { return CSSKeywordKind.Large }
        comptime_fnv1_hash("x-large") => { return CSSKeywordKind.XLarge }
        comptime_fnv1_hash("xx-large") => { return CSSKeywordKind.XXLarge }
        comptime_fnv1_hash("smaller") => { return CSSKeywordKind.Smaller }
        comptime_fnv1_hash("larger") => { return CSSKeywordKind.Larger }
        default => { return CSSKeywordKind.Unknown }
    }
}


func getTextAlignKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left; }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("justify") => { return CSSKeywordKind.Justify }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getDisplayKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("inline") => { return CSSKeywordKind.Inline }
        comptime_fnv1_hash("block") => { return CSSKeywordKind.Block }
        comptime_fnv1_hash("inline-block") => { return CSSKeywordKind.InlineBlock }
        comptime_fnv1_hash("flex") => { return CSSKeywordKind.Flex }
        comptime_fnv1_hash("grid") => { return CSSKeywordKind.Grid }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("static") => { return CSSKeywordKind.Static; }
        comptime_fnv1_hash("relative") => { return CSSKeywordKind.Relative }
        comptime_fnv1_hash("absolute") => { return CSSKeywordKind.Absolute }
        comptime_fnv1_hash("fixed") => { return CSSKeywordKind.Fixed }
        comptime_fnv1_hash("sticky") => { return CSSKeywordKind.Sticky }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getOverflowKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("visible") => { return CSSKeywordKind.Visible; }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("scroll") => { return CSSKeywordKind.Scroll }
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFloatKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left; }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getClearKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("both") => { return CSSKeywordKind.Both }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getVerticalAlignKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("baseline") => { return CSSKeywordKind.Baseline; }
        comptime_fnv1_hash("sub") => { return CSSKeywordKind.Sub }
        comptime_fnv1_hash("super") => { return CSSKeywordKind.Super }
        comptime_fnv1_hash("text-top") => { return CSSKeywordKind.TextTop }
        comptime_fnv1_hash("text-bottom") => { return CSSKeywordKind.TextBottom }
        comptime_fnv1_hash("middle") => { return CSSKeywordKind.Middle }
        comptime_fnv1_hash("top") => { return CSSKeywordKind.Top }
        comptime_fnv1_hash("bottom") => { return CSSKeywordKind.Bottom }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getWhitespaceKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("nowrap") => { return CSSKeywordKind.Nowrap }
        comptime_fnv1_hash("pre") => { return CSSKeywordKind.Pre }
        comptime_fnv1_hash("pre-wrap") => { return CSSKeywordKind.PreWrap }
        comptime_fnv1_hash("pre-line") => { return CSSKeywordKind.PreLine }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextTransformKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("capitalize") => { return CSSKeywordKind.Capitalize }
        comptime_fnv1_hash("uppercase") => { return CSSKeywordKind.Uppercase }
        comptime_fnv1_hash("lowercase") => { return CSSKeywordKind.Lowercase }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getVisibilityKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("visible") => { return CSSKeywordKind.Visible; }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("collapse") => { return CSSKeywordKind.Collapse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getCursorKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("default") => { return CSSKeywordKind.Default }
        comptime_fnv1_hash("pointer") => { return CSSKeywordKind.Pointer }
        comptime_fnv1_hash("move") => { return CSSKeywordKind.Move }
        comptime_fnv1_hash("text") => { return CSSKeywordKind.Text }
        comptime_fnv1_hash("wait") => { return CSSKeywordKind.Wait }
        comptime_fnv1_hash("help") => { return CSSKeywordKind.Help }
        comptime_fnv1_hash("not-allowed") => { return CSSKeywordKind.NotAllowed }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getDirectionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("ltr") => { return CSSKeywordKind.Ltr; }
        comptime_fnv1_hash("rtl") => { return CSSKeywordKind.Rtl }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getResizeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("both") => { return CSSKeywordKind.Both }
        comptime_fnv1_hash("horizontal") => { return CSSKeywordKind.Horizontal }
        comptime_fnv1_hash("vertical") => { return CSSKeywordKind.Vertical }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTableLayoutKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("fixed") => { return CSSKeywordKind.Fixed }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBorderCollapseKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("collapse") => { return CSSKeywordKind.Collapse; }
        comptime_fnv1_hash("separate") => { return CSSKeywordKind.Separate }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextOverflowKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("clip") => { return CSSKeywordKind.Clip; }
        comptime_fnv1_hash("ellipsis") => { return CSSKeywordKind.Ellipsis }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getOverflowWrapKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("break-word") => { return CSSKeywordKind.BreakWord }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getWordBreakKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("break-all") => { return CSSKeywordKind.BreakAll; }
        comptime_fnv1_hash("keep-all") => { return CSSKeywordKind.KeepAll; }
        comptime_fnv1_hash("break-word") => { return CSSKeywordKind.BreakWord }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getObjectFitKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("fill") => { return CSSKeywordKind.Fill; }
        comptime_fnv1_hash("contain") => { return CSSKeywordKind.Contain; }
        comptime_fnv1_hash("cover") => { return CSSKeywordKind.Cover; }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("scale-down") => { return CSSKeywordKind.ScaleDown }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getImageRenderingKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("crisp-edges") => { return CSSKeywordKind.CrispEdges; }
        comptime_fnv1_hash("pixelated") => { return CSSKeywordKind.Pixelated; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackFaceVisibilityKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("visible") => { return CSSKeywordKind.Visible; }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFlexDirectionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("row") => { return CSSKeywordKind.Row }
        comptime_fnv1_hash("row-reverse") => { return CSSKeywordKind.RowReverse }
        comptime_fnv1_hash("column") => { return CSSKeywordKind.Column }
        comptime_fnv1_hash("column-reverse") => { return CSSKeywordKind.ColumnReverse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFlexWrapKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("nowrap") => { return CSSKeywordKind.Nowrap; }
        comptime_fnv1_hash("wrap") => { return CSSKeywordKind.Wrap; }
        comptime_fnv1_hash("wrap-reverse") => { return CSSKeywordKind.WrapReverse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAlignSelfKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch; }
        comptime_fnv1_hash("flex-start") => { return CSSKeywordKind.FlexStart }
        comptime_fnv1_hash("flex-end") => { return CSSKeywordKind.FlexEnd }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("baseline") => { return CSSKeywordKind.Baseline }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        comptime_fnv1_hash("self-start") => { return CSSKeywordKind.SelfStart }
        comptime_fnv1_hash("self-end") => { return CSSKeywordKind.SelfEnd }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getCaptionSideKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("top") => { return CSSKeywordKind.Top; }
        comptime_fnv1_hash("bottom") => { return CSSKeywordKind.Bottom; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getEmptyCellsKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("show") => { return CSSKeywordKind.Show; }
        comptime_fnv1_hash("hide") => { return CSSKeywordKind.Hide; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPageBreakInsideKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPageBreakBeforeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("always") => { return CSSKeywordKind.Always; }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPageBreakAfterKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("always") => { return CSSKeywordKind.Always; }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getIsolationKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("isolate") => { return CSSKeywordKind.Isolate; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMixBlendModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("multiply") => { return CSSKeywordKind.Multiply; }
        comptime_fnv1_hash("screen") => { return CSSKeywordKind.Screen }
        comptime_fnv1_hash("overlay") => { return CSSKeywordKind.Overlay }
        comptime_fnv1_hash("darken") => { return CSSKeywordKind.Darken }
        comptime_fnv1_hash("lighten") => { return CSSKeywordKind.Lighten }
        comptime_fnv1_hash("color-dodge") => { return CSSKeywordKind.ColorDodge }
        comptime_fnv1_hash("color-burn") => { return CSSKeywordKind.ColorBurn }
        comptime_fnv1_hash("hard-light") => { return CSSKeywordKind.HardLight }
        comptime_fnv1_hash("soft-light") => { return CSSKeywordKind.SoftLight }
        comptime_fnv1_hash("difference") => { return CSSKeywordKind.Difference }
        comptime_fnv1_hash("exclusion") => { return CSSKeywordKind.Exclusion }
        comptime_fnv1_hash("hue") => { return CSSKeywordKind.Hue }
        comptime_fnv1_hash("saturation") => { return CSSKeywordKind.Saturation }
        comptime_fnv1_hash("color") => { return CSSKeywordKind.Color }
        comptime_fnv1_hash("luminosity") => { return CSSKeywordKind.Luminosity }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getUserSelectKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("text") => { return CSSKeywordKind.Text; }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("all") => { return CSSKeywordKind.All }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getScrollBehaviorKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("smooth") => { return CSSKeywordKind.Smooth; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getScrollSnapAlignKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start; }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End; }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getJustifyItemsKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start; }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End; }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getWritingModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("horizontal-tb") => { return CSSKeywordKind.HorizontalTB; }
        comptime_fnv1_hash("vertical-rl") => { return CSSKeywordKind.VerticalRL; }
        comptime_fnv1_hash("vertical-lr") => { return CSSKeywordKind.VerticalLR }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAnimationDirectionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("reverse") => { return CSSKeywordKind.Reverse; }
        comptime_fnv1_hash("alternate") => { return CSSKeywordKind.Alternate }
        comptime_fnv1_hash("alternate-reverse") => { return CSSKeywordKind.AlternateReverse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAnimationFillModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("forwards") => { return CSSKeywordKind.Forwards; }
        comptime_fnv1_hash("backwards") => { return CSSKeywordKind.Backwards }
        comptime_fnv1_hash("both") => { return CSSKeywordKind.Both }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAnimationPlayStateKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("running") => { return CSSKeywordKind.Running; }
        comptime_fnv1_hash("paused") => { return CSSKeywordKind.Paused; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getAppearanceKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getOverflowAnchorKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getScrollSnapStopKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("always") => { return CSSKeywordKind.Always; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getClipRuleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("nonzero") => { return CSSKeywordKind.NonZero; }
        comptime_fnv1_hash("evenodd") => { return CSSKeywordKind.EvenOdd; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getShapeRenderingKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("optimizeSpeed") => { return CSSKeywordKind.OptimizeSpeed; }
        comptime_fnv1_hash("crispEdges") => { return CSSKeywordKind.CrispEdges }
        comptime_fnv1_hash("geometricPrecision") => { return CSSKeywordKind.GeometricPrecision }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextRenderingKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto; }
        comptime_fnv1_hash("optimizeSpeed") => { return CSSKeywordKind.OptimizeSpeed; }
        comptime_fnv1_hash("optimizeLegibility") => { return CSSKeywordKind.OptimizeLegibility }
        comptime_fnv1_hash("geometricPrecision") => { return CSSKeywordKind.GeometricPrecision }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTransformStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("flat") => { return CSSKeywordKind.Flat; }
        comptime_fnv1_hash("preserve-3d") => { return CSSKeywordKind.Preserve3d; }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getUnicodeBidiKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal; }
        comptime_fnv1_hash("embed") => { return CSSKeywordKind.Embed; }
        comptime_fnv1_hash("isolate") => { return CSSKeywordKind.Isolate }
        comptime_fnv1_hash("bidi-override") => { return CSSKeywordKind.BidiOverride }
        comptime_fnv1_hash("isolate-override") => { return CSSKeywordKind.IsolateOverride }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundRepeatKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("repeat") => { return CSSKeywordKind.Repeat }
        comptime_fnv1_hash("repeat-x") => { return CSSKeywordKind.RepeatX }
        comptime_fnv1_hash("repeat-y") => { return CSSKeywordKind.RepeatY }
        comptime_fnv1_hash("no-repeat") => { return CSSKeywordKind.NoRepeat }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundAttachmentKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("scroll") => { return CSSKeywordKind.Scroll }
        comptime_fnv1_hash("fixed") => { return CSSKeywordKind.Fixed }
        comptime_fnv1_hash("local") => { return CSSKeywordKind.Local }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundClipKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("border-box") => { return CSSKeywordKind.BorderBox }
        comptime_fnv1_hash("padding-box") => { return CSSKeywordKind.PaddingBox }
        comptime_fnv1_hash("content-box") => { return CSSKeywordKind.ContentBox }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundOriginKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("border-box") => { return CSSKeywordKind.BorderBox }
        comptime_fnv1_hash("padding-box") => { return CSSKeywordKind.PaddingBox }
        comptime_fnv1_hash("content-box") => { return CSSKeywordKind.ContentBox }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundSizeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("cover") => { return CSSKeywordKind.Cover }
        comptime_fnv1_hash("contain") => { return CSSKeywordKind.Contain }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBackgroundBlendModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("multiply") => { return CSSKeywordKind.Multiply }
        comptime_fnv1_hash("screen") => { return CSSKeywordKind.Screen }
        comptime_fnv1_hash("overlay") => { return CSSKeywordKind.Overlay }
        comptime_fnv1_hash("darken") => { return CSSKeywordKind.Darken }
        comptime_fnv1_hash("lighten") => { return CSSKeywordKind.Lighten }
        comptime_fnv1_hash("color-dodge") => { return CSSKeywordKind.ColorDodge }
        comptime_fnv1_hash("color-burn") => { return CSSKeywordKind.ColorBurn }
        comptime_fnv1_hash("hard-light") => { return CSSKeywordKind.HardLight }
        comptime_fnv1_hash("soft-light") => { return CSSKeywordKind.SoftLight }
        comptime_fnv1_hash("difference") => { return CSSKeywordKind.Difference }
        comptime_fnv1_hash("exclusion") => { return CSSKeywordKind.Exclusion }
        comptime_fnv1_hash("hue") => { return CSSKeywordKind.Hue }
        comptime_fnv1_hash("saturation") => { return CSSKeywordKind.Saturation }
        comptime_fnv1_hash("color") => { return CSSKeywordKind.Color }
        comptime_fnv1_hash("luminosity") => { return CSSKeywordKind.Luminosity }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextDecorationLineKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("underline") => { return CSSKeywordKind.Underline }
        comptime_fnv1_hash("overline") => { return CSSKeywordKind.Overline }
        comptime_fnv1_hash("line-through") => { return CSSKeywordKind.LineThrough }
        comptime_fnv1_hash("blink") => { return CSSKeywordKind.Blink }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextDecorationStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("solid") => { return CSSKeywordKind.Solid }
        comptime_fnv1_hash("double") => { return CSSKeywordKind.Double }
        comptime_fnv1_hash("dotted") => { return CSSKeywordKind.Dotted }
        comptime_fnv1_hash("dashed") => { return CSSKeywordKind.Dashed }
        comptime_fnv1_hash("wavy") => { return CSSKeywordKind.Wavy }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextAlignLastKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("justify") => { return CSSKeywordKind.Justify }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextJustifyKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("inter-word") => { return CSSKeywordKind.InterWord }
        comptime_fnv1_hash("inter-ideograph") => { return CSSKeywordKind.InterIdeograph }
        comptime_fnv1_hash("distribute") => { return CSSKeywordKind.Distribute }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBoxSizingKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("content-box") => { return CSSKeywordKind.ContentBox }
        comptime_fnv1_hash("border-box") => { return CSSKeywordKind.BorderBox }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getPointerEventsKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBorderImageRepeatKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch }
        comptime_fnv1_hash("repeat") => { return CSSKeywordKind.Repeat }
        comptime_fnv1_hash("round") => { return CSSKeywordKind.Round }
        comptime_fnv1_hash("space") => { return CSSKeywordKind.Space }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBreakAfterKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid }
        comptime_fnv1_hash("always") => { return CSSKeywordKind.Always }
        comptime_fnv1_hash("all") => { return CSSKeywordKind.All }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("page") => { return CSSKeywordKind.Page }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBreakBeforeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid }
        comptime_fnv1_hash("always") => { return CSSKeywordKind.Always }
        comptime_fnv1_hash("all") => { return CSSKeywordKind.All }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("page") => { return CSSKeywordKind.Page }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBreakInsideKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("avoid") => { return CSSKeywordKind.Avoid }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getColumnRuleStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("hidden") => { return CSSKeywordKind.Hidden }
        comptime_fnv1_hash("dotted") => { return CSSKeywordKind.Dotted }
        comptime_fnv1_hash("dashed") => { return CSSKeywordKind.Dashed }
        comptime_fnv1_hash("solid") => { return CSSKeywordKind.Solid }
        comptime_fnv1_hash("double") => { return CSSKeywordKind.Double }
        comptime_fnv1_hash("groove") => { return CSSKeywordKind.Groove }
        comptime_fnv1_hash("ridge") => { return CSSKeywordKind.Ridge }
        comptime_fnv1_hash("inset") => { return CSSKeywordKind.Inset }
        comptime_fnv1_hash("outset") => { return CSSKeywordKind.Outset }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getColumnSpanKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("all") => { return CSSKeywordKind.All }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getColumnFillKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("balance") => { return CSSKeywordKind.Balance }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getBoxDecorationBreakKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("slice") => { return CSSKeywordKind.Slice }
        comptime_fnv1_hash("clone") => { return CSSKeywordKind.Clone }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskModeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("alpha") => { return CSSKeywordKind.Alpha }
        comptime_fnv1_hash("luminance") => { return CSSKeywordKind.Luminance }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskRepeatKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("repeat") => { return CSSKeywordKind.Repeat }
        comptime_fnv1_hash("repeat-x") => { return CSSKeywordKind.RepeatX }
        comptime_fnv1_hash("repeat-y") => { return CSSKeywordKind.RepeatY }
        comptime_fnv1_hash("no-repeat") => { return CSSKeywordKind.NoRepeat }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskClipKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("border-box") => { return CSSKeywordKind.BorderBox }
        comptime_fnv1_hash("padding-box") => { return CSSKeywordKind.PaddingBox }
        comptime_fnv1_hash("content-box") => { return CSSKeywordKind.ContentBox }
        comptime_fnv1_hash("text") => { return CSSKeywordKind.Text }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskCompositeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("add") => { return CSSKeywordKind.Add }
        comptime_fnv1_hash("subtract") => { return CSSKeywordKind.Subtract }
        comptime_fnv1_hash("intersect") => { return CSSKeywordKind.Intersect }
        comptime_fnv1_hash("exclude") => { return CSSKeywordKind.Exclude }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getMaskTypeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("luminance") => { return CSSKeywordKind.Luminance }
        comptime_fnv1_hash("alpha") => { return CSSKeywordKind.Alpha }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getScrollbarWidthKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("thin") => { return CSSKeywordKind.Thin }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTouchActionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("manipulation") => { return CSSKeywordKind.Manipulation }
        comptime_fnv1_hash("pan-x") => { return CSSKeywordKind.PanX }
        comptime_fnv1_hash("pan-y") => { return CSSKeywordKind.PanY }
        comptime_fnv1_hash("pinch-zoom") => { return CSSKeywordKind.PinchZoom }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getHyphensKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("manual") => { return CSSKeywordKind.Manual }
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getLineBreakKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("loose") => { return CSSKeywordKind.Loose }
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("strict") => { return CSSKeywordKind.Strict }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextEmphasisStyleKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("dot") => { return CSSKeywordKind.Dot }
        comptime_fnv1_hash("circle") => { return CSSKeywordKind.Circle }
        comptime_fnv1_hash("double-circle") => { return CSSKeywordKind.DoubleCircle }
        comptime_fnv1_hash("triangle") => { return CSSKeywordKind.Triangle }
        comptime_fnv1_hash("sesame") => { return CSSKeywordKind.Sesame }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextEmphasisPositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("under") => { return CSSKeywordKind.Under }
        comptime_fnv1_hash("over") => { return CSSKeywordKind.Over }
        comptime_fnv1_hash("right") => { return CSSKeywordKind.Right }
        comptime_fnv1_hash("left") => { return CSSKeywordKind.Left }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getTextOrientationKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("mixed") => { return CSSKeywordKind.Mixed }
        comptime_fnv1_hash("upright") => { return CSSKeywordKind.Upright }
        comptime_fnv1_hash("sideways") => { return CSSKeywordKind.Sideways }
        comptime_fnv1_hash("sideways-right") => { return CSSKeywordKind.SidewaysRight }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getRubyAlignKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("space-between") => { return CSSKeywordKind.SpaceBetween }
        comptime_fnv1_hash("space-around") => { return CSSKeywordKind.SpaceAround }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getRubyMergeKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("separate") => { return CSSKeywordKind.Separate }
        comptime_fnv1_hash("collapse") => { return CSSKeywordKind.Collapse }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getRubyPositionKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("over") => { return CSSKeywordKind.Over }
        comptime_fnv1_hash("under") => { return CSSKeywordKind.Under }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getJustifySelfKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("auto") => { return CSSKeywordKind.Auto }
        comptime_fnv1_hash("start") => { return CSSKeywordKind.Start }
        comptime_fnv1_hash("end") => { return CSSKeywordKind.End }
        comptime_fnv1_hash("center") => { return CSSKeywordKind.Center }
        comptime_fnv1_hash("stretch") => { return CSSKeywordKind.Stretch }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getContainKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("none") => { return CSSKeywordKind.None }
        comptime_fnv1_hash("strict") => { return CSSKeywordKind.Strict }
        comptime_fnv1_hash("content") => { return CSSKeywordKind.Content }
        comptime_fnv1_hash("layout") => { return CSSKeywordKind.Layout }
        comptime_fnv1_hash("style") => { return CSSKeywordKind.Style }
        comptime_fnv1_hash("paint") => { return CSSKeywordKind.Paint }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontWidthKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("normal") => { return CSSKeywordKind.Normal }
        comptime_fnv1_hash("ultra-condensed") => { return CSSKeywordKind.UltraCondensed }
        comptime_fnv1_hash("extra-condensed") => { return CSSKeywordKind.ExtraCondensed }
        comptime_fnv1_hash("condensed") => { return CSSKeywordKind.Condensed }
        comptime_fnv1_hash("semi-condensed") => { return CSSKeywordKind.SemiCondensed }
        comptime_fnv1_hash("semi-expanded") => { return CSSKeywordKind.SemiExpanded }
        comptime_fnv1_hash("expanded") => { return CSSKeywordKind.Expanded }
        comptime_fnv1_hash("extra-expanded") => { return CSSKeywordKind.ExtraExpanded }
        comptime_fnv1_hash("ultra-expanded") => { return CSSKeywordKind.UltraExpanded }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getSystemFamilyNameKeywordKind(hash : size_t) : CSSKeywordKind {
    switch(hash) {
        comptime_fnv1_hash("caption") => { return CSSKeywordKind.Caption }
        comptime_fnv1_hash("icon") => { return CSSKeywordKind.Icon }
        comptime_fnv1_hash("menu") => { return CSSKeywordKind.Menu }
        comptime_fnv1_hash("message-box") => { return CSSKeywordKind.MessageBox }
        comptime_fnv1_hash("small-caption") => { return CSSKeywordKind.SmallCaption }
        comptime_fnv1_hash("status-bar") => { return CSSKeywordKind.StatusBar }
        default => { return CSSKeywordKind.Unknown }
    }
}

func getFontStretchKeywordKind(hash : size_t) : CSSKeywordKind {
    return getFontWidthKeywordKind(hash)
}

func (parser : &mut Parser) not_id_val_err(prop : &std::string_view) {
    var errStr = std::string("unknown value for '")
    errStr.append_with_len(prop.data(), prop.size())
    errStr.append('\'')
    const n = std::string_view(", property requires an identifier value")
    errStr.append_with_len(n.data(), n.size())
    parser.error(std::string_view(errStr.data(), errStr.size()))
}

func (parser : &mut Parser) wrong_val_kw_err(prop : &std::string_view) {
    var errStr = std::string("unknown value for '")
    errStr.append_with_len(prop.data(), prop.size())
    errStr.append('\'');
    parser.error(std::string_view(errStr.data(), errStr.size()))
}

func (token : &mut Token) fnv1() : size_t {
    return fnv1_hash(token.value.data())
}

func (cssParser : &mut CSSParser) parseFontWeight(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const kind = getFontWeightKeywordKind(token.fnv1())
        if(kind == CSSKeywordKind.Unknown) {
            parser.wrong_val_kw_err("font-weight")
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_number(builder, value, token.value);
    } else {
        parser.wrong_val_kw_err("font-weight");
        return;
    }
}

func (cssParser : &mut CSSParser) parseFontSize(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const kind = getFontSizeKeywordKind(token.fnv1())
        if(kind == CSSKeywordKind.Unknown) {
            parser.wrong_val_kw_err("font-size");
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_length(parser, builder, value, token.value);
    } else {
        parser.wrong_val_kw_err("font-size");
        return;
    }
}

func (cssParser : &mut CSSParser) parseTextAlign(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-align");
        return;
    }
    const kind = getTextAlignKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-align");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseDisplay(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("display");
        return;
    }
    const kind = getDisplayKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("display");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parsePosition(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("position");
        return;
    }
    const kind = getPositionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("position");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseOverflow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overflow");
        return;
    }
    const kind = getOverflowKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overflow");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFloat(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("float");
        return;
    }
    const kind = getFloatKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("float");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseClear(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("clear");
        return;
    }
    const kind = getClearKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("clear");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseVerticalAlign(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("vertical-align");
        return;
    }
    const kind = getVerticalAlignKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("vertical-align");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseWhitespace(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("white-space");
        return;
    }
    const kind = getWhitespaceKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("white-space");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTextTransform(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-transform");
        return;
    }
    const kind = getTextTransformKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-transform");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseVisibility(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("visibility");
        return;
    }
    const kind = getVisibilityKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("visibility");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseCursor(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("cursor");
        return;
    }
    const kind = getCursorKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("cursor");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseDirection(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("direction");
        return;
    }
    const kind = getDirectionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("direction");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseResize(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("resize");
        return;
    }
    const kind = getResizeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("resize");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTableLayout(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("table-layout");
        return;
    }
    const kind = getTableLayoutKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("table-layout");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBorderCollapse(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("border-collapse");
        return;
    }
    const kind = getBorderCollapseKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("border-collapse");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTextOverflow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-overflow");
        return;
    }
    const kind = getTextOverflowKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-overflow");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseOverflowWrap(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overflow-wrap");
        return;
    }
    const kind = getOverflowWrapKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overflow-wrap");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseWordBreak(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("word-break");
        return;
    }
    const kind = getWordBreakKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("word-break");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseObjectFit(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("object-fit");
        return;
    }
    const kind = getObjectFitKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("object-fit");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseImageRendering(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("image-rendering");
        return;
    }
    const kind = getImageRenderingKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("image-rendering");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBackFaceVisibilityValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("backface-visibility");
        return;
    }
    const kind = getBackFaceVisibilityKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("backface-visibility");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFontStyleValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-style");
        return;
    }
    const kind = getFontStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-style");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFontVariantValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-variant");
        return;
    }
    const kind = getFontVariantKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-variant");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseListStyleType(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("list-style-type");
        return;
    }
    const kind = getListStyleTypeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("list-style-type");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseListStylePosition(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("list-style-position");
        return;
    }
    const kind = getListStylePositionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("list-style-position");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseAlignItems(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("align-items");
        return;
    }
    const kind = getAlignItemsKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("align-items");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseAlignContent(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("align-content");
        return;
    }
    const kind = getAlignContentKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("align-content");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseJustifyContent(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("justify-content");
        return;
    }
    const kind = getJustifyContentKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("align-content");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFlexDirection(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("flex-direction")
        return;
    }
    const kind = getFlexDirectionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("flex-direction")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseFlexWrap(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("flex-wrap")
        return;
    }
    const kind = getFlexWrapKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("flex-wrap")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseAlignSelf(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("align-self")
        return;
    }
    const kind = getAlignSelfKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("align-self")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseCaptionSide(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("caption-side")
        return;
    }
    const kind = getCaptionSideKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("caption-side")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseEmptyCells(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("empty-cells")
        return;
    }
    const kind = getEmptyCellsKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("empty-cells")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parsePageBreakInside(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("page-break-inside")
        return;
    }
    const kind = getPageBreakInsideKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("page-break-inside")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parsePageBreakBefore(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("page-break-before")
        return;
    }
    const kind = getPageBreakBeforeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("page-break-before")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parsePageBreakAfter(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("page-break-after")
        return;
    }
    const kind = getPageBreakAfterKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("page-break-after")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseIsolation(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("isolation")
        return;
    }
    const kind = getIsolationKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("isolation")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseMixBlendMode(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mix-blend-mode")
        return;
    }
    const kind = getMixBlendModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mix-blend-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseUserSelect(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("user-select")
        return;
    }
    const kind = getUserSelectKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("user-select")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseScrollBehavior(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("scroll-behavior")
        return;
    }
    const kind = getScrollBehaviorKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("scroll-behavior")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseScrollSnapAlign(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("scroll-snap-align")
        return;
    }
    const kind = getScrollSnapAlignKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("scroll-snap-align")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseJustifyItems(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("justify-items")
        return;
    }
    const kind = getJustifyItemsKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("justify-items")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseWritingMode(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("writing-mode")
        return;
    }
    const kind = getWritingModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("writing-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseAnimationDirection(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("animation-direction")
        return;
    }
    const kind = getAnimationDirectionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("animation-direction")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseAnimationFillMode(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("animation-fill-mode")
        return;
    }
    const kind = getAnimationFillModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("animation-fill-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseAnimationPlayState(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("animation-play-state")
        return;
    }
    const kind = getAnimationPlayStateKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("animation-play-state")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseAppearance(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("appearance")
        return;
    }
    const kind = getAppearanceKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("appearance")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseOverflowAnchor(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overflow-anchor")
        return;
    }
    const kind = getOverflowAnchorKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overflow-anchor")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseScrollSnapStop(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("scroll-snap-stop")
        return;
    }
    const kind = getScrollSnapStopKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("scroll-snap-stop")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseClipRule(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("clip-rule")
        return;
    }
    const kind = getClipRuleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("clip-rule")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseShapeRendering(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("shape-rendering")
        return;
    }
    const kind = getShapeRenderingKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("shape-rendering")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseTextRendering(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-rendering")
        return;
    }
    const kind = getTextRenderingKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-rendering")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseTransformStyle(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("transform-style")
        return;
    }
    const kind = getTransformStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("transform-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}
func (cssParser : &mut CSSParser) parseUnicodeBidi(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("unicode-bidi")
        return;
    }
    const kind = getUnicodeBidiKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("unicode-bidi")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBackgroundRepeat(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-repeat")
        return;
    }
    const kind = getBackgroundRepeatKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-repeat")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBackgroundAttachment(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-attachment")
        return;
    }
    const kind = getBackgroundAttachmentKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-attachment")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBackgroundClip(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-clip")
        return;
    }
    const kind = getBackgroundClipKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-clip")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBackgroundOrigin(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-origin")
        return;
    }
    const kind = getBackgroundOriginKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-origin")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBackgroundSize(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-size")
        return;
    }
    const kind = getBackgroundSizeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-size")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBackgroundBlendMode(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("background-blend-mode")
        return;
    }
    const kind = getBackgroundBlendModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("background-blend-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextDecorationLine(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-decoration-line")
        return;
    }
    const kind = getTextDecorationLineKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-decoration-line")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextDecorationStyle(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-decoration-style")
        return;
    }
    const kind = getTextDecorationStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-decoration-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextAlignLast(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-align-last")
        return;
    }
    const kind = getTextAlignLastKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-align-last")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextJustify(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-justify")
        return;
    }
    const kind = getTextJustifyKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-justify")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBoxSizing(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("box-sizing")
        return;
    }
    const kind = getBoxSizingKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("box-sizing")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parsePointerEvents(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("pointer-events")
        return;
    }
    const kind = getPointerEventsKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("pointer-events")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseGridAutoFlow(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("grid-auto-flow")
        return;
    }
    const rowHash = comptime_fnv1_hash("row")
    const columnHash = comptime_fnv1_hash("column")
    const denseHash = comptime_fnv1_hash("dense")
    const firstHash = fnv1_hash(token.value.data())
    var kind = CSSKeywordKind.Unknown
    if(firstHash == rowHash) {
        kind = CSSKeywordKind.Row
    } else if(firstHash == columnHash) {
        kind = CSSKeywordKind.Column
    } else if(token.value.equals(std::string_view("dense"))) {
        kind = CSSKeywordKind.Dense
    } else {
        parser.wrong_val_kw_err("grid-auto-flow");
    }
    parser.increment();
    const next = parser.getToken()
    if(kind != CSSKeywordKind.Dense && next.type == TokenType.Identifier && next.value.equals(std::string_view("dense"))) {
        if(kind == CSSKeywordKind.Row) {
            parser.increment();
            alloc_two_value_keywords(builder, value, kind, CSSKeywordKind.Dense, token.value, next.value);
            return;
        } else if(kind == CSSKeywordKind.Column) {
            parser.increment();
            alloc_two_value_keywords(builder, value, kind, CSSKeywordKind.Dense, token.value, next.value);
            return;
        } else {
            parser.wrong_val_kw_err("grid-auto-flow")
        }
    }
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBorderImageRepeat(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("border-image-repeat")
        return;
    }
    const kind = getBorderImageRepeatKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("border-image-repeat")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBreakAfter(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("break-after")
        return;
    }
    const kind = getBreakAfterKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("break-after")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBreakBefore(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("break-before")
        return;
    }
    const kind = getBreakBeforeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("break-before")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBreakInside(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("break-inside")
        return;
    }
    const kind = getBreakInsideKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("break-inside")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseColumnRuleStyle(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("column-rule-style")
        return;
    }
    const kind = getColumnRuleStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("column-rule-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseColumnSpan(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("column-span")
        return;
    }
    const kind = getColumnSpanKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("column-span")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseColumnFill(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("column-fill")
        return;
    }
    const kind = getColumnFillKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("column-fill")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseBoxDecorationBreak(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("box-decoration-break")
        return;
    }
    const kind = getBoxDecorationBreakKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("box-decoration-break")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskMode(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-mode")
        return;
    }
    const kind = getMaskModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskRepeat(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-repeat")
        return;
    }
    const kind = getMaskRepeatKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-repeat")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskClip(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-clip")
        return;
    }
    const kind = getMaskClipKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-clip")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskComposite(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-composite")
        return;
    }
    const kind = getMaskCompositeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-composite")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskType(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-type")
        return;
    }
    const kind = getMaskTypeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-type")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseScrollbarWidth(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("scrollbar-width")
        return;
    }
    const kind = getScrollbarWidthKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("scrollbar-width")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTouchAction(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("touch-action")
        return;
    }
    const kind = getTouchActionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("touch-action")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseHyphens(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("hyphens")
        return;
    }
    const kind = getHyphensKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("hyphens")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseLineBreak(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("line-break")
        return;
    }
    const kind = getLineBreakKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("line-break")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextEmphasisStyle(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-emphasis-style")
        return;
    }
    const kind = getTextEmphasisStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-emphasis-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextEmphasisPosition(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-emphasis-position")
        return;
    }
    const kind = getTextEmphasisPositionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-emphasis-position")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextOrientation(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-orientation")
        return;
    }
    const kind = getTextOrientationKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-orientation")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseRubyAlign(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("ruby-align")
        return;
    }
    const kind = getRubyAlignKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("ruby-align")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseRubyMerge(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("ruby-merge")
        return;
    }
    const kind = getRubyMergeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("ruby-merge")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseRubyPosition(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("ruby-position")
        return;
    }
    const kind = getRubyPositionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("ruby-position")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseJustifySelf(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("justify-self")
        return;
    }
    const kind = getJustifySelfKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("justify-self")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseContain(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("contain")
        return;
    }
    const kind = getContainKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("contain")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseOverscrollBehavior(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overscroll-behavior")
        return;
    }
    const kind = getOverscrollBehaviorKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overscroll-behavior")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parsePageOrientation(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("page-orientation")
        return;
    }
    const kind = getPageOrientationKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("page-orientation")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextCombineUpright(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-combine-upright")
        return;
    }
    const kind = getTextCombineUprightKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-combine-upright")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseFontKerning(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-kerning")
        return;
    }
    const kind = getFontKerningKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-kerning")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseOutlineStyle(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("outline-style")
        return;
    }
    const kind = getOutlineStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("outline-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTransformBox(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("transform-box")
        return;
    }
    const kind = getTransformBoxKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("transform-box")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseFontVariantCaps(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-variant-caps")
        return;
    }
    const kind = getFontVariantCapsKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-variant-caps")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseFontVariantNumeric(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-variant-numeric")
        return;
    }
    const kind = getFontVariantNumericKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-variant-numeric")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseFontVariantEastAsian(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-variant-east-asian")
        return;
    }
    const kind = getFontVariantEastAsianKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-variant-east-asian")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseImageOrientation(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("image-orientation")
        return;
    }
    const kind = getImageOrientationKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("image-orientation")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTransitionTimingFunction(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("transition-timing-function")
        return;
    }
    const kind = getTransitionTimingFunctionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("transition-timing-function")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseVectorEffect(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("vector-effect")
        return;
    }
    const kind = getVectorEffectKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("vector-effect")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseForcedColorAdjust(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("forced-color-adjust")
        return;
    }
    const kind = getForcedColorAdjustKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("forced-color-adjust")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseColorScheme(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("color-scheme")
        return;
    }
    const kind = getColorSchemeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("color-scheme")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parsePrintColorAdjust(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("print-color-adjust")
        return;
    }
    const kind = getPrintColorAdjustKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("print-color-adjust")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBorderStyle(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("border-style")
        return;
    }
    const kind = getBorderStyleKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("border-style")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseAnimationTimingFunction(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("animation-timing-function")
        return;
    }
    const kind = getAnimationTimingFunctionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("animation-timing-function")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskBorderMode(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-border-mode")
        return;
    }
    const kind = getMaskBorderModeKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-border-mode")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseMaskBorderRepeat(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("mask-border-repeat")
        return;
    }
    const kind = getMaskBorderRepeatKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("mask-border-repeat")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextDecorationSkipInk(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-decoration-skip-ink")
        return;
    }
    const kind = getTextDecorationSkipInkKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-decoration-skip-ink")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseTextUnderlinePosition(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-underline-position")
        return;
    }
    const kind = getTextUnderlinePositionKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-underline-position")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}


func (cssParser : &mut CSSParser) parseFontOpticalSizing(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-optical-sizing")
        return;
    }
    const kind = getFontOpticalSizingKeywordKind(token.fnv1())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-optical-sizing")
    }
    parser.increment()
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseScrollbarGutter(
        parser : *mut Parser,
        builder : *mut ASTBuilder,
        value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("scrollbar-gutter")
        return;
    }
    if(token.value.equals(std::string_view("auto"))) {
        parser.increment();
        alloc_value_keyword(builder, value, CSSKeywordKind.Auto, token.value)
    } else if(token.value.equals(std::string_view("stable"))) {
        parser.increment();
        const next = parser.getToken();
        if(next.value.equals(std::string_view("both-edges"))) {
            parser.increment();
            alloc_two_value_keywords(builder, value, CSSKeywordKind.Stable, CSSKeywordKind.BothEdges, token.value, next.value);
            return;
        }
        alloc_value_keyword(builder, value, CSSKeywordKind.Auto, token.value)
    } else {
        parser.wrong_val_kw_err("scrollbar-gutter")
    }
}