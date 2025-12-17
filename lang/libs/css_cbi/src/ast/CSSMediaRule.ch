
struct CSSMediaRule {

    var query : std::string_view

    var declarations : std::vector<*mut CSSDeclaration>

    var parent : *mut ASTNode

}
