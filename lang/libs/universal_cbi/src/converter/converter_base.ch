enum BufferType {
    JavaScript,
    HTML
}

struct JsStateInit {
    var name : std::string_view
    var init : std::string_view
}

// A local variable declared in a universal component body (e.g. `var variant =
// props.variant || "default"`). SSR emits these as Chemical var statements so
// JSX attributes that reference them ({variant}) resolve at SSR runtime.
struct JsSsrLocal {
    var name : std::string_view
    var varInit : *mut ASTNode
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
    // Local variables declared in the current universal component body, tracked
    // so JSX attribute/child expressions can reference them during SSR.
    var ssr_locals : std::vector<JsSsrLocal>

    // Temporary binding used while statically evaluating `.filter()` predicates
    // against the elements of a static (state/array-literal) source.
    var ssr_bound_param : std::string_view = ""
    var ssr_bound_param_valid : bool = false
    var ssr_bound_param_value : SsrJsExprEval
}
