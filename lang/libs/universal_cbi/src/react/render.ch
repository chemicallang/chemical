
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
            if(element.opening.tagName.kind != JsNodeKind.Identifier) return false;
            const tagName = (element.opening.tagName as *mut JsIdentifier).value;

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
