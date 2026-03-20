func (converter : &mut JsConverter) make_ssr_text(val : &std::string_view, location : ubigint) : *mut Value {
    return make_ssr_text_val(converter.builder, val, converter.support.ssrTextLinkedNode, location);
}

func strip_js_string_quotes(value : std::string_view) : std::string_view {
    if(value.size() >= 2) {
        const first = value.get(0);
        const last = value.get(value.size() - 1);
        if((first == '"' || first == '\'' || first == '`') && first == last) {
            return value.subview(1, value.size() - 1);
        }
    }
    return value;
}

func (converter : &mut JsConverter) convert_js_literal_to_ssr_value(lit : *mut JsLiteral, attrValConv : &mut AttrValueConverter, location : ubigint) : *mut Value {
    const val = lit.value;
    const builder = converter.builder;
    if(val.equals("true")) return attrValConv.wrapArgAttrValueVariantCall(builder, "Boolean", builder.make_bool_value(true, location));
    if(val.equals("false")) return attrValConv.wrapArgAttrValueVariantCall(builder, "Boolean", builder.make_bool_value(false, location));

    const text = strip_js_string_quotes(val);
    return attrValConv.wrapArgAttrValueVariantCall(builder, "Text", converter.make_ssr_text(text, location));
}

func append_js_node_text(node : *mut JsNode, out : &mut std::string) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Literal => {
            out.append_view((node as *mut JsLiteral).value);
            return true;
        }
        JsNodeKind.Identifier => {
            out.append_view((node as *mut JsIdentifier).value);
            return true;
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(!append_js_node_text(mem.object, out)) return false;
            out.append('.');
            out.append_view(mem.property);
            return true;
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            if(!append_js_node_text(idx.object, out)) return false;
            out.append('[');
            if(!append_js_node_text(idx.index, out)) return false;
            out.append(']');
            return true;
        }
        JsNodeKind.UnaryOp => {
            const unary = node as *mut JsUnaryOp;
            if(unary.prefix) {
                out.append_view(unary.operator);
                return append_js_node_text(unary.operand, out);
            }
            if(!append_js_node_text(unary.operand, out)) return false;
            out.append_view(unary.operator);
            return true;
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            if(!append_js_node_text(bin.left, out)) return false;
            out.append(' ');
            out.append_view(bin.op);
            out.append(' ');
            return append_js_node_text(bin.right, out);
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            if(!append_js_node_text(tern.condition, out)) return false;
            out.append_view(" ? ");
            if(!append_js_node_text(tern.consequent, out)) return false;
            out.append_view(" : ");
            return append_js_node_text(tern.alternate, out);
        }
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall;
            if(!append_js_node_text(call.callee, out)) return false;
            out.append('(');
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(i > 0) out.append_view(", ");
                if(!append_js_node_text(call.args.get(i), out)) return false;
            }
            out.append(')');
            return true;
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            const arr = node as *mut JsArrayLiteral;
            out.append('[');
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(i > 0) out.append_view(", ");
                const elem = arr.elements.get(i);
                if(elem != null && !append_js_node_text(elem, out)) return false;
            }
            out.append(']');
            return true;
        }
        JsNodeKind.ObjectLiteral => {
            const obj = node as *mut JsObjectLiteral;
            out.append('{');
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(i > 0) out.append_view(", ");
                const prop = obj.properties.get(i);
                out.append_view(prop.key);
                out.append_view(": ");
                if(!append_js_node_text(prop.value, out)) return false;
            }
            out.append('}');
            return true;
        }
        default => return false
    }
}

func build_js_node_text_view(builder : *mut ASTBuilder, node : *mut JsNode) : std::string_view {
    var text = std::string();
    if(!append_js_node_text(node, text)) return std::string_view();
    return builder.allocate_view(text.to_view());
}

func is_event_attribute_name(name : std::string_view) : bool {
    return name.size() > 2 && name.get(0) == 'o' && name.get(1) == 'n';
}

