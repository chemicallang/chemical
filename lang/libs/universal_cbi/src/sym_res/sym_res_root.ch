func sym_res_components(
    components : &mut std::vector<*mut JsJSXElement>,
    resolver : *mut SymbolResolver,
    diagnoser : *mut ASTDiagnoser
) : bool {
    var ok = true;
    for (var i : uint = 0; i < components.size(); i += 1) {
        var element = components.get(i);

        var name : std::string_view;
        if(element.opening.tagName.kind == JsNodeKind.Identifier) {
             const jsId = element.opening.tagName as *mut JsIdentifier
             name = jsId.value;
        } else {
             continue;
        }

        const compNode = resolver.resolve(unsafe(&name));
        if (compNode == null) {
            var msg = std::string("<")
            msg.append_view(&name)
            msg.append_view("> is not a known component: no symbol named '")
            msg.append_view(&name)
            msg.append_view("' is in scope")
            diagnoser.error(msg.to_view(), element.loc);
            ok = false;
            continue;
        }

        if (compNode.getKind() != ASTNodeKind.EmbeddedNode) {
            var msg = std::string("'")
            msg.append_view(&name)
            msg.append_view("' is not a valid component: expected an #universal component declaration")
            diagnoser.error(msg.to_view(), element.loc);
            ok = false;
            continue;
        }

        const controller = resolver.getAnnotationController();

        if(!controller.isMarked(compNode, "component")) {
            var msg = std::string("'")
            msg.append_view(&name)
            msg.append_view("' is not a component: add the #universal annotation to declare it")
            diagnoser.error(msg.to_view(), element.loc);
            ok = false;
            continue;
        }

        var embedded = compNode as *mut EmbeddedNode;
        var comp = embedded.getDataPtr() as *mut JsComponentDecl;
        var signature = &raw mut comp.signature;
        element.componentSignature = signature;

        // Collect every missing required prop (instead of stopping at the first)
        // so a single compile reports all of them. A spread may supply any prop
        // at runtime, so its presence satisfies the requirement.
        var missing = std::vector<std::string_view>();
        for (var j : uint = 0; j < signature.params.size(); j += 1) {
            const param = signature.params.get_ptr(j);
            if (param.is_optional) continue;
            var found = false;
            for (var k : uint = 0; k < element.opening.attributes.size(); k += 1) {
                const attrNode = element.opening.attributes.get(k);
                if(attrNode.kind == JsNodeKind.JSXAttribute) {
                    const attr = attrNode as *mut JsJSXAttribute;
                    if (attr.name.equals(&param.name)) {
                        found = true;
                        break;
                    }
                } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
                    found = true;
                    break;
                }
            }
            if (!found) missing.push(param.name);
        }
        if (!missing.empty()) {
            var msg = std::string("missing required ")
            if (missing.size() == 1) {
                msg.append_view("prop '")
                msg.append_view(&missing.get(0))
                msg.append_view("'")
            } else {
                msg.append_view("props ")
                for (var m : uint = 0; m < missing.size(); m += 1) {
                    if (m > 0) msg.append_view(", ")
                    msg.append_view("'")
                    msg.append_view(&missing.get(m))
                    msg.append_view("'")
                }
            }
            msg.append_view(" on <")
            msg.append_view(&name)
            msg.append_view(">")
            diagnoser.error(msg.to_view(), element.loc);
            ok = false;
        }
    }
    return ok;
}
