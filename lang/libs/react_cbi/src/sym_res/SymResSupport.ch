public struct SymResSupport {

    // ssr types
    var ssrAttrLinkedNode : *mut ASTNode = null
    var ssrTextLinkedNode : *mut ASTNode = null
    var ssrAttributeValueNode : *mut ASTNode = null
    var multipleAttributeValueNode : *mut ASTNode = null
    var ssrAttributeListNode : *mut ASTNode = null

    // ssr attribute rendering functions
    var renderHtmlAttrs : *mut ASTNode = null
    var renderJsAttrs : *mut ASTNode = null
    var renderHtmlAttrValueFn : *mut ASTNode = null
    var renderJsAttrValueFn : *mut ASTNode = null
    var getSsrAttributeValueFn : *mut ASTNode = null

    var move_html_to_js_with_lambda_start : *mut ASTNode = null

    var pageNode : *mut ASTNode = null
    var appendHeadJsFn : *mut ASTNode = null
    var appendHeadJsCharFn : *mut ASTNode = null
    var appendHeadJsCharPtrFn : *mut ASTNode = null
    var appendHeadJsIntFn : *mut ASTNode = null
    var appendHeadJsUIntFn : *mut ASTNode = null
    var appendHeadJsFloatFn : *mut ASTNode = null
    var appendHeadJsDoubleFn : *mut ASTNode = null

    var requireComponentFn : *mut ASTNode = null
    var setComponentHashFn : *mut ASTNode = null

    var getHtmlSizeFn : *mut ASTNode = null
}