func is_non_ssr_expression(expr : *mut JsNode) : bool {
    if(expr == null) return false;
    switch(expr.kind) {
        JsNodeKind.ArrowFunction, JsNodeKind.FunctionDecl => return true
        JsNodeKind.ChemicalValue => {
            const chem = expr as *mut JsChemicalValue;
            return chem.value != null && chem.value.getKind() == ValueKind.LambdaFunc;
        }
        default => return false
    }
}

func (converter : &mut JsConverter) build_ssr_attributes(element : *mut JsJSXElement) : *mut Value {
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    const listStruct = builder.make_struct_value(support.ssrAttributeListNode, location);

    if(element.opening.attributes.empty()) {

        listStruct.add_value(std::string_view("data"), builder.make_null_value(location));
        listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(0, location));

    } else {
        const ssrAttrLinkedType = builder.make_linked_type(std::string_view("SsrAttribute"), support.ssrAttrLinkedNode, location);
        const arrayValue = builder.make_array_value(ssrAttrLinkedType, location);
        const attrValues = arrayValue.get_values();

        var attrValConv = AttrValueConverter {
            pageNode : support.pageNode,
            ssrTextNode : support.ssrTextLinkedNode,
            ssrAttributeValueNode : support.ssrAttributeValueNode,
            multipleAttributeValueNode : support.multipleAttributeValueNode,
            parent : converter.parent
        }

        const attributes = &element.opening.attributes;
        var pushedCount : ubigint = 0;
        for(var i : uint = 0; i < attributes.size(); i++) {
            const attrNode = attributes.get(i);
            if(attrNode == null) continue;

            if(attrNode.kind == JsNodeKind.JSXAttribute) {

                const attr = attrNode as *mut JsJSXAttribute;
                if(is_event_attribute_name(attr.name)) continue;
                var skipAttr = false;

                if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const container = attr.value as *mut JsJSXExpressionContainer;
                    if(is_non_ssr_expression(container.expression)) continue;
                } else if(is_non_ssr_expression(attr.value)) {
                    continue;
                }

                const attrStructVal = builder.make_struct_value(support.ssrAttrLinkedNode, location);
                const isClass = attr.name.equals("className") || attr.name.equals("class")
                const attrName = if(isClass) std::string_view("class") else attr.name;
                attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text(attrName, location));

                if(attr.value == null) {
                    const boolVal = builder.make_bool_value(true, location);
                    attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                } else if(attr.value.kind == JsNodeKind.Literal) {
                    attrStructVal.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(attr.value as *mut JsLiteral, attrValConv, location));
                } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const container = attr.value as *mut JsJSXExpressionContainer;
                    if(container.expression != null) {
                        if(container.expression.kind == JsNodeKind.ChemicalValue) {
                            const chem = container.expression as *mut JsChemicalValue;
                            attrStructVal.add_value(std::string_view("value"), attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value));
                        } else if(container.expression.kind == JsNodeKind.Literal) {
                            attrStructVal.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(container.expression as *mut JsLiteral, attrValConv, location));
                        } else if(container.expression.kind == JsNodeKind.ObjectLiteral) {
                            var objText = std::string_view();
                            if(attr.name.equals("style")) {
                                objText = build_js_node_text_view_style_attr(builder, container.expression as *mut JsObjectLiteral)
                            } else {
                                objText = build_js_node_text_view(builder, container.expression)
                            }
                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(objText, location)));
                        } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                            const mem = container.expression as *mut JsMemberAccess;
                            if(mem.object.kind == JsNodeKind.Identifier && (mem.object as *mut JsIdentifier).value.equals("props")) {
                                skipAttr = true;
                            }
                            if(!skipAttr) {
                                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text("null", location)));
                            }
                        } else {
                            // Fallback for other expressions to avoid "value not given" error.
                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text("null", location)));
                        }
                    } else {
                        const boolVal = builder.make_bool_value(true, location);
                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                    }
                }
                if(skipAttr) continue;
                attrValues.push(attrStructVal);
                pushedCount++;
            } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute){
                // TODO: runtime argument unhandled
            }
        }

        listStruct.add_value(std::string_view("data"), arrayValue);
        listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(pushedCount, location));
    }

    return listStruct as *mut Value;
}
