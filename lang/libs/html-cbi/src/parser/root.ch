
func parseHtmlRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *HtmlRoot {
    var rootElement = parseElement(parser, builder);
    if(rootElement == null) {
        parser.error("expected a root element for #html");
        return null;
    } else {
        var root = builder.allocate<HtmlRoot>()
        new (root) HtmlRoot {
            element : rootElement,
            parent : parser.getParentNode()
        }
        return root;
    }
}