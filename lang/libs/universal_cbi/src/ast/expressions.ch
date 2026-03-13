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

public struct JsBinaryOp {
    var base : JsNode
    var left : *mut JsNode
    var right : *mut JsNode
    var op : std::string_view
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

public struct JsArrowFunction {
    var base : JsNode
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var is_async : bool
    var contains_jsx : bool
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

public struct JsSpread {
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

public struct JsYield {
    var base : JsNode
    var argument : *mut JsNode
    var delegate : bool
}
