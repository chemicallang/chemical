/**
 * Runtime parse entry for the shared universal_parser.
 *
 * Parses a stream of JS/JSX statements (like a component body) until the end
 * of the token stream, returning a JsBlock. Used by the runtime `universal`
 * package; the CBI plugin keeps its own component-declaration entry in
 * react/macro.ch.
 */
public func parseUniversalRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *mut JsBlock {
    var dyn_values = std::vector<*mut Value>()
    var components = std::vector<*mut JsJSXElement>()

    var jsParser = JsParser {
        dyn_values : &raw mut dyn_values,
        components : &raw mut components
    }

    var block = builder.allocate<JsBlock>()
    new (block) JsBlock {
        base : JsNode { kind : JsNodeKind.Block },
        statements : std::vector<*mut JsNode>()
    }

    while(true) {
        const token = parser.getToken()
        if(token.type == JsTokenType.EndOfFile as int) {
            break
        }
        var stmt = jsParser.parseStatement(parser, builder)
        if(stmt != null) {
            block.statements.push(stmt)
        } else {
            break
        }
    }
    return block
}
