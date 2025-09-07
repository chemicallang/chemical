struct GlobalBlock {

    var rules : std::vector<CSSRuleSet>

}

struct CSSOM {

    var parent : *mut ASTNode

    var declarations : std::vector<*mut CSSDeclaration>

    // chemical values
    var dyn_values : std::vector<*mut Value>

    var className : std::string_view

    var global : GlobalBlock

    var support : SymResSupport

    func has_dynamic_values(&self) : bool {
        return !dyn_values.empty()
    }

}