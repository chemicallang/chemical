import "/parser/value/margin.ch"

func putAllCSSValueParsers(
    map : &mut std::unordered_map<std::string_view, void*>
) {
    map.insert(std::string_view("margin"), CSSParser::parseMargin)
}