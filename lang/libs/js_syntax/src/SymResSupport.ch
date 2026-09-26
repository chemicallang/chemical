// Single shared symbol-resolution support struct.
//
// Union of the former per-package SymResSupport structs (universal_parser's SSR
// fields plus js_parser's plain-JS appendJs* fields). Both front ends import
// this so `SymResSupport` is declared once.
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
    var renderHtmlChildValueFn : *mut ASTNode = null
    var renderJsAttrValueFn : *mut ASTNode = null
    var getSsrAttributeValueFn : *mut ASTNode = null
    var ssrAttrValuePropFn : *mut ASTNode = null
    var ssrTextIncludesFn : *mut ASTNode = null
    var ssrTextStartsWithFn : *mut ASTNode = null
    var ssrTextEndsWithFn : *mut ASTNode = null
    var ssrTextIncludesFoldFn : *mut ASTNode = null
    var ssrTextStartsWithFoldFn : *mut ASTNode = null
    var ssrTextEndsWithFoldFn : *mut ASTNode = null
    var isSsrAttributeValueTruthyFn : *mut ASTNode = null
    var getMultipleAttributeValuesFn : *mut ASTNode = null
    var ssrMultipleGetFn : *mut ASTNode = null
    var ssrTextEqualsFn : *mut ASTNode = null
    var ssrValuesEqualFn : *mut ASTNode = null
    var ssrPickValueFn : *mut ASTNode = null
    var ssrMakeTextValueFn : *mut ASTNode = null
    var ssrMakeBoolValueFn : *mut ASTNode = null
    var ssrMakeUIntegerValueFn : *mut ASTNode = null
    var ssrMakeIntegerValueFn : *mut ASTNode = null
    var ssrMakeMultipleValueFn : *mut ASTNode = null
    var ssrNoneValueFn : *mut ASTNode = null

    var pageNode : *mut ASTNode = null

    // children of page that append to head js
    var appendHeadJsFn : *mut ASTNode = null
    var appendHeadJsCharFn : *mut ASTNode = null
    var appendHeadJsCharPtrFn : *mut ASTNode = null
    var appendHeadJsEscapedCharPtrFn : *mut ASTNode = null
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

    // capture nodes
    var pageHtmlNode : *mut ASTNode = null
    var pageJsNode : *mut ASTNode = null
    var getHtmlSizeFn : *mut ASTNode = null
    var truncateHtmlFn : *mut ASTNode = null
    var stringNodeMake : *mut ASTNode = null
    var appendWithLenFn : *mut ASTNode = null
    var dataFn : *mut ASTNode = null
    var sizeFn : *mut ASTNode = null
    var childrenParamNode : *mut ASTNode = null
    var getJsPosFn : *mut ASTNode = null
    var moveJsRangeFn : *mut ASTNode = null
    var js_hoist_pos : *mut ASTNode = null
    var renderJsOnlyNode : *mut ASTNode = null
    // universal router: `router::apply_route_url` (optional; only resolved when
    // the app imports the `router` library). Used by the generated router
    // function for server-side URL matching (§6.1).
    var applyRouteUrlFn : *mut ASTNode = null
    // plain-JS emitter support (formerly js_parser's SymResSupport)
    var appendJsFn : *mut ASTNode = null
    var appendJsCharFn : *mut ASTNode = null
    var appendJsCharPtrFn : *mut ASTNode = null
    var appendJsIntFn : *mut ASTNode = null
    var appendJsUIntFn : *mut ASTNode = null
    var appendJsFloatFn : *mut ASTNode = null
    var appendJsDoubleFn : *mut ASTNode = null
}
