
struct HtmlRoot {

    var children : std::vector<*mut HtmlChild>

    var parent : *mut ASTNode

    var support : SymResSupport

}