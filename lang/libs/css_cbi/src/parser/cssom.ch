
func parseCSSOM(parser : *mut Parser, builder : *mut ASTBuilder) : *CSSOM {
    var root = builder.allocate<CSSOM>()
    new (root) CSSOM {
        parent : parser.getParentNode(),
        declarations : std::vector<*mut CSSDeclaration>(),
        dyn_values : std::vector<*mut Value>(),
        className : std::string_view(),
        global : GlobalBlock(),
        support : SymResSupport()
    }
    var cssParser = CSSParser();
    while(true) {
        var decl = cssParser.parseDeclaration(parser, builder);
        if(decl) {
            root.declarations.push(decl)
        } else {
            break;
        }
    }
    root.dyn_values = std::replace(cssParser.dyn_values, std::vector<*mut Value>());
    cssParser.parseAtRule(*root, parser, builder)
    return root;
}