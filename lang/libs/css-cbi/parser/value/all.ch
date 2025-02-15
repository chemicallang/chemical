import "/parser/value/margin.ch"

func putAllCSSValueParsers(
    map : &mut std::unordered_map<std::string_view, void*>
) {
    map.insert(std::string_view("margin"), CSSParser::parseMargin)
    map.insert(std::string_view("margin-left"), CSSParser::parseMarginSingle)
    map.insert(std::string_view("margin-right"), CSSParser::parseMarginSingle)
    map.insert(std::string_view("margin-top"), CSSParser::parseMarginSingle)
    map.insert(std::string_view("margin-bottom"), CSSParser::parseMarginSingle)
    map.insert(std::string_view("padding"), CSSParser::parsePadding)
    map.insert(std::string_view("width"), CSSParser::parseWidth)
    map.insert(std::string_view("height"), CSSParser::parseHeight)
    map.insert(std::string_view("font-weight"), CSSParser::parseFontWeight)
    map.insert(std::string_view("font-size"), CSSParser::parseFontSize)
    map.insert(std::string_view("text-align"), CSSParser::parseTextAlign)
    map.insert(std::string_view("display"), CSSParser::parseDisplay)
    map.insert(std::string_view("position"), CSSParser::parsePosition)
    map.insert(std::string_view("overflow"), CSSParser::parseOverflow)
    map.insert(std::string_view("z-index"), CSSParser::parseZIndex)
    map.insert(std::string_view("float"), CSSParser::parseFloat)
    map.insert(std::string_view("clear"), CSSParser::parseClear)
    map.insert(std::string_view("vertical-align"), CSSParser::parseVerticalAlign)
    map.insert(std::string_view("white-space"), CSSParser::parseWhitespace)
    map.insert(std::string_view("text-transform"), CSSParser::parseTextTransform)
    map.insert(std::string_view("visibility"), CSSParser::parseVisibility)
    map.insert(std::string_view("cursor"), CSSParser::parseCursor)
    map.insert(std::string_view("direction"), CSSParser::parseDirection)
    map.insert(std::string_view("resize"), CSSParser::parseResize)
    map.insert(std::string_view("table-layout"), CSSParser::parseTableLayout)
    map.insert(std::string_view("border-collapse"), CSSParser::parseBorderCollapse)
    map.insert(std::string_view("text-overflow"), CSSParser::parseTextOverflow)
    map.insert(std::string_view("overflow-wrap"), CSSParser::parseOverflowWrap)
    map.insert(std::string_view("word-break"), CSSParser::parseWordBreak)
    map.insert(std::string_view("object-fit"), CSSParser::parseObjectFit)
    map.insert(std::string_view("image-rendering"), CSSParser::parseImageRendering)
    map.insert(std::string_view("backface-visibility"), CSSParser::parseBackFaceVisibilityValue)
    map.insert(std::string_view("font-style"), CSSParser::parseFontStyleValue)
    map.insert(std::string_view("font-variant"), CSSParser::parseFontVariantValue)
    map.insert(std::string_view("list-style-type"), CSSParser::parseListStyleType)
    map.insert(std::string_view("list-style-position"), CSSParser::parseListStylePosition)
    map.insert(std::string_view("align-items"), CSSParser::parseAlignItems)
    map.insert(std::string_view("align-content"), CSSParser::parseAlignContent)
    map.insert(std::string_view("justify-content"), CSSParser::parseJustifyContent)
}