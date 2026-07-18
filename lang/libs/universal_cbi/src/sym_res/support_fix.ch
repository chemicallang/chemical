func resolve_page_children(
    resolver : *mut SymbolResolver,
    support : &mut SymResSupport,
    diagnoser : *mut ASTDiagnoser,
    page : *mut ASTNode,
    loc : ubigint
) : bool {

    const appendHeadJsCharFn = page.child("append_js_char")
    if(appendHeadJsCharFn == null) {
        diagnoser.error("'append_js_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendHeadJsCharPtrFn == null) {
        diagnoser.error("'append_js_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsFn = page.child("append_js");
    if(appendHeadJsFn == null) {
        diagnoser.error("'append_js' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsIntFn = page.child("append_js_integer");
    if(appendHeadJsIntFn == null) {
        diagnoser.error("'append_js_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsUIntFn = page.child("append_js_uinteger");
    if(appendHeadJsUIntFn == null) {
        diagnoser.error("'append_js_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsFloatFn = page.child("append_js_float");
    if(appendHeadJsFloatFn == null) {
        diagnoser.error("'append_js_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadJsDoubleFn = page.child("append_js_double");
    if(appendHeadJsDoubleFn == null) {
        diagnoser.error("'append_js_double' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) {
        diagnoser.error("'append_html' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharFn = page.child("append_html_char");
    if(appendHtmlCharFn == null) {
        diagnoser.error("'append_html_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharPtrFn = page.child("append_html_char_ptr");
    if(appendHtmlCharPtrFn == null) {
        diagnoser.error("'append_html_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlIntFn = page.child("append_html_integer");
    if(appendHtmlIntFn == null) {
        diagnoser.error("'append_html_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlUIntFn = page.child("append_html_uinteger");
    if(appendHtmlUIntFn == null) {
        diagnoser.error("'append_html_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFloatFn = page.child("append_html_float");
    if(appendHtmlFloatFn == null) {
        diagnoser.error("'append_html_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlDoubleFn = page.child("append_html_double");
    if(appendHtmlDoubleFn == null) {
        diagnoser.error("'append_html_double' function is required on 'page' for html to work", loc);
        return false;
    }

    const requireComponentFn = page.child("require_component");
    if(requireComponentFn == null) {
        diagnoser.error("'require_component' function is required on 'page' for html to work", loc);
        return false;
    }

    const setComponentHashFn = page.child("set_component_hash");
    if(setComponentHashFn == null) {
        diagnoser.error("'set_component_hash' function is required on 'page' for html to work", loc);
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
        diagnoser.error("'get_html_size' function is required on 'page' for html to work", loc);
        return false;
    }
    const truncateHtmlFn = page.child("truncate_html");
    if(truncateHtmlFn == null) {
        diagnoser.error("'truncate_html' function is required on 'page' for html to work", loc);
        return false;
    }
    const pageHtmlNode = page.child("pageHtml");
    if(pageHtmlNode == null) {
        diagnoser.error("'pageHtml' member is required on 'page' for html to work", loc);
        return false;
    }
    const pageJsNode = page.child("pageJs");
    if(pageJsNode == null) {
        diagnoser.error("'pageJs' member is required on 'page' for js to work", loc);
        return false;
    }
    const capture_html_delta_to_js = page.child("capture_html_delta_to_js");
    if(capture_html_delta_to_js == null) {
        diagnoser.error("'capture_html_delta_to_js' function is required on 'page' for html capture to work", loc);
        return false;
    }

    const getJsPosFn = page.child("get_js_pos");
    if(getJsPosFn == null) {
        diagnoser.error("'get_js_pos' function is required on 'page' for hoisting to work", loc);
        return false;
    }
    const moveJsRangeFn = page.child("move_js_range");
    if(moveJsRangeFn == null) {
        diagnoser.error("'move_js_range' function is required on 'page' for hoisting to work", loc);
        return false;
    }
    const js_hoist_pos = page.child("js_hoist_pos");
    if(js_hoist_pos == null) {
        diagnoser.error("'js_hoist_pos' field is required on 'page' for hoisting to work", loc);
        return false;
    }

    support.getHtmlSizeFn = getHtmlSizeFn;
    support.truncateHtmlFn = truncateHtmlFn;
    support.pageHtmlNode = pageHtmlNode;
    support.pageJsNode = pageJsNode;
    support.capture_html_delta_to_js = capture_html_delta_to_js;
    support.getJsPosFn = getJsPosFn;
    support.moveJsRangeFn = moveJsRangeFn;
    support.js_hoist_pos = js_hoist_pos;

    return true
}

func sym_res_support(resolver : *mut SymbolResolver, support : &mut SymResSupport, diagnoser : *mut ASTDiagnoser, loc : ubigint) : bool {

    const ssrAttrLinkedNode = resolver.resolve("SsrAttribute");
    if(ssrAttrLinkedNode == null) {
        diagnoser.error("couldn't find 'SsrAttribute' node", loc);
        return false;
    }

    const ssrTextLinkedNode = resolver.resolve("SsrText");
    if(ssrTextLinkedNode == null) {
        diagnoser.error("couldn't find 'SsrText' node", loc);
        return false;
    }

    const ssrAttributeValueNode = resolver.resolve("SsrAttributeValue");
    if(ssrAttributeValueNode == null) {
        diagnoser.error("couldn't find 'SsrAttributeValue' node", loc);
        return false;
    }

    const multipleAttributeValueNode = resolver.resolve("MultipleAttributeValues");
    if(multipleAttributeValueNode == null) {
        diagnoser.error("couldn't find 'MultipleAttributeValues' node", loc);
        return false;
    }

    const ssrAttributeListNode = resolver.resolve("SsrAttributeList");
    if(ssrAttributeListNode == null) {
        diagnoser.error("couldn't find 'SsrAttributeList' node", loc);
        return false;
    }

    const renderHtmlAttrs = resolver.resolve("renderHtmlAttrs")
    if(renderHtmlAttrs == null) {
        diagnoser.error("couldn't find 'renderHtmlAttrs' node", loc);
        return false;
    }

    const renderJsAttrs = resolver.resolve("renderJsAttrs")
    if(renderJsAttrs == null) {
        diagnoser.error("couldn't find 'renderJsAttrs' node", loc);
        return false;
    }

    const getSsrAttributeValueFn = resolver.resolve("getSsrAttributeValue")
    if(getSsrAttributeValueFn == null) {
        diagnoser.error("couldn't find 'getSsrAttributeValue' node", loc);
        return false;
    }

    const renderHtmlAttrValueFn = resolver.resolve("renderHtmlAttrValue")
    if(renderHtmlAttrValueFn == null) {
        diagnoser.error("couldn't find 'renderHtmlAttrValue' node", loc);
        return false;
    }

    const renderJsAttrValueFn = resolver.resolve("renderJsAttrValue")
    if(renderJsAttrValueFn == null) {
        diagnoser.error("couldn't find 'renderJsAttrValue' node", loc);
        return false;
    }

    const isSsrAttributeValueTruthyFn = resolver.resolve("isSsrAttributeValueTruthy")
    if(isSsrAttributeValueTruthyFn == null) {
        diagnoser.error("couldn't find 'isSsrAttributeValueTruthy' node", loc);
        return false;
    }

    const renderHtmlChildValueFn = resolver.resolve("renderHtmlChildValue")
    if(renderHtmlChildValueFn == null) {
        diagnoser.error("couldn't find 'renderHtmlChildValue' node", loc);
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
    support.renderHtmlChildValueFn = renderHtmlChildValueFn
    support.renderJsAttrValueFn = renderJsAttrValueFn
    support.getSsrAttributeValueFn = getSsrAttributeValueFn
    support.isSsrAttributeValueTruthyFn = isSsrAttributeValueTruthyFn

    const stdNamespace = resolver.resolve("std")
    if(stdNamespace == null) {
        diagnoser.error("couldn't find 'std' namespace", loc);
        return false;
    }
    const stringNode = stdNamespace.child("string")
    if(stringNode == null) {
        diagnoser.error("couldn't find 'std::string' node", loc);
        return false;
    }
    const stringNodeMake = stringNode.child("empty_str");
    if(stringNodeMake == null) {
        diagnoser.error("couldn't find 'std::string::empty_str'", loc);
        return false;
    }
    const appendWithLenFn = stringNode.child("append_with_len")
    if(appendWithLenFn == null) {
        diagnoser.error("'append_with_len' function is required on 'std::string'", loc);
        return false;
    }
    const dataFn = stringNode.child("data")
    if(dataFn == null) {
        diagnoser.error("'data' function is required on 'std::string'", loc);
        return false;
    }
    const sizeFn = stringNode.child("size")
    if(sizeFn == null) {
        diagnoser.error("'size' function is required on 'std::string'", loc);
        return false;
    }

    support.stringNodeMake = stringNodeMake
    support.appendWithLenFn = appendWithLenFn
    support.dataFn = dataFn
    support.sizeFn = sizeFn

    return true;

}
