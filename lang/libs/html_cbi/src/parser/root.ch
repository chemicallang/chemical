
func parseHtmlRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *HtmlRoot {
    var rootElement = parseElement(parser, builder);
    if(rootElement == null) {
        parser.error("expected a root element for #html");
    }
    var root = builder.allocate<HtmlRoot>()
    new (root) HtmlRoot {
        element : rootElement,
        parent : parser.getParentNode(),
        support : SymResSupport {}
    }
    return root;
}