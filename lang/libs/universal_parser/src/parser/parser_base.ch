public type JsStyleBlockFn = (parser : *mut Parser, builder : *mut ASTBuilder, dyn_values : *mut std::vector<*mut Value>) => *mut JsNode

public struct JsParser {
    var dyn_values : *mut std::vector<*mut Value>
    var components : *mut std::vector<*mut JsJSXElement>
    // When false, the lexer is plain JavaScript (`<` is comparison/shift,
    // `state` is an identifier, no `??`/`?.`/`**`). Grouping parentheses are
    // preserved as `JsParen` nodes in both modes.
    var jsx_enabled : bool = true
    // Set by universal_cbi: parses a `style { … }` CSS block. Lives outside this
    // package so universal_parser need not depend on css_parser.
    var style_fn : JsStyleBlockFn = null
}
