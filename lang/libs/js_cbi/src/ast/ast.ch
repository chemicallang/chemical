public enum JsNodeKind {
    VarDecl,
    Literal
}

public struct JsNode {
    var kind : JsNodeKind
}

public struct JsRoot {
    var statements : std::vector<*mut JsNode>
    var parent : *mut ASTNode
    var support : SymResSupport
}

public struct JsVarDecl {
    var base : JsNode
    var name : std::string_view
    var value : *mut JsNode
}

public struct JsLiteral {
    var base : JsNode
    var value : std::string_view
}
