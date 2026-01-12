struct HtmlParser {

    var dyn_values : *mut std::vector<*mut Value>

    var dyn_nodes : *mut std::vector<*mut ASTNode>

    var components : *mut std::vector<*mut HtmlElement>

}