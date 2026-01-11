public struct JsComponentDecl {
    var base : JsNode
    var name : std::string_view
    var params : std::vector<std::string_view>
    var body : *mut JsNode
    var support : SymResSupport
    var dyn_values : std::vector<*mut Value>
}

public struct JsJSXAttribute {
    var base : JsNode
    var name : std::string_view // string if hyphenated? or identifier? 
    // Usually name can be Namespaced (ns:attr) or simple. For now treat as string_view.
    var value : *mut JsNode // Literal (String) or JSXExpressionContainer or null (bool true)
}

public struct JsJSXSpreadAttribute {
    var base : JsNode
    var argument : *mut JsNode
}

public struct JsJSXOpeningElement {
    var tagName : *mut JsNode // Identifier, MemberExpression
    var attributes : std::vector<*mut JsNode> // JSXAttribute or JSXSpreadAttribute
    var selfClosing : bool
}

public struct JsJSXClosingElement {
    var tagName : *mut JsNode
}

public struct JsJSXElement {
    var base : JsNode
    var opening : JsJSXOpeningElement
    var children : std::vector<*mut JsNode>
    var closing : JsJSXClosingElement // Optional if selfClosing? But structure usually keeps it or null. 
    // Typically we might not store closing element in the AST if it matches, but for exact reproduction or source mapping we might.
    // Let's simplify and assume consistency check is done during parsing.
    // But wait, the `closing` struct above is useful for parsing return.
}

public struct JsJSXFragment {
    var base : JsNode
    var children : std::vector<*mut JsNode>
}

public struct JsJSXText {
    var base : JsNode
    var value : std::string_view
}

public struct JsJSXExpressionContainer {
    var base : JsNode
    var expression : *mut JsNode // Can be empty? {} -> Empty expression? Or null.
}
