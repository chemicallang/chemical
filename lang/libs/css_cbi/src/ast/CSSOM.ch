struct GlobalBlock {

    var rules : std::vector<CSSRuleSet>

}

struct CSSOM {

    var parent : *mut ASTNode

    // if NOT has dynamic values, we will automatically put
    // class name that is hashed and prefixed with 'h'
    var has_dynamic_values : bool

    var declarations : std::vector<*mut CSSDeclaration>

    var className : std::string_view

    var global : GlobalBlock

    var support : SymResSupport

}