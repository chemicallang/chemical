func (converter : &mut JsConverter) make_ssr_text(val : std::string_view, location : ubigint) : *mut Value {
    const builder = converter.builder;
    const ssrTextLinkedType = builder.make_linked_type(std::string_view("SsrText"), converter.support.ssrTextLinkedNode, location);
    const structVal = builder.make_struct_value(ssrTextLinkedType, converter.parent, location);
    structVal.add_value(std::string_view("data"), builder.make_string_value(val, location));
    structVal.add_value(std::string_view("size"), builder.make_ubigint_value(val.size(), location));
    return structVal as *mut Value;
}

func (converter : &mut JsConverter) build_ssr_attributes(element : *mut JsJSXElement) : *mut Value {
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    const ssrAttrLinkedType = builder.make_linked_type(std::string_view("SsrAttribute"), support.ssrAttrLinkedNode, location);
    const arrayValue = builder.make_array_value(ssrAttrLinkedType, location);
    const attrValues = arrayValue.get_values();

    var attrValConv = AttrValueConverter {
        pageNode : support.pageNode,
        ssrAttributeValueNode : support.ssrAttributeValueNode,
        multipleAttributeValueNode : support.multipleAttributeValueNode,
        parent : converter.parent
    }

    const attributes = &element.opening.attributes;
    for(var i : uint = 0; i < attributes.size(); i++) {
        const attrNode = attributes.get(i);
        if(attrNode == null) continue;

        const attrStructVal = builder.make_struct_value(ssrAttrLinkedType, converter.parent, location);

        if(attrNode.kind == JsNodeKind.JSXAttribute) {
            const attr = attrNode as *mut JsJSXAttribute;
            attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text(attr.name, location));

            if(attr.value == null) {
                const boolVal = builder.make_bool_value(true, location);
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal as *mut Value));
            } else if(attr.value.kind == JsNodeKind.Literal) {
                const lit = attr.value as *mut JsLiteral;
                const text = strip_js_string_quotes(lit.value);
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(text, location)));
            } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                const container = attr.value as *mut JsJSXExpressionContainer;
                if(container.expression != null) {
                    if(container.expression.kind == JsNodeKind.ChemicalValue) {
                        const chem = container.expression as *mut JsChemicalValue;
                        attrStructVal.add_value(std::string_view("value"), attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value));
                    }
                }
            }
        }
        attrValues.push(attrStructVal as *mut Value);
    }

    const ssrAttributeListType = builder.make_linked_type(std::string_view("SsrAttributeList"), support.ssrAttributeListNode, location);
    const listStruct = builder.make_struct_value(ssrAttributeListType, converter.parent, location);
    listStruct.add_value(std::string_view("data"), arrayValue as *mut Value);
    listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(attributes.size(), location));
    return listStruct as *mut Value;
}

func render_universal_jsx(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    path : std::string_view,
    states : &std::vector<UniversalStateDecl>,
    textBindings : &mut std::vector<UniversalTextBinding>,
    eventBindings : &mut std::vector<UniversalEventBinding>,
    propBindings : &mut std::vector<UniversalPropTextBinding>,
    nestedBindings : &mut std::vector<UniversalNestedBinding>,
    propsName : std::string_view,
    converter : &mut JsConverter
) : bool {
    if(node == null) return true;
    const location = intrinsics::get_raw_location();

    switch(node.kind) {
        JsNodeKind.JSXText => {
            const text = node as *mut JsJSXText;
            if(converter.target == BufferType.HTML) {
                converter.escapeHtml(text.value);
            } else {
                converter.str.append_view(text.value);
            }
            return true;
        }
        JsNodeKind.JSXElement => {
            const element = node as *mut JsJSXElement;
            if(element.opening.name.kind != JsNodeKind.Identifier) return false;
            const tagName = (element.opening.name as *mut JsIdentifier).value;

            if(is_native_tag(tagName)) {
                if(converter.target == BufferType.HTML) {
                    converter.str.append('<');
                    converter.str.append_view(tagName);
                    converter.str.append(' ');
                    converter.put_chain_in();

                    const attrs = converter.build_ssr_attributes(element);
                    var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                    var call = builder.make_function_call_node(builder.make_identifier(std::string_view("renderHtmlAttrs"), converter.support.renderHtmlAttrs, false, location), converter.parent, location);
                    call.get_args().push(pageId as *mut Value);
                    call.get_args().push(attrs);
                    converter.vec.push(call as *mut ASTNode);

                    converter.str.append('>');
                    converter.put_chain_in();

                    for(var i : uint = 0; i < element.children.size(); i++) {
                        const childPath = build_child_path(builder, path, i);
                        render_universal_jsx(builder, element.children.get(i), childPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter);
                    }

                    converter.str.append_view("</");
                    converter.str.append_view(tagName);
                    converter.str.append('>');
                    converter.put_chain_in();
                    return true;
                } else {
                    converter.str.append_view("$_ur.createElement('");
                    converter.str.append_view(tagName);
                    converter.str.append_view("', {");
                    // ... props ...
                    converter.str.append_view("}");
                    for(var i : uint = 0; i < element.children.size(); i++) {
                        converter.str.append_view(",");
                        render_universal_jsx(builder, element.children.get(i), path, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter);
                    }
                    converter.str.append_view(")");
                    return true;
                }
            } else {
                const signature = element.componentSignature;
                if(signature != null) {
                    if(converter.target == BufferType.HTML) {
                        const attrs = converter.build_ssr_attributes(element);
                        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                        var call = builder.make_function_call_node(builder.make_identifier(signature.name, signature.functionNode as *mut ASTNode, false, location), converter.parent, location);
                        call.get_args().push(pageId as *mut Value);
                        call.get_args().push(attrs);
                        converter.vec.push(call as *mut ASTNode);
                    } else {
                        converter.str.append_view("$_ur.createElement(");
                        get_module_scoped_name(signature.functionNode as *mut ASTNode, signature.name, converter.str);
                        converter.str.append_view(", ");
                        converter.str.append_view(build_nested_props_expr(builder, element, states, converter.support));
                        converter.str.append_view(")");
                    }
                    return true;
                }
            }
            return false;
        }
        JsNodeKind.JSXExpressionContainer => {
            const container = node as *mut JsJSXExpressionContainer;
            if(container.expression == null) return true;
            if(converter.target == BufferType.HTML) {
                if(container.expression.kind == JsNodeKind.ChemicalValue) {
                    const chem = container.expression as *mut JsChemicalValue;
                    converter.put_chain_in();
                    converter.put_chemical_value_in(chem.value);
                }
            } else {
                converter.str.append_view(js_node_to_source(builder, container.expression, states, converter.support));
            }
            return true;
        }
        default => return false;
    }
    return false;
}
