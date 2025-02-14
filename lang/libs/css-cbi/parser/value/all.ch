import "/parser/value/margin.ch"

func putAllCSSValueParsers(
    map : &mut std::unordered_map<std::string_view, void*>
) {
    map.insert(std::string_view("margin"), CSSParser::parseMargin)
    map.insert(std::string_view("width"), CSSParser::parseWidth)
    map.insert(std::string_view("height"), CSSParser::parseHeight)
    map.insert(std::string_view("font-weight"), CSSParser::parseFontWeight)
    map.insert(std::string_view("text-align"), CSSParser::parseTextAlign)
    map.insert(std::string_view("display"), CSSParser::parseDisplay)
    map.insert(std::string_view("position"), CSSParser::parsePosition)
    map.insert(std::string_view("overflow"), CSSParser::parseOverflow)
}