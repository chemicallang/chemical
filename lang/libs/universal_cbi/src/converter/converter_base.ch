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

// A local variable bound to `createContext(name, default)` or `useContext(name)`
// in a universal component body (`const ctx = createContext("rg-" + props.name,
// "")`). The name/default expressions let consumers resolve the same registry
// entry as the provider, and let SSR resolve `ctx.value` to the static default
// (children render before their provider's SSR function, so a provider's
// published value is never observable at SSR time).
struct JsContextVar {
    var name : std::string_view
    var nameExpr : *mut JsNode
    var defaultExpr : *mut JsNode
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
    // Context variables bound to createContext/useContext in the current body.
    var context_vars : std::vector<JsContextVar>

    // Temporary binding used while statically evaluating `.filter()` predicates
    // against the elements of a static (state/array-literal) source.
    var ssr_bound_param : std::string_view = ""
    var ssr_bound_param_valid : bool = false
    var ssr_bound_param_value : SsrJsExprEval

    // Second `.map()` callback parameter (`(item, index) => ...`) bound during
    // compile-time unrolling of static sources. The runtime loop path binds it
    // as an ordinary SSR local (see emit_ssr_map_children).
    var ssr_index_param : std::string_view = ""
    var ssr_index_param_valid : bool = false
    var ssr_index_param_value : SsrJsExprEval
}
