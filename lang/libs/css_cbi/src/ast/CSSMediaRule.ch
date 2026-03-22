
struct CSSMediaRule {

    var queryList : *mut MediaQueryList

    var declarations : std::vector<*mut CSSDeclaration>

    var nested_rules : std::vector<*mut CSSNestedRule>

    var parent : *mut ASTNode

}
