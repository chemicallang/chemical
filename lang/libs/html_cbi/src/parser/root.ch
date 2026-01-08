
func parseHtmlRoot(parser : *mut Parser, builder : *mut ASTBuilder) : *HtmlRoot {
    var root = builder.allocate<HtmlRoot>()
    new (root) HtmlRoot {
        children : std::vector<*mut HtmlChild>(),
        parent : parser.getParentNode(),
        support : SymResSupport {},
        dyn_values : std::vector<*mut Value>(),
        dyn_nodes : std::vector<*mut ASTNode>(),
    }
    var htmlParser = HtmlParser {
        dyn_values : &mut root.dyn_values,
        dyn_nodes : &mut root.dyn_nodes
    }
    while(true) {
        var child = htmlParser.parseElementChild(parser, builder);
        if(child != null) {
            root.children.push(child)
        } else {
            break;
        }
    }
    return root;
}