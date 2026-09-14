// Compatibility entry point for plain-JS root parsing (`#js` macro bodies and
// the runtime `js` package). Delegates to the canonical parser in
// universal_parser; the JSX branches never run because the plain-JS lexer does
// not emit JSX tokens.
public func parseJsRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *JsRoot {
    var root = builder.allocate<JsRoot>()
    new (root) JsRoot {
        statements : std::vector<*mut JsNode>(),
        parent : parser.getParentNode(),
        support : SymResSupport {},
        dyn_values : std::vector<*mut Value>()
    }

    var components = std::vector<*mut JsJSXElement>()
    var jsParser = JsParser {
        dyn_values : &raw mut root.dyn_values,
        components : &raw mut components,
        jsx_enabled : false
    }

    while(true) {
        const token = parser.getToken();
        if(token.type == JsTokenType.RBrace as int || token.type == JsTokenType.EndOfFile as int) {
            break;
        }
        var stmt = jsParser.parseStatement(parser, builder);
        if(stmt != null) {
            root.statements.push(stmt);
        } else {
            break;
        }
    }
    return root;
}
