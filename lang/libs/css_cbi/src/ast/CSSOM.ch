struct GlobalBlock {

    var rules : std::vector<CSSRuleSet>

}

struct CSSOM {

    var parent : *mut ASTNode

    var declarations : std::vector<*mut CSSDeclaration>

    var media_queries : std::vector<*mut CSSMediaRule>

    // chemical values
    var dyn_values : std::vector<*mut Value>

    var className : std::string_view

    var global : GlobalBlock

    var support : SymResSupport

    func has_dynamic_values(&self) : bool {
        return !dyn_values.empty()
    }

    func is_hashable(&self) : bool {
        return dyn_values.empty() && media_queries.empty()
    }

}