
func putAllCSSValueParsers(
    map : &mut std::unordered_map<std::string_view, void*>
);

// in css different properties can support different values
// parsing these different values, requires different functions
// we index functions based on property name as key to call them
struct ValueParserMap {

    var map : std::unordered_map<std::string_view, void*>

    @make
    func make() {
        putAllCSSValueParsers(map)
    }

    func getParserFor(&self, name : &std::string_view) : void* {
        var value : void*
        if(map.find(name, value)) {
            return value;
        } else {
            return null
        }
    }

}