// Attribute names every component accepts regardless of its declared props.
func is_implicit_jsx_prop_name(name : &std::string_view) : bool {
    if(name.equals(std::string_view("children"))) return true
    if(name.equals(std::string_view("class"))) return true
    if(name.equals(std::string_view("className"))) return true
    if(name.equals(std::string_view("key"))) return true
    if(name.equals(std::string_view("ref"))) return true
    if(name.equals(std::string_view("style"))) return true
    return name.size() > 2 && name.at(0) == 'o' && name.at(1) == 'n'
}

// Typed props, JSX path: reject attributes the component's props struct does not
// declare. The struct is resolved from `propsTypeName` here rather than read from
// `signature.propsStruct`, because a call site may be visited before the
// declaring component's own symres pass has run.
func append_unknown_prop_errors(resolver : *mut SymbolResolver, diagnoser : *mut ASTDiagnoser, signature : *mut ComponentSignature, element : *mut JsJSXElement, compName : &std::string_view, ok : &mut bool) {
        unsafe {
        if(signature.propsTypeName.size() == 0) return
        var propsTypeName = signature.propsTypeName
        const propsNode = resolver.resolve(&propsTypeName)
        if(propsNode == null) {
            var msg = std::string("props type '")
            msg.append_view(&propsTypeName)
            msg.append_view("' of component '")
            msg.append_view(compName)
            msg.append_view("' was not found")
            diagnoser.error(msg.to_view(), element.loc)
            *ok = false
            return
        }
        if(propsNode.getKind() != ASTNodeKind.StructDecl) {
            var msg = std::string("props type '")
            msg.append_view(&propsTypeName)
            msg.append_view("' of component '")
            msg.append_view(compName)
            msg.append_view("' must be a struct")
            diagnoser.error(msg.to_view(), element.loc)
            *ok = false
            return
        }
        const propsStruct = propsNode as *mut StructDefinition
        signature.propsStruct = propsStruct
        const members = propsStruct.getMembers()
        if(members == null) return
        const count : uint = members.size()

        for(var i : uint = 0; i < element.opening.attributes.size(); i += 1) {
            const attrNode = element.opening.attributes.at(i)
            if(attrNode.kind != JsNodeKind.JSXAttribute) continue
            const attr = attrNode as *mut JsJSXAttribute
            if(is_implicit_jsx_prop_name(&attr.name)) continue
            var found : bool = false
            for(var j : uint = 0; j < count; j += 1) {
                const member = members.get(j)
                if(member.getName().equals(&attr.name)) { found = true; break }
            }
            if(found) continue

            var msg = std::string("unknown prop '")
            msg.append_view(&attr.name)
            msg.append_view("' for component '")
            msg.append_view(compName)
            msg.append_view("'")
            if(count > 0) {
                msg.append_view("; declared props: ")
                for(var j : uint = 0; j < count; j += 1) {
                    if(j > 0) { msg.append_view(", ") }
                    const member = members.get(j)
                    msg.append_view(&member.getName())
                }
            }
            diagnoser.error(msg.to_view(), element.loc)
            *ok = false
        }

        // Required props for typed components: a field with no initializer must be
        // supplied. A spread may supply any prop at runtime, so its presence
        // satisfies them all (same rule as the legacy required-prop check above).
        var spread : bool = false
        for(var i : uint = 0; i < element.opening.attributes.size(); i += 1) {
            if(element.opening.attributes.at(i).kind == JsNodeKind.JSXSpreadAttribute) { spread = true; break }
        }
        if(spread) return

        for(var j : uint = 0; j < count; j += 1) {
            const member = members.get(j)
            // v1: defaults are rejected rather than half-applied (SSR attrs and the
            // client props payload would have to agree).
            if(member.getDefaultValue() != null) {
                var dmsg = std::string("props field '")
                dmsg.append_view(&member.getName())
                dmsg.append_view("' of component '")
                dmsg.append_view(compName)
                dmsg.append_view("' has a default value; v1 requires every declared prop to be supplied at the call site")
                diagnoser.error(dmsg.to_view(), element.loc)
                *ok = false
                continue
            }
            var present : bool = false
            for(var i : uint = 0; i < element.opening.attributes.size(); i += 1) {
                const attrNode = element.opening.attributes.at(i)
                if(attrNode.kind != JsNodeKind.JSXAttribute) continue
                const attr = attrNode as *mut JsJSXAttribute
                if(attr.name.equals(&member.getName())) { present = true; break }
            }
            if(present) continue
            var msg = std::string("missing required prop '")
            msg.append_view(&member.getName())
            msg.append_view("' for component '")
            msg.append_view(compName)
            msg.append_view("'")
            diagnoser.error(msg.to_view(), element.loc)
            *ok = false
        }

        }
}

func sym_res_components(
    components : &mut std::vector<*mut JsJSXElement>,
    resolver : *mut SymbolResolver,
    diagnoser : *mut ASTDiagnoser
) : bool {
        unsafe {
        var ok = true;
        for (var i : uint = 0; i < components.size(); i += 1) {
            var element = components.at(i);

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
                    const attrNode = element.opening.attributes.at(k);
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
            if (!missing.empty()) {                var msg = std::string("missing required ")
                if (missing.size() == 1) {
                    msg.append_view("prop '")
                    msg.append_view(&missing.at(0))
                    msg.append_view("'")
                } else {
                    msg.append_view("props ")
                    for (var m : uint = 0; m < missing.size(); m += 1) {
                        if (m > 0) msg.append_view(", ")
                        msg.append_view("'")
                        msg.append_view(&missing.at(m))
                        msg.append_view("'")
                    }
                }
                msg.append_view(" on <")
                msg.append_view(&name)
                msg.append_view(">")
                diagnoser.error(msg.to_view(), element.loc);
                ok = false;
            }

            // Typed props: an attribute the component's props struct does not
            // declare is an error here (JSX path), matching the #html path.
            append_unknown_prop_errors(resolver, diagnoser, signature, element, &name, &mut ok);        }
        return ok;

        }
    }
