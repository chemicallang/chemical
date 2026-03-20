func resolve_page_children(
    resolver : *mut SymbolResolver,
    support : &mut SymResSupport,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendHeadJsCharFn = page.child("append_js_char")
    if(appendHeadJsCharFn == null) {
        resolver.error("'append_js_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendHeadJsCharPtrFn == null) {
        resolver.error("'append_js_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsFn = page.child("append_js");
    if(appendHeadJsFn == null) {
        resolver.error("'append_js' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsIntFn = page.child("append_js_integer");
    if(appendHeadJsIntFn == null) {
        resolver.error("'append_js_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsUIntFn = page.child("append_js_uinteger");
    if(appendHeadJsUIntFn == null) {
        resolver.error("'append_js_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsFloatFn = page.child("append_js_float");
    if(appendHeadJsFloatFn == null) {
        resolver.error("'append_js_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsDoubleFn = page.child("append_js_double");
    if(appendHeadJsDoubleFn == null) {
        resolver.error("'append_js_double' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) {
        resolver.error("'append_html' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharFn = page.child("append_html_char");
    if(appendHtmlCharFn == null) {
        resolver.error("'append_html_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharPtrFn = page.child("append_html_char_ptr");
    if(appendHtmlCharPtrFn == null) {
        resolver.error("'append_html_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlIntFn = page.child("append_html_integer");
    if(appendHtmlIntFn == null) {
        resolver.error("'append_html_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlUIntFn = page.child("append_html_uinteger");
    if(appendHtmlUIntFn == null) {
        resolver.error("'append_html_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFloatFn = page.child("append_html_float");
    if(appendHtmlFloatFn == null) {
        resolver.error("'append_html_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlDoubleFn = page.child("append_html_double");
    if(appendHtmlDoubleFn == null) {
        resolver.error("'append_html_double' function is required on 'page' for html to work", loc);
        return false;
    }

    const requireComponentFn = page.child("require_component");
    if(requireComponentFn == null) {
        resolver.error("'require_component' function is required on 'page' for html to work", loc);
        return false;
    }

    const setComponentHashFn = page.child("set_component_hash");
    if(setComponentHashFn == null) {
        resolver.error("'set_component_hash' function is required on 'page' for html to work", loc);
        return false;
    }

    support.requireComponentFn = requireComponentFn;
    support.setComponentHashFn = setComponentHashFn;

    support.appendHeadJsFn = appendHeadJsFn
    support.appendHeadJsCharFn = appendHeadJsCharFn
    support.appendHeadJsCharPtrFn = appendHeadJsCharPtrFn
    support.appendHeadJsIntFn = appendHeadJsIntFn;
    support.appendHeadJsUIntFn = appendHeadJsUIntFn;
    support.appendHeadJsFloatFn = appendHeadJsFloatFn;
    support.appendHeadJsDoubleFn = appendHeadJsDoubleFn;

    support.appendHtmlFn = appendHtmlFn;
    support.appendHtmlCharFn = appendHtmlCharFn;
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn;
    support.appendHtmlIntFn = appendHtmlIntFn;
    support.appendHtmlUIntFn = appendHtmlUIntFn;
    support.appendHtmlFloatFn = appendHtmlFloatFn;
    support.appendHtmlDoubleFn = appendHtmlDoubleFn;

    const getHtmlSizeFn = page.child("get_html_size");
    if(getHtmlSizeFn == null) {
        resolver.error("'get_html_size' function is required on 'page' for html to work", loc);
        return false;
    }
    const truncateHtmlFn = page.child("truncate_html");
    if(truncateHtmlFn == null) {
        resolver.error("'truncate_html' function is required on 'page' for html to work", loc);
        return false;
    }
    const pageHtmlNode = page.child("pageHtml");
    if(pageHtmlNode == null) {
        resolver.error("'pageHtml' member is required on 'page' for html to work", loc);
        return false;
    }
    const pageJsNode = page.child("pageJs");
    if(pageJsNode == null) {
        resolver.error("'pageJs' member is required on 'page' for js to work", loc);
        return false;
    }
    support.getHtmlSizeFn = getHtmlSizeFn;
    support.truncateHtmlFn = truncateHtmlFn;
    support.pageHtmlNode = pageHtmlNode;
    support.pageJsNode = pageJsNode;

    return true
}

func sym_res_support(resolver : *mut SymbolResolver, support : &mut SymResSupport, loc : ubigint) : bool {

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

    const stdNamespace = resolver.find("std")
    if(stdNamespace == null) {
        resolver.error("couldn't find 'std' namespace", loc);
        return false;
    }
    const stringNode = stdNamespace.child("string")
    if(stringNode == null) {
        resolver.error("couldn't find 'std::string' node", loc);
        return false;
    }
    const stringNodeMake = stringNode.child("empty_str");
    if(stringNodeMake == null) {
        resolver.error("couldn't find 'std::string::empty_str'", loc);
        return false;
    }
    const appendWithLenFn = stringNode.child("append_with_len")
    if(appendWithLenFn == null) {
        resolver.error("'append_with_len' function is required on 'std::string'", loc);
        return false;
    }
    const dataFn = stringNode.child("data")
    if(dataFn == null) {
        resolver.error("'data' function is required on 'std::string'", loc);
        return false;
    }
    const sizeFn = stringNode.child("size")
    if(sizeFn == null) {
        resolver.error("'size' function is required on 'std::string'", loc);
        return false;
    }

    support.stringNodeMake = stringNodeMake
    support.appendWithLenFn = appendWithLenFn
    support.dataFn = dataFn
    support.sizeFn = sizeFn

    return true;

}
