public struct JsParser {
    var dyn_values : *mut std::vector<*mut Value>
    var components : *mut std::vector<*mut JsJSXElement>
    // When false (plain-JS mode), parenthesized expressions are unwrapped
    // instead of producing a JsParen node.
    var jsx_enabled : bool = true
}
