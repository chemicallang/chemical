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
    ArrayDestructuring
}

public struct JsNode {
    var kind : JsNodeKind
}

public struct JsRoot {
    var statements : std::vector<*mut JsNode>
    var parent : *mut ASTNode
    var support : SymResSupport
    var dyn_values : std::vector<*mut Value>
}

public struct JsVarDecl {
    var base : JsNode
    var name : std::string_view
    var pattern : *mut JsNode
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
    var is_async : bool
    var is_generator : bool
}

public struct JsMemberAccess {
    var base : JsNode
    var object : *mut JsNode
    var property : std::string_view
}

public struct JsIndexAccess {
    var base : JsNode
    var object : *mut JsNode
    var index : *mut JsNode
}

public struct JsExpressionStatement {
    var base : JsNode
    var expression : *mut JsNode
}

public struct JsArrowFunction {
    var base : JsNode
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var is_async : bool
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

public struct JsForIn {
    var base : JsNode
    var left : *mut JsNode
    var right : *mut JsNode
    var body : *mut JsNode
}

public struct JsForOf {
    var base : JsNode
    var left : *mut JsNode
    var right : *mut JsNode
    var body : *mut JsNode
}

public struct JsSpread {
    var base : JsNode
    var argument : *mut JsNode
}

public struct JsWhile {
    var base : JsNode
    var condition : *mut JsNode
    var body : *mut JsNode
}

public struct JsDoWhile {
    var base : JsNode
    var condition : *mut JsNode
    var body : *mut JsNode
}

public struct JsCase {
    var test : *mut JsNode  // null for default
    var body : std::vector<*mut JsNode>
}

public struct JsSwitch {
    var base : JsNode
    var discriminant : *mut JsNode
    var cases : std::vector<JsCase>
}

public struct JsBreak {
    var base : JsNode
}

public struct JsContinue {
    var base : JsNode
}

public struct JsTryCatch {
    var base : JsNode
    var tryBlock : *mut JsNode
    var catchParam : std::string_view
    var catchBlock : *mut JsNode
    var finallyBlock : *mut JsNode
}

public struct JsThrow {
    var base : JsNode
    var argument : *mut JsNode
}

public struct JsTernary {
    var base : JsNode
    var condition : *mut JsNode
    var consequent : *mut JsNode
    var alternate : *mut JsNode
}

public struct JsUnaryOp {
    var base : JsNode
    var operator : std::string_view
    var operand : *mut JsNode
    var prefix : bool
}

public struct JsClassMethod {
    var name : std::string_view
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var is_static : bool
}

public struct JsClassDecl {
    var base : JsNode
    var name : std::string_view
    var superClass : std::string_view // optional
    var methods : std::vector<JsClassMethod>
}

public struct JsImport {
    var base : JsNode
    var source : std::string_view
    // Simple import for now: import { x } from "y" or import "y"
    // For now just storing raw source string or assuming specific structure?
    // Let's store specifiers as string for now if we don't parse them fully, 
    // but better to parse them.
    // Let's assume generic structure for now:
    // import parts from "source";
    var specifiers : std::vector<ImportSpecifier>
}

public struct ImportSpecifier {
    var imported : std::string_view
    var local : std::string_view
}

public struct JsExport {
    var base : JsNode
    // export var x = ...;
    // export default ...;
    var declaration : *mut JsNode
    var is_default : bool
}

public struct JsYield {
    var base : JsNode
    var argument : *mut JsNode
    var delegate : bool
}

public struct JsDebugger {
    var base : JsNode
}
