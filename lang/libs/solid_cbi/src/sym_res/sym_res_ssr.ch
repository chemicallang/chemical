func sym_res_ssr_support(resolver : *mut SymbolResolver, support : &mut SymResSupport, loc : ubigint) : bool {

    const ssrAttrLinkedNode = resolver.find("SsrAttribute");
    if(ssrAttrLinkedNode == null) {
        resolver.error("couldn't find 'SsrAttribute' node", loc);
        return false;
    }

    const ssrTextLinkedNode = resolver.find("SsrText");
    if(ssrTextLinkedNode == null) {
        resolver.error("couldn't find 'SsrText' node", loc);
        return false;
    }

    const ssrAttributeValueNode = resolver.find("SsrAttributeValue");
    if(ssrAttributeValueNode == null) {
        resolver.error("couldn't find 'SsrAttributeValue' node", loc);
        return false;
    }

    const multipleAttributeValueNode = resolver.find("MultipleAttributeValues");
    if(multipleAttributeValueNode == null) {
        resolver.error("couldn't find 'MultipleAttributeValues' node", loc);
        return false;
    }

    const ssrAttributeListNode = resolver.find("SsrAttributeList");
    if(ssrAttributeListNode == null) {
        resolver.error("couldn't find 'SsrAttributeList' node", loc);
        return false;
    }

    const renderHtmlAttrs = resolver.find("renderHtmlAttrs")
    if(renderHtmlAttrs == null) {
        resolver.error("couldn't find 'renderHtmlAttrs' node", loc);
        return false;
    }

    const renderJsAttrs = resolver.find("renderJsAttrs")
    if(renderJsAttrs == null) {
        resolver.error("couldn't find 'renderJsAttrs' node", loc);
        return false;
    }

    const getSsrAttributeValueFn = resolver.find("getSsrAttributeValue")
    if(getSsrAttributeValueFn == null) {
        resolver.error("couldn't find 'getSsrAttributeValue' node", loc);
        return false;
    }

    const renderHtmlAttrValueFn = resolver.find("renderHtmlAttrValue")
    if(renderHtmlAttrValueFn == null) {
        resolver.error("couldn't find 'renderHtmlAttrValue' node", loc);
        return false;
    }

    const renderJsAttrValueFn = resolver.find("renderJsAttrValue")
    if(renderJsAttrValueFn == null) {
        resolver.error("couldn't find 'renderJsAttrValue' node", loc);
        return false;
    }

    support.ssrAttrLinkedNode = ssrAttrLinkedNode
    support.ssrTextLinkedNode = ssrTextLinkedNode
    support.ssrAttributeValueNode = ssrAttributeValueNode
    support.multipleAttributeValueNode = multipleAttributeValueNode
    support.ssrAttributeListNode = ssrAttributeListNode
    support.renderHtmlAttrs = renderHtmlAttrs
    support.renderJsAttrs = renderJsAttrs
    support.renderHtmlAttrValueFn = renderHtmlAttrValueFn
    support.renderJsAttrValueFn = renderJsAttrValueFn
    support.getSsrAttributeValueFn = getSsrAttributeValueFn

    return true;

}
