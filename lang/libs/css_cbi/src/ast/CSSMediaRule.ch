
struct CSSMediaRule {

    var queryList : *mut MediaQueryList

    var declarations : std::vector<*mut CSSDeclaration>

    var parent : *mut ASTNode

}
