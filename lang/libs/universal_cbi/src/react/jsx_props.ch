func try_build_style_object_text(
    builder : *mut ASTBuilder,
    obj : *mut JsObjectLiteral,
    outText : &mut std::string_view
) : bool {
    if(obj == null) return false;
    var s = std::string();
    for(var i : uint = 0; i < obj.properties.size(); i++) {
        const prop = obj.properties.get(i);
        if(prop.value == null) return false;
        if(prop.value.kind != JsNodeKind.Literal) return false;
        const lit = prop.value as *mut JsLiteral;
        const valText = strip_js_string_quotes(lit.value);
        if(s.size() > 0) s.append_view("; ");
        if(!append_css_key(s, prop.key)) return false;
        s.append_view(": ");
        s.append_view(valText);
    }
    outText = builder.allocate_view(s.to_view());
    return true;
}

func find_state_init_text(states : &std::vector<UniversalStateDecl>, name : std::string_view) : std::string_view {
    for(var i : uint = 0; i < states.size(); i++) {
        const st = states.get(i);
        if(st.name.equals(name)) {
            return st.initText;
        }
    }
    return view("");
}

func has_state(states : &std::vector<UniversalStateDecl>, name : std::string_view) : bool {
    for(var i : uint = 0; i < states.size(); i++) {
        if(states.get(i).name.equals(name)) return true;
    }
    return false;
}

func get_prop_access_path(builder : *mut ASTBuilder, node : *mut JsNode, propsName : std::string_view) : std::string_view {
    if(node == null) return view("");
    if(node.kind == JsNodeKind.Identifier) {
        const id = node as *mut JsIdentifier;
        if(id.value.equals(propsName)) return view("");
    } else if(node.kind == JsNodeKind.MemberAccess) {
        const ma = node as *mut JsMemberAccess;
        const sub = get_prop_access_path(builder, ma.object, propsName);
        if(!sub.empty() || (ma.object.kind == JsNodeKind.Identifier && (ma.object as *mut JsIdentifier).value.equals(propsName))) {
            var path = std::string();
            if(!sub.empty()) {
                path.append_view(sub);
                path.append('.');
            }
            path.append_view(ma.property);
            return builder.allocate_view(path.to_view());
        }
    }
    return view("");
}

func find_jsx_attribute(element : *mut JsJSXElement, name : std::string_view) : *mut JsJSXAttribute {
    if(element == null) return null;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i);
        if(attrNode == null || attrNode.kind != JsNodeKind.JSXAttribute) continue;
        const attr = attrNode as *mut JsJSXAttribute;
        if(attr.name.equals(name)) return attr;
    }
    return null;
}

func resolve_nested_prop_as_text(
    builder : *mut ASTBuilder,
    element : *mut JsJSXElement,
    propPath : std::string_view,
    propsName : std::string_view,
    states : &std::vector<UniversalStateDecl>,
    outText : &mut std::string_view,
    outPropAccess : &mut std::string_view,
    outChem : &mut *mut Value
) : int {
    if(propPath.empty()) return 0;
    var dotPos : uint = 0;
    while(dotPos < propPath.size() && propPath.data()[dotPos] != '.') dotPos++;
    const attrName = if(dotPos == propPath.size()) propPath else std::string_view(propPath.data(), dotPos);
    const attr = find_jsx_attribute(element, attrName);
    if(attr == null || attr.value == null) return 0;
    if(attr.value.kind == JsNodeKind.Literal) {
        const lit = attr.value as *mut JsLiteral;
        outText = strip_js_string_quotes(lit.value);
        return 1;
    }
    if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
        const container = attr.value as *mut JsJSXExpressionContainer;
        const expr = container.expression;
        if(expr == null) return 0;
        if(expr.kind == JsNodeKind.Literal) {
            const lit = expr as *mut JsLiteral;
            outText = strip_js_string_quotes(lit.value);
            return 1;
        }
        if(expr.kind == JsNodeKind.Identifier) {
            const id = expr as *mut JsIdentifier;
            if(has_state(states, id.value)) {
                outText = find_state_init_text(states, id.value);
                return 1;
            }
        }
        if(expr.kind == JsNodeKind.MemberAccess) {
            const p = get_prop_access_path(builder, expr, propsName);
            if(!p.empty()) {
                outPropAccess = p;
                return 2;
            }
        }
        if(expr.kind == JsNodeKind.ChemicalValue) {
            const cv = expr as *mut JsChemicalValue;
            outChem = cv.value;
            return 3;
        }
    }
    return 0;
}

func build_nested_props_expr(
    converter : &mut JsConverter,
    element : *mut JsJSXElement,
    states : &std::vector<UniversalStateDecl>
) {
    if(element == null) {
        converter.str.append_view("{}");
        return;
    }
    const builder = converter.builder;
    var s = &mut converter.str;
    s.append_view("{");
    var first = true;
    for(var i : uint = 0; i < element.opening.attributes.size(); i++) {
        const attrNode = element.opening.attributes.get(i);
        if(attrNode == null) continue;
        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            const attr = attrNode as *mut JsJSXAttribute;
            if(!first) s.append_view(",");
            first = false;
            s.append_view(attr.name);
            s.append_view(":");
            if(attr.value == null) {
                s.append_view("true");
            } else if(attr.value.kind == JsNodeKind.Literal) {
                const lit = attr.value as *mut JsLiteral;
                s.append_view(lit.value);
            } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                const container = attr.value as *mut JsJSXExpressionContainer;
                const expr = container.expression;
                if(expr != null) {
                    converter.convert_jsx_runtime_expr(expr);
                } else {
                    s.append_view("true");
                }
            } else {
                s.append_view("true");
            }
        } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute) {
            const spread = attrNode as *mut JsJSXSpreadAttribute;
            if(!first) s.append_view(",");
            first = false;
            s.append_view("...");
            converter.convertJsNode(spread.argument);
        }
    }
    s.append_view("}");
}
