import "utils/NamedColorMap.ch"

struct CSSParser {

    var colorMap : NamedColorMap

    // if NOT has dynamic values, we will automatically put
    // class name that is hashed and prefixed with 'h'
    var has_dynamic_values : bool = false

    @make
    func make() {

    }

    func isColor(&self, color : &std::string_view) : bool {
        return colorMap.isColor(color);
    }

}