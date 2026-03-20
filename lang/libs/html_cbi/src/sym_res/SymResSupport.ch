struct SymResSupport {

    var ssrAttrLinkedNode : *mut ASTNode = null

    var ssrTextLinkedNode : *mut ASTNode = null

    var ssrAttributeValueNode : *mut ASTNode = null

    var multipleAttributeValueNode : *mut ASTNode = null

    var ssrAttributeListNode : *mut ASTNode = null

    var pageNode : *mut ASTNode = null

    // ssr attribute rendering functions
    var renderJsAttrs : *mut ASTNode = null

    // Page Children

    var appendHtmlCharFn : *mut ASTNode = null

    var appendHtmlCharPtrFn : *mut ASTNode = null

    var appendHtmlFn : *mut ASTNode = null

    var appendHtmlIntFn : *mut ASTNode = null

    var appendHtmlUIntFn : *mut ASTNode = null

    var appendHtmlFloatFn : *mut ASTNode = null

    var appendHtmlDoubleFn : *mut ASTNode = null

    var appendHeadCharFn : *mut ASTNode = null

    var appendHeadCharPtrFn : *mut ASTNode = null

    var appendHeadFn : *mut ASTNode = null

    var appendHeadIntFn : *mut ASTNode = null

    var appendHeadUIntFn : *mut ASTNode = null

    var appendHeadFloatFn : *mut ASTNode = null

    var appendHeadDoubleFn : *mut ASTNode = null

    var appendJsCharFn : *mut ASTNode = null

    var appendJsCharPtrFn : *mut ASTNode = null

    var appendJsFn : *mut ASTNode = null

    var appendJsIntFn : *mut ASTNode = null

    var appendJsUIntFn : *mut ASTNode = null

    var appendJsFloatFn : *mut ASTNode = null

    var appendJsDoubleFn : *mut ASTNode = null

    var requireComponentFn : *mut ASTNode = null

    var setComponentHashFn : *mut ASTNode = null

    // capture nodes
    var pageHtmlNode : *mut ASTNode = null
    var getHtmlSizeFn : *mut ASTNode = null
    var truncateHtmlFn : *mut ASTNode = null
    var stringNodeMake : *mut ASTNode = null
    var appendWithLenFn : *mut ASTNode = null
    var dataFn : *mut ASTNode = null
    var sizeFn : *mut ASTNode = null
    var childrenParamNode : *mut ASTNode = null

}
