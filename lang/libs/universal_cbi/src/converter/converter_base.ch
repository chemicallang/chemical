enum BufferType {
    JavaScript,
    HTML
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
    var target : BufferType = BufferType.JavaScript
    var current_func : *mut FunctionDeclaration = null
}
