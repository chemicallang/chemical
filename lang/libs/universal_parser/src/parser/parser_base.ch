public type JsStyleBlockFn = (parser : *mut Parser, builder : *mut ASTBuilder, dyn_values : *mut std::vector<*mut Value>) => *mut JsNode

public struct JsParser {
    var dyn_values : *mut std::vector<*mut Value>
    var components : *mut std::vector<*mut JsJSXElement>
    // When false (plain-JS mode), parenthesized expressions are unwrapped
    // instead of producing a JsParen node.
    var jsx_enabled : bool = true
    // Set by universal_cbi: parses a `style { … }` CSS block. Lives outside this
    // package so universal_parser need not depend on css_parser.
    var style_fn : JsStyleBlockFn = null
}
