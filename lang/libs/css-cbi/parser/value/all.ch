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
}