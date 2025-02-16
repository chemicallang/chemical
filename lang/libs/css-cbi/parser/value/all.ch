import "/parser/value/margin.ch"

func (map : &mut std::unordered_map<std::string_view, void*>) put(view : &std::string_view, val : void*) {
    var f : void*
    if(map.find(view, f)) {
        printf("INSERTED %s ALREADY CONTAINS PARSER\n", view.data());
        fflush(null)
    }
    map.insert(view, val)
}

func putAllCSSValueParsers(
    map : &mut std::unordered_map<std::string_view, void*>
) {

    map.put(std::string_view("margin"), CSSParser::parseMargin)
    map.put(std::string_view("margin-left"), CSSParser::parseMarginSingle)
    map.put(std::string_view("margin-right"), CSSParser::parseMarginSingle)
    map.put(std::string_view("margin-top"), CSSParser::parseMarginSingle)
    map.put(std::string_view("margin-bottom"), CSSParser::parseMarginSingle)
    map.put(std::string_view("border"), CSSParser::parseBorder)
    map.put(std::string_view("border-radius"), CSSParser::parseBorderRadius)
    map.put(std::string_view("padding"), CSSParser::parsePadding)
    map.put(std::string_view("width"), CSSParser::parseWidth)
    map.put(std::string_view("height"), CSSParser::parseHeight)
    map.put(std::string_view("font"), CSSParser::parseFont)
    map.put(std::string_view("font-weight"), CSSParser::parseFontWeight)
    map.put(std::string_view("font-size"), CSSParser::parseFontSize)
    map.put(std::string_view("text-align"), CSSParser::parseTextAlign)
    map.put(std::string_view("display"), CSSParser::parseDisplay)
    map.put(std::string_view("position"), CSSParser::parsePosition)
    map.put(std::string_view("overflow"), CSSParser::parseOverflow)
    map.put(std::string_view("z-index"), CSSParser::parseZIndex)
    map.put(std::string_view("float"), CSSParser::parseFloat)
    map.put(std::string_view("clear"), CSSParser::parseClear)
    map.put(std::string_view("vertical-align"), CSSParser::parseVerticalAlign)
    map.put(std::string_view("white-space"), CSSParser::parseWhitespace)
    map.put(std::string_view("text-transform"), CSSParser::parseTextTransform)
    map.put(std::string_view("visibility"), CSSParser::parseVisibility)
    map.put(std::string_view("cursor"), CSSParser::parseCursor)
    map.put(std::string_view("direction"), CSSParser::parseDirection)
    map.put(std::string_view("resize"), CSSParser::parseResize)
    map.put(std::string_view("table-layout"), CSSParser::parseTableLayout)
    map.put(std::string_view("border-collapse"), CSSParser::parseBorderCollapse)
    map.put(std::string_view("text-overflow"), CSSParser::parseTextOverflow)
    map.put(std::string_view("overflow-wrap"), CSSParser::parseOverflowWrap)
    map.put(std::string_view("word-break"), CSSParser::parseWordBreak)
    map.put(std::string_view("object-fit"), CSSParser::parseObjectFit)
    map.put(std::string_view("image-rendering"), CSSParser::parseImageRendering)
    map.put(std::string_view("backface-visibility"), CSSParser::parseBackFaceVisibilityValue)
    map.put(std::string_view("font-style"), CSSParser::parseFontStyleValue)
    map.put(std::string_view("font-variant"), CSSParser::parseFontVariantValue)
    map.put(std::string_view("list-style-type"), CSSParser::parseListStyleType)
    map.put(std::string_view("list-style-position"), CSSParser::parseListStylePosition)
    map.put(std::string_view("align-items"), CSSParser::parseAlignItems)
    map.put(std::string_view("align-content"), CSSParser::parseAlignContent)
    map.put(std::string_view("justify-content"), CSSParser::parseJustifyContent)
    map.put(std::string_view("flex-direction"), CSSParser::parseFlexDirection)
    map.put(std::string_view("flex-wrap"), CSSParser::parseFlexWrap)
    map.put(std::string_view("align-self"), CSSParser::parseAlignSelf)
    map.put(std::string_view("caption-side"), CSSParser::parseCaptionSide)
    map.put(std::string_view("empty-cells"), CSSParser::parseEmptyCells)
    map.put(std::string_view("page-break-inside"), CSSParser::parsePageBreakInside)
    map.put(std::string_view("page-break-before"), CSSParser::parsePageBreakBefore)
    map.put(std::string_view("page-break-after"), CSSParser::parsePageBreakAfter)
    map.put(std::string_view("isolation"), CSSParser::parseIsolation)
    map.put(std::string_view("mix-blend-mode"), CSSParser::parseMixBlendMode)
    map.put(std::string_view("user-select"), CSSParser::parseUserSelect)
    map.put(std::string_view("scroll-behavior"), CSSParser::parseScrollBehavior)
    map.put(std::string_view("scroll-snap-align"), CSSParser::parseScrollSnapAlign)
    map.put(std::string_view("justify-items"), CSSParser::parseJustifyItems)
    map.put(std::string_view("writing-mode"), CSSParser::parseWritingMode)
    map.put(std::string_view("animation-direction"), CSSParser::parseAnimationDirection)
    map.put(std::string_view("animation-fill-mode"), CSSParser::parseAnimationFillMode)
    map.put(std::string_view("animation-play-state"), CSSParser::parseAnimationPlayState)
    map.put(std::string_view("appearance"), CSSParser::parseAppearance)
    map.put(std::string_view("overflow-anchor"), CSSParser::parseOverflowAnchor)
    map.put(std::string_view("scroll-snap-stop"), CSSParser::parseScrollSnapStop)
    map.put(std::string_view("clip-rule"), CSSParser::parseClipRule)
    map.put(std::string_view("shape-rendering"), CSSParser::parseShapeRendering)
    map.put(std::string_view("text-rendering"), CSSParser::parseTextRendering)
    map.put(std::string_view("transform-style"), CSSParser::parseTransformStyle)
    map.put(std::string_view("unicode-bidi"), CSSParser::parseUnicodeBidi)
    map.put(std::string_view("background-repeat"), CSSParser::parseBackgroundRepeat)
    map.put(std::string_view("background-attachment"), CSSParser::parseBackgroundAttachment)
    map.put(std::string_view("background-clip"), CSSParser::parseBackgroundClip)
    map.put(std::string_view("background-origin"), CSSParser::parseBackgroundOrigin)
    map.put(std::string_view("background-size"), CSSParser::parseBackgroundSize)
    map.put(std::string_view("background-blend-mode"), CSSParser::parseBackgroundBlendMode)
    map.put(std::string_view("text-decoration-line"), CSSParser::parseTextDecorationLine)
    map.put(std::string_view("text-decoration-style"), CSSParser::parseTextDecorationStyle)
    map.put(std::string_view("text-align-last"), CSSParser::parseTextAlignLast)
    map.put(std::string_view("text-justify"), CSSParser::parseTextJustify)
    map.put(std::string_view("box-sizing"), CSSParser::parseBoxSizing)
    map.put(std::string_view("overflow-x"), CSSParser::parseOverflow)
    map.put(std::string_view("overflow-y"), CSSParser::parseOverflow)
    map.put(std::string_view("pointer-events"), CSSParser::parsePointerEvents)
    map.put(std::string_view("grid-auto-flow"), CSSParser::parseGridAutoFlow)
    map.put(std::string_view("border-image-repeat"), CSSParser::parseBorderImageRepeat)
    map.put(std::string_view("break-after"), CSSParser::parseBreakAfter)
    map.put(std::string_view("break-before"), CSSParser::parseBreakBefore)
    map.put(std::string_view("break-inside"), CSSParser::parseBreakInside)
    map.put(std::string_view("column-rule-style"), CSSParser::parseColumnRuleStyle)
    map.put(std::string_view("column-span"), CSSParser::parseColumnSpan)
    map.put(std::string_view("column-fill"), CSSParser::parseColumnFill)
    map.put(std::string_view("box-decoration-break"), CSSParser::parseBoxDecorationBreak)
    map.put(std::string_view("mask-mode"), CSSParser::parseMaskMode)
    map.put(std::string_view("mask-repeat"), CSSParser::parseMaskRepeat)
    map.put(std::string_view("mask-clip"), CSSParser::parseMaskClip)
    map.put(std::string_view("mask-composite"), CSSParser::parseMaskComposite)
    map.put(std::string_view("mask-type"), CSSParser::parseMaskType)
    map.put(std::string_view("scrollbar-width"), CSSParser::parseScrollbarWidth)
    map.put(std::string_view("touch-action"), CSSParser::parseTouchAction)
    map.put(std::string_view("hyphens"), CSSParser::parseHyphens)
    map.put(std::string_view("line-break"), CSSParser::parseLineBreak)
    map.put(std::string_view("text-emphasis-style"), CSSParser::parseTextEmphasisStyle)
    map.put(std::string_view("text-emphasis-position"), CSSParser::parseTextEmphasisPosition)
    map.put(std::string_view("text-orientation"), CSSParser::parseTextOrientation)
    map.put(std::string_view("ruby-align"), CSSParser::parseRubyAlign)
    map.put(std::string_view("ruby-merge"), CSSParser::parseRubyMerge)
    map.put(std::string_view("ruby-position"), CSSParser::parseRubyPosition)
    map.put(std::string_view("justify-self"), CSSParser::parseJustifySelf)
    map.put(std::string_view("contain"), CSSParser::parseContain)
    map.put(std::string_view("overscroll-behavior"), CSSParser::parseOverscrollBehavior)
    map.put(std::string_view("overscroll-behavior-x"), CSSParser::parseOverscrollBehavior)
    map.put(std::string_view("overscroll-behavior-y"), CSSParser::parseOverscrollBehavior)
    map.put(std::string_view("page-orientation"), CSSParser::parsePageOrientation)
    map.put(std::string_view("text-combine-upright"), CSSParser::parseTextCombineUpright)
    map.put(std::string_view("font-kerning"), CSSParser::parseFontKerning)
    map.put(std::string_view("outline-style"), CSSParser::parseOutlineStyle)
    map.put(std::string_view("transform-box"), CSSParser::parseTransformBox)
    map.put(std::string_view("font-variant-caps"), CSSParser::parseFontVariantCaps)
    map.put(std::string_view("font-variant-numeric"), CSSParser::parseFontVariantNumeric)
    map.put(std::string_view("font-variant-east-asian"), CSSParser::parseFontVariantEastAsian)
    map.put(std::string_view("image-orientation"), CSSParser::parseImageOrientation)
    map.put(std::string_view("transition-timing-function"), CSSParser::parseTransitionTimingFunction)
    map.put(std::string_view("vector-effect"), CSSParser::parseVectorEffect)
    map.put(std::string_view("forced-color-adjust"), CSSParser::parseForcedColorAdjust)
    map.put(std::string_view("color-scheme"), CSSParser::parseColorScheme)
    map.put(std::string_view("print-color-adjust"), CSSParser::parsePrintColorAdjust)
    map.put(std::string_view("border-style"), CSSParser::parseBorderStyle)
    map.put(std::string_view("animation-timing-function"), CSSParser::parseAnimationTimingFunction)
    map.put(std::string_view("mask-border-mode"), CSSParser::parseMaskBorderMode)
    map.put(std::string_view("mask-border-repeat"), CSSParser::parseMaskBorderRepeat)
    map.put(std::string_view("text-decoration-skip-ink"), CSSParser::parseTextDecorationSkipInk)
    map.put(std::string_view("text-underline-position"), CSSParser::parseTextUnderlinePosition)
    map.put(std::string_view("font-optical-sizing"), CSSParser::parseFontOpticalSizing)
    map.put(std::string_view("scrollbar-gutter"), CSSParser::parseScrollbarGutter)


}