func sym_res_components(
    components : &mut std::vector<*mut JsJSXElement>,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {
    for (var i : uint = 0; i < components.size(); i += 1) {
        var element = components.get(i);

        var name : std::string_view;
        if(element.opening.tagName.kind == JsNodeKind.Identifier) {
             const jsId = element.opening.tagName as *mut JsIdentifier
             name = jsId.value;
        } else {
             continue;
        }

        const compNode = resolver.find(name);
        if (compNode == null) {
            resolver.error("component not found", loc);
            return false;
        }

        if (compNode.getKind() != ASTNodeKind.EmbeddedNode) {
            resolver.error("symbol is not a valid component", loc);
            return false;
        }

        const controller = resolver.getAnnotationController();

        if(!controller.isMarked(compNode, "component")) {
            resolver.error("symbol is not a component", loc);
            return false;
        }

        var embedded = compNode as *mut EmbeddedNode;
        var signature = embedded.getDataPtr() as *mut ComponentSignature;
        element.componentSignature = signature;

        for (var j : uint = 0; j < signature.params.size(); j += 1) {
            const param = signature.params.get_ptr(j);
            if (!param.is_optional) {
                var found = false;
                for (var k : uint = 0; k < element.opening.attributes.size(); k += 1) {
                    const attrNode = element.opening.attributes.get(k);
                    if(attrNode.kind == JsNodeKind.JSXAttribute) {
                        const attr = attrNode as *mut JsJSXAttribute;
                        if (attr.name.equals(param.name)) {
                            found = true;
                            break;
                        }
                    } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                         // We can't verify spread attributes statically easily, assume it provides?
                         // Or strict mode?
                         // For now, let's assume spread *might* provide it, so we can't fail?
                         // Or just ignore spread and enforce explicit props?
                         // React usually allows spread to pass anything.
                         // But for strict checking...
                         // Let's assume if there is a spread, we assume it's valid?
                         // Or we just don't check spread.
                         found = true; // Bail out check if spread is present?
                         break;
                    }
                }
                if (!found) {
                     resolver.error("missing required component argument", loc);
                     return false;
                }
            }
        }
    }
    return true;
}

func sym_res_root(
    root : *mut JsRoot,
    resolver : *mut SymbolResolver,
    loc : ubigint
) : bool {

    const page = resolver.find("page");
    if(page == null) {
        resolver.error("couldn't find page variable", loc);
        return false;
    }

    const appendJsCharFn = page.child("append_js_char")
    if(appendJsCharFn == null) {
        resolver.error("'append_js_char' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsCharPtrFn = page.child("append_js_char_ptr")
    if(appendJsCharPtrFn == null) {
        resolver.error("'append_js_char_ptr' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsFn = page.child("append_js");
    if(appendJsFn == null) {
        resolver.error("'append_js' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsIntFn = page.child("append_js_integer");
    if(appendJsIntFn == null) {
        resolver.error("'append_js_integer' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsUIntFn = page.child("append_js_uinteger");
    if(appendJsUIntFn == null) {
        resolver.error("'append_js_uinteger' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsFloatFn = page.child("append_js_float");
    if(appendJsFloatFn == null) {
        resolver.error("'append_js_float' function is required on 'page' for js to work", loc);
        return false;
    }

    const appendJsDoubleFn = page.child("append_js_double");
    if(appendJsDoubleFn == null) {
        resolver.error("'append_js_double' function is required on 'page' for js to work", loc);
        return false;
    }

    var support = &mut root.support
    support.pageNode = page;
    support.appendJsCharFn = appendJsCharFn
    support.appendJsCharPtrFn = appendJsCharPtrFn
    support.appendJsFn = appendJsFn
    support.appendJsIntFn = appendJsIntFn;
    support.appendJsUIntFn = appendJsUIntFn;
    support.appendJsFloatFn = appendJsFloatFn;
    support.appendJsDoubleFn = appendJsDoubleFn;

    if(!sym_res_components(root.components, resolver, loc)) {
        return false;
    }

    return true;

}
