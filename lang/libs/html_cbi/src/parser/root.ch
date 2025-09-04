
func parseHtmlRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *HtmlRoot {
    var root = builder.allocate<HtmlRoot>()
    new (root) HtmlRoot {
        children : std::vector<*mut HtmlChild>(),
        parent : parser.getParentNode(),
        support : SymResSupport {}
    }
    while(true) {
        var child = parseElementChild(parser, builder);
        if(child != null) {
            root.children.push(child)
        } else {
            break;
        }
    }
    return root;
}