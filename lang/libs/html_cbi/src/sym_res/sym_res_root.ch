func sym_res_root(
    root : *mut HtmlRoot,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {

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

    var support = &mut root.support
    support.ssrAttrLinkedNode = ssrAttrLinkedNode
    support.ssrTextLinkedNode = ssrTextLinkedNode
    support.ssrAttributeValueNode = ssrAttributeValueNode
    support.multipleAttributeValueNode = multipleAttributeValueNode
    support.ssrAttributeListNode = ssrAttributeListNode

    const page = resolver.find("page");
    if(page == null) {
        resolver.error("couldn't find page variable", loc);
        return false;
    }

    const appendHtmlCharFn = page.child("append_html_char")
    if(appendHtmlCharFn == null) {
        resolver.error("'append_html_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlCharPtrFn = page.child("append_html_char_ptr")
    if(appendHtmlCharPtrFn == null) {
        resolver.error("'append_html_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHtmlFn = page.child("append_html");
    if(appendHtmlFn == null) {
        resolver.error("'append_html' function is required on 'page' for html to work", loc);
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

    support.pageNode = page;
    support.appendHtmlCharFn = appendHtmlCharFn
    support.appendHtmlCharPtrFn = appendHtmlCharPtrFn
    support.appendHtmlFn = appendHtmlFn
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
    support.getHtmlSizeFn = getHtmlSizeFn;
    support.truncateHtmlFn = truncateHtmlFn;
    support.pageHtmlNode = pageHtmlNode;

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

    const appendHeadCharFn = page.child("append_head_char")
    if(appendHeadCharFn == null) {
        resolver.error("'append_head_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadCharPtrFn = page.child("append_head_char_ptr")
    if(appendHeadCharPtrFn == null) {
        resolver.error("'append_head_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadFn = page.child("append_head");
    if(appendHeadFn == null) {
        resolver.error("'append_head' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadIntFn = page.child("append_head_integer");
    if(appendHeadIntFn == null) {
        resolver.error("'append_head_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadUIntFn = page.child("append_head_uinteger");
    if(appendHeadUIntFn == null) {
        resolver.error("'append_head_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadFloatFn = page.child("append_head_float");
    if(appendHeadFloatFn == null) {
        resolver.error("'append_head_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendHeadDoubleFn = page.child("append_head_double");
    if(appendHeadDoubleFn == null) {
        resolver.error("'append_head_double' function is required on 'page' for html to work", loc);
        return false;
    }

    support.appendHeadCharFn = appendHeadCharFn
    support.appendHeadCharPtrFn = appendHeadCharPtrFn
    support.appendHeadFn = appendHeadFn
    support.appendHeadIntFn = appendHeadIntFn;
    support.appendHeadUIntFn = appendHeadUIntFn;
    support.appendHeadFloatFn = appendHeadFloatFn;
    support.appendHeadDoubleFn = appendHeadDoubleFn;

    const appendJsCharFn = page.child("append_js_char")
    if(appendJsCharFn == null) {
        resolver.error("'append_js_char' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendJsCharPtrFn == null) {
        resolver.error("'append_js_char_ptr' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsFn = page.child("append_js");
    if(appendJsFn == null) {
        resolver.error("'append_js' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsIntFn = page.child("append_js_integer");
    if(appendJsIntFn == null) {
        resolver.error("'append_js_integer' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsUIntFn = page.child("append_js_uinteger");
    if(appendJsUIntFn == null) {
        resolver.error("'append_js_uinteger' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsFloatFn = page.child("append_js_float");
    if(appendJsFloatFn == null) {
        resolver.error("'append_js_float' function is required on 'page' for html to work", loc);
        return false;
    }

    const appendJsDoubleFn = page.child("append_js_double");
    if(appendJsDoubleFn == null) {
        resolver.error("'append_js_double' function is required on 'page' for html to work", loc);
        return false;
    }

    support.appendJsCharFn = appendJsCharFn
    support.appendJsCharPtrFn = appendJsCharPtrFn
    support.appendJsFn = appendJsFn
    support.appendJsIntFn = appendJsIntFn;
    support.appendJsUIntFn = appendJsUIntFn;
    support.appendJsFloatFn = appendJsFloatFn;
    support.appendJsDoubleFn = appendJsDoubleFn;

    const requireComponentFn = page.child("require_component")
    if(requireComponentFn == null) {
        resolver.error("'require_component' function is required on 'page' for html components to work", loc);
        return false;
    }

    const setComponentHashFn = page.child("set_component_hash")
    if(setComponentHashFn == null) {
        resolver.error("'set_component_hash' function is required on 'page' for html components to work", loc);
        return false;
    }

    support.requireComponentFn = requireComponentFn
    support.setComponentHashFn = setComponentHashFn;

    const renderJsAttrs = resolver.find("renderJsAttrs")
    if(renderJsAttrs == null) {
        resolver.error("'renderJsAttrs' function is required for universal hydration to work", loc);
        return false;
    }
    support.renderJsAttrs = renderJsAttrs;

    for (var i : uint = 0; i < root.components.size(); i += 1) {
        var element = root.components.get(i);
        
        const compNode = resolver.find(element.name);
        if (compNode == null) {
            resolver.error("component not found", element.loc);
            return false;
        }

        if (compNode.getKind() != ASTNodeKind.EmbeddedNode) {
            resolver.error("symbol is not a valid component", element.loc);
            return false;
        }

        const controller = resolver.getAnnotationController();

        if(!controller.isMarked(compNode, "component")) {
            resolver.error("symbol is not a component", element.loc);
            return false;
        }

        var embedded = compNode as *mut EmbeddedNode;
        var signature = embedded.getDataPtr() as *mut ComponentSignature;
        element.componentSignature = signature;

        for (var j : uint = 0; j < signature.params.size(); j += 1) {
            const param = signature.params.get_ptr(j);
            if (!param.is_optional) {
                var found = false;
                for (var k : uint = 0; k < element.attributes.size(); k += 1) {
                    const attr = element.attributes.get(k);
                    if (attr.name.equals(param.name)) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                     resolver.error("missing required component argument", element.loc);
                     return false;
                }
            }
        }
    }

    return true;

}
