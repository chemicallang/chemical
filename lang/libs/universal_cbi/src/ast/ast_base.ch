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
    While,
    DoWhile,
    Switch,
    Case,
    Break,
    Continue,
    TryCatch,
    Throw,
    Ternary,
    UnaryOp,
    ForIn,
    ForOf,
    Spread,
    ClassDecl,
    Import,
    Export,
    Yield,
    Debugger,
    ComponentDecl,
    JSXElement,
    JSXExpressionContainer,
    JSXText,
    JSXFragment,
    JSXAttribute,
    JSXSpreadAttribute,
    IndexAccess,
    ArrayDestructuring,
    Paren
}

public struct JsNode {
    var kind : JsNodeKind
}

public struct JsParam {
    var name : std::string_view
    var default_value : *mut JsNode
}