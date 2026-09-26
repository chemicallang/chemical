// Shared JavaScript/JSX AST nodes.
//
// Moved here from js_parser and universal_parser; the two copies were
// byte-identical except JsArrowFunction (universal had `contains_jsx`).
// Both parsers import this module. Do NOT redeclare these structs.

public struct ImportSpecifier {
    var imported : std::string_view
    var local : std::string_view
}

public struct JsArrayLiteral {
    var base : JsNode
    var elements : std::vector<*mut JsNode>
}

public struct JsArrowFunction {
    var base : JsNode
    var params : std::vector<JsParam>
    var body : *mut JsNode
    var is_async : bool
    var contains_jsx : bool
}

public struct JsBinaryOp {
    var base : JsNode
    var left : *mut JsNode
    var right : *mut JsNode
    var op : std::string_view
}

public struct JsBlock {
    var base : JsNode
    var statements : std::vector<*mut JsNode>
}

public struct JsBreak {
    var base : JsNode
}

public struct JsCase {
    var test : *mut JsNode  // null for default
    var body : std::vector<*mut JsNode>
}

public struct JsChemicalValue {
    var base : JsNode
    var value : *mut Value
}

public struct JsClassDecl {
    var base : JsNode
    var name : std::string_view
    var superClass : std::string_view // optional
    var methods : std::vector<JsClassMethod>
}

public struct JsClassMethod { 
    var name : std::string_view
    var params : std::vector<JsParam>
    var body : *mut JsNode
    var is_static : bool
}

public struct JsContinue {
    var base : JsNode
}

public struct JsDebugger {
    var base : JsNode
}

public struct JsDoWhile {
    var base : JsNode
    var condition : *mut JsNode
    var body : *mut JsNode
}

public struct JsExport {
    var base : JsNode
    // export var x = ...;
    // export default ...;
    var declaration : *mut JsNode
    var is_default : bool
}

public struct JsExpressionStatement {
    var base : JsNode
    var expression : *mut JsNode
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

public struct JsFunctionCall {
    var base : JsNode
    var callee : *mut JsNode
    var args : std::vector<*mut JsNode>
}

public struct JsFunctionDecl {
    var base : JsNode
    var name : std::string_view
    var params : std::vector<JsParam>
    var body : *mut JsNode
    var is_async : bool
    var is_generator : bool
}

public struct JsIdentifier {
    var base : JsNode
    var value : std::string_view
}

public struct JsIf {
    var base : JsNode
    var condition : *mut JsNode
    var thenBlock : *mut JsNode
    var elseBlock : *mut JsNode
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

public struct JsIndexAccess {
    var base : JsNode
    var object : *mut JsNode
    var index : *mut JsNode
}

public struct JsLiteral {
    var base : JsNode
    var value : std::string_view
}

public struct JsMemberAccess {
    var base : JsNode
    var object : *mut JsNode
    var property : std::string_view
}

public struct JsNode {
    var kind : JsNodeKind
}

public struct JsObjectLiteral {
    var base : JsNode
    var properties : std::vector<JsProperty>
}

public struct JsParam {
    var name : std::string_view
    var default_value : *mut JsNode
}

public struct JsProperty {
    var key : std::string_view
    var value : *mut JsNode
}

public struct JsReturn {
    var base : JsNode
    var value : *mut JsNode
}

public struct JsSpread {
    var base : JsNode
    var argument : *mut JsNode
}

public struct JsSwitch {
    var base : JsNode
    var discriminant : *mut JsNode
    var cases : std::vector<JsCase>
}

public struct JsTernary {
    var base : JsNode
    var condition : *mut JsNode
    var consequent : *mut JsNode
    var alternate : *mut JsNode
}

public struct JsThrow {
    var base : JsNode
    var argument : *mut JsNode
}

public struct JsTryCatch {
    var base : JsNode
    var tryBlock : *mut JsNode
    var catchParam : std::string_view
    var catchBlock : *mut JsNode
    var finallyBlock : *mut JsNode
}

public struct JsUnaryOp {
    var base : JsNode
    var operator : std::string_view
    var operand : *mut JsNode
    var prefix : bool
}

public struct JsVarDecl {
    var base : JsNode
    var name : std::string_view
    var pattern : *mut JsNode
    var value : *mut JsNode
    var keyword : std::string_view
}

public struct JsWhile {
    var base : JsNode
    var condition : *mut JsNode
    var body : *mut JsNode
}

public struct JsYield {
    var base : JsNode
    var argument : *mut JsNode
    var delegate : bool
}

// ── Universal router declarations (see lang/docs/universal-router-design.md §14.2)

// `router "name" { route ... }` inside a `#universal` component body.
public struct JsRouterDecl {
    var base : JsNode
    var name : std::string_view
    var routes : std::vector<*mut JsNode>
    var decl_loc : ubigint                       // source location of the router keyword
}

// One `route` inside a router block.
// raw is exactly as written:  "#id" | "/a/{b}" | "*"
public struct JsRouteDecl {
    var base : JsNode
    var raw : std::string_view
    var id : std::string_view                    // resolved id / normalized pattern
    var pattern : std::string_view               // "" for id + fallback routes, "/a/{b}" for URL routes
    var is_url : bool
    var is_fallback : bool
    var is_default : bool
    var mode : std::string_view                  // "" | "lazy" | "preload" | "remote"
    var noscroll : bool                          // `route #"x" noscroll`: skip scroll restore
    var title : std::string_view                 // "" when absent
    var hooks : std::vector<*mut JsNode>
    var body : *mut JsNode
    var decl_loc : ubigint
}

// `onActivate(() => { ... })` / `onDeactivate` / `onBeforeActivate` next to the route body.
public struct JsRouteHook {
    var base : JsNode
    var name : std::string_view
    var fn : *mut JsNode
}
