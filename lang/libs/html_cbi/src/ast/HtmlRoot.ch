
struct HtmlRoot {

    var children : std::vector<*mut HtmlChild>

    var parent : *mut ASTNode

    var support : SymResSupport

    var dyn_values : std::vector<*mut Value>

}