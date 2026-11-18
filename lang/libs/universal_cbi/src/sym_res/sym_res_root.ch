// Attribute names every component accepts regardless of its declared props.
func is_implicit_jsx_prop_name(name : &std::string_view) : bool {
    if(name.equals(std::string_view("children"))) return true
    if(name.equals(std::string_view("class"))) return true
    if(name.equals(std::string_view("className"))) return true
    if(name.equals(std::string_view("key"))) return true
    if(name.equals(std::string_view("ref"))) return true
    if(name.equals(std::string_view("style"))) return true
    // Universal HTML attributes (WHATWG "global attributes"). These are what let a
    // component keep `{...props}` and still be typed: the spread forwards them, and
    // a typo in a real prop is still a compile error.
    if(name.equals(std::string_view("id"))) return true
    if(name.equals(std::string_view("title"))) return true
    if(name.equals(std::string_view("role"))) return true
    if(name.equals(std::string_view("tabindex"))) return true
    if(name.equals(std::string_view("hidden"))) return true
    if(name.equals(std::string_view("dir"))) return true
    if(name.equals(std::string_view("lang"))) return true
    if(name.equals(std::string_view("slot"))) return true
    if(name.equals(std::string_view("part"))) return true
    if(name.equals(std::string_view("contenteditable"))) return true
    if(name.equals(std::string_view("draggable"))) return true
    if(name.equals(std::string_view("spellcheck"))) return true
    if(name.equals(std::string_view("inert"))) return true
    if(name.equals(std::string_view("translate"))) return true
    if(name.equals(std::string_view("autofocus"))) return true
    // The Chemical spelling of the aria-label attribute: components that take it
    // forward it to `aria-label` explicitly, so requiring every props struct to
    // declare it again would be noise. `aria-*` is implicit for the same reason.
    if(name.equals(std::string_view("ariaLabel"))) return true
    // `data-*` / `aria-*` are the DOM's pass-through namespaces, never a prop typo.
    if(name.size() > 5 && name.at(0) == 'd' && name.at(1) == 'a' && name.at(2) == 't' && name.at(3) == 'a' && name.at(4) == '-') return true
    if(name.size() > 5 && name.at(0) == 'a' && name.at(1) == 'r' && name.at(2) == 'i' && name.at(3) == 'a' && name.at(4) == '-') return true
    return name.size() > 2 && name.at(0) == 'o' && name.at(1) == 'n'
}

// A raw JS literal inside a `#universal` body has no Chemical type, so the type
// checker cannot judge it — but a string literal assigned to an `int` prop is a
// mistake regardless. Classify the literal text and reject the cross-kind cases;
// `null`/`undefined` always mean "absent", and anything that is not a plain literal
// (a call, a member access) stays unchecked.
func literal_text_kind(
    text : &std::string_view
    // 0 = not a literal, 1 = string, 2 = number, 3 = bool, 4 = null/undefined
) : int {
    const n = text.size()
    if(n == 0) return 0
    const c0 = text.at(0)
    if(c0 == '"' || c0 == '\'') return 1
    if(text.equals(std::string_view("true")) || text.equals(std::string_view("false"))) return 3
    if(text.equals(std::string_view("null")) || text.equals(std::string_view("undefined"))) return 4
    // A number: the first char is a digit, or a sign/dot followed by a digit.
    var start : uint = 0
    if(c0 == '-' || c0 == '+' || c0 == '.') { start = 1 }
    if(start >= n) return 0
    const d = text.at(start)
    if(d >= '0' && d <= '9') return 2
    return 0
}

// The literal kinds a declared prop type can accept, as a bitmask over
// literal_text_kind: 1 = text, 2 = number, 4 = bool. 0 means "do not check".
func accepted_literal_kinds(kind : BaseTypeKind) : int {
    if(kind == BaseTypeKind.String) return 1
    if(kind == BaseTypeKind.ExpressiveString) return 1
    if(kind == BaseTypeKind.Pointer) return 1
    if(kind == BaseTypeKind.Bool) return 4
    // Numbers (IntN/Float/Double/…). Anything else is left unchecked by returning 0.
    if(kind == BaseTypeKind.IntN) return 2
    if(kind == BaseTypeKind.Float) return 2
    if(kind == BaseTypeKind.Double) return 2
    if(kind == BaseTypeKind.LongDouble) return 2
    if(kind == BaseTypeKind.Float128) return 2
    return 0
}

func literal_kind_name(k : int) : std::string_view {
    if(k == 1) return std::string_view("a string literal")
    if(k == 2) return std::string_view("a number literal")
    if(k == 3) return std::string_view("a boolean literal")
    return std::string_view("null")
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

            // A **declared** prop is always checked, even when its name is also a
            // universal attribute (`title` is both), so declaring it cannot silently
            // opt out of value checking.
            var member : *mut BaseDefMember = null
            for(var j : uint = 0; j < count; j += 1) {
                const candidate = members.get(j)
                if(candidate.getName().equals(&attr.name)) { member = candidate; break }
            }
            if(member == null && is_implicit_jsx_prop_name(&attr.name)) continue

            if(member == null) {
                var msg = std::string("unknown prop '")
                msg.append_view(&attr.name)
                msg.append_view("' for component '")
                msg.append_view(compName)
                msg.append_view("'")
                if(count > 0) {
                    msg.append_view("; declared props: ")
                    for(var j : uint = 0; j < count; j += 1) {
                        if(j > 0) { msg.append_view(", ") }
                        const candidate = members.get(j)
                        msg.append_view(&candidate.getName())
                    }
                }
                diagnoser.error(msg.to_view(), element.loc)
                *ok = false
                continue
            }

            // Declared prop: check a Chemical value against the field's declared
            // type with the type checker's own rule (same as the #html path).
            var vnode = attr.value
            if(vnode != null && vnode.kind == JsNodeKind.JSXExpressionContainer) {
                vnode = (vnode as *mut JsJSXExpressionContainer).expression
            }
            var chemValue : *mut Value = null
            if(vnode != null && vnode.kind == JsNodeKind.ChemicalValue) {
                chemValue = (vnode as *mut JsChemicalValue).value
            }
            if(chemValue == null) {
                // A raw JS literal: no Chemical type, so check its kind against the
                // declared field type instead of skipping it silently.
                if(vnode != null && vnode.kind == JsNodeKind.Literal) {
                    const fieldTypeLit = member.getType()
                    if(fieldTypeLit != null) {
                        const lit = (vnode as *mut JsLiteral).value
                        const lk = literal_text_kind(&lit)
                        if(lk != 0 && lk != 4) {
                            const accepted = accepted_literal_kinds(fieldTypeLit.getKind())
                            // 0 means "this type is not literal-checked" (structs, enums, …)
                            if(accepted != 0 && (accepted & (1 << (lk - 1))) == 0) {
                                var expectedLit : char[96]
                                const expLenLit = unsafe(fieldTypeLit.representation(unsafe(&raw mut expectedLit[0]), 96))
                                var msgLit = std::string("prop '")
                                msgLit.append_view(&attr.name)
                                msgLit.append_view("' of component '")
                                msgLit.append_view(compName)
                                msgLit.append_view("' expects '")
                                msgLit.append_with_len(unsafe(&raw mut expectedLit[0]), expLenLit)
                                msgLit.append_view("' but got ")
                                const lkn = literal_kind_name(lk)
                                msgLit.append_view(&lkn)
                                diagnoser.error(msgLit.to_view(), element.loc)
                                *ok = false
                            }
                        }
                    }
                }
                continue
            }
            const fieldType = member.getType()
            if(fieldType == null) continue
            if(fieldType.satisfies_value(chemValue, true)) continue

            var expected : char[96]
            var got : char[96]
            const expLen = unsafe(fieldType.representation(unsafe(&raw mut expected[0]), 96))
            var gotLen : ubigint = 0
            const valType = chemValue.getType()
            if(valType != null) {
                gotLen = unsafe(valType.representation(unsafe(&raw mut got[0]), 96))
            }
            var msg = std::string("prop '")
            msg.append_view(&attr.name)
            msg.append_view("' of component '")
            msg.append_view(compName)
            msg.append_view("' expects '")
            msg.append_with_len(unsafe(&raw mut expected[0]), expLen)
            msg.append_view("' but got '")
            if(gotLen == 0) {
                msg.append_view("an unknown type")
            } else {
                msg.append_with_len(unsafe(&raw mut got[0]), gotLen)
            }
            msg.append('\'')
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
            // A field with a default may be omitted: the converter injects the value
            // into both the SSR attributes and the client props payload from
            // `ResolvedAttr.defaultValue` (§3.15a).
            if(member.getDefaultValue() != null) continue
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
