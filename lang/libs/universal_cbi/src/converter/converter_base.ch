enum BufferType {
    JavaScript,
    HTML
}

struct JsStateInit {
    var name : std::string_view
    var init : std::string_view
}

struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
    var jsx_parent : std::string_view
    var t_counter : int = 0
    var id_counter : int = 0
    var state_vars : std::vector<std::string_view>
    var computed_vars : std::vector<std::string_view>
    var state_inits : std::vector<JsStateInit>
    var target : BufferType = BufferType.JavaScript
    var current_func : *mut FunctionDeclaration = null
    var component_props_name : std::string_view
    var in_jsx_attribute : bool = false
    var skip_reactive_deref : bool = false
    var function_depth : int = 0
}
