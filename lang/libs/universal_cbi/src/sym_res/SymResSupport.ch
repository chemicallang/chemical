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

    var pageNode : *mut ASTNode = null

    // children of page that append to head js
    var appendHeadJsFn : *mut ASTNode = null
    var appendHeadJsCharFn : *mut ASTNode = null
    var appendHeadJsCharPtrFn : *mut ASTNode = null
    var appendHeadJsIntFn : *mut ASTNode = null
    var appendHeadJsUIntFn : *mut ASTNode = null
    var appendHeadJsFloatFn : *mut ASTNode = null
    var appendHeadJsDoubleFn : *mut ASTNode = null

    // children of page that append to html
    var appendHtmlFn : *mut ASTNode = null
    var appendHtmlCharFn : *mut ASTNode = null
    var appendHtmlCharPtrFn : *mut ASTNode = null
    var appendHtmlIntFn : *mut ASTNode = null
    var appendHtmlUIntFn : *mut ASTNode = null
    var appendHtmlFloatFn : *mut ASTNode = null
    var appendHtmlDoubleFn : *mut ASTNode = null

    // require component, set component hash
    var requireComponentFn : *mut ASTNode = null
    var setComponentHashFn : *mut ASTNode = null

}
