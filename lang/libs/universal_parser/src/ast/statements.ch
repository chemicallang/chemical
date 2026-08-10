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

public struct JsDebugger {
    var base : JsNode
}
