public enum JsNodeKind {
    VarDecl,
    FunctionDecl,
    Literal,
    ChemicalValue,
    Identifier,
    MemberAccess,
    FunctionCall,
    ExpressionStatement,
    Block,
    If,
    Return,
    BinaryOp,
    ArrowFunction,
    ArrayLiteral,
    ObjectLiteral,
    For,
    While
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
    var keyword : std::string_view
}

public struct JsLiteral {
    var base : JsNode
    var value : std::string_view
}

public struct JsIdentifier {
    var base : JsNode
    var value : std::string_view
}

public struct JsChemicalValue {
    var base : JsNode
    var value : *mut Value
}

public struct JsFunctionCall {
    var base : JsNode
    var callee : *mut JsNode
    var args : std::vector<*mut JsNode>
}

public struct JsBlock {
    var base : JsNode
    var statements : std::vector<*mut JsNode>
}

public struct JsIf {
    var base : JsNode
    var condition : *mut JsNode
    var thenBlock : *mut JsNode
    var elseBlock : *mut JsNode
}

public struct JsReturn {
    var base : JsNode
    var value : *mut JsNode
}

public struct JsBinaryOp {
    var base : JsNode
    var left : *mut JsNode
    var right : *mut JsNode
    var op : std::string_view
}

public struct JsFunctionDecl {
    var base : JsNode
    var name : std::string_view
    var params : std::vector<std::string_view>
    var body : *mut JsNode
}

public struct JsMemberAccess {
    var base : JsNode
    var object : *mut JsNode
    var property : std::string_view
}

public struct JsExpressionStatement {
    var base : JsNode
    var expression : *mut JsNode
}

public struct JsArrowFunction {
    var base : JsNode
    var params : std::vector<std::string_view>
    var body : *mut JsNode
}

public struct JsArrayLiteral {
    var base : JsNode
    var elements : std::vector<*mut JsNode>
}

public struct JsProperty {
    var key : std::string_view
    var value : *mut JsNode
}

public struct JsObjectLiteral {
    var base : JsNode
    var properties : std::vector<JsProperty>
}

public struct JsFor {
    var base : JsNode
    var init : *mut JsNode
    var condition : *mut JsNode
    var update : *mut JsNode
    var body : *mut JsNode
}

public struct JsWhile {
    var base : JsNode
    var condition : *mut JsNode
    var body : *mut JsNode
}
