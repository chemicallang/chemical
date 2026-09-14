// JsNodeKind lives in js_syntax; the shared AST nodes live in js_syntax/src/Ast.ch.
// Only js_parser-specific nodes remain here.

public struct JsRoot {
    var statements : std::vector<*mut JsNode>
    var parent : *mut ASTNode
    var support : SymResSupport
    var dyn_values : std::vector<*mut Value>
}

public struct JsRegexLiteral {
    var base : JsNode
    var value : std::string_view
}
