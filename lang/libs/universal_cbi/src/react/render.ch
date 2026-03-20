
func render_universal_jsx(
    builder : *mut ASTBuilder,
    node : *mut JsNode,
    parentPath : std::string_view,
    states : &std::vector<UniversalStateDecl>,
    textBindings : &mut std::vector<UniversalTextBinding>,
    eventBindings : &mut std::vector<UniversalEventBinding>,
    propBindings : &mut std::vector<UniversalPropTextBinding>,
    nestedBindings : &mut std::vector<UniversalNestedBinding>,
    propsName : std::string_view,
    converter : &mut JsConverter,
    offset : &mut uint
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
            *offset += 1;
            return true;
        }
        JsNodeKind.JSXElement => {
            const element = node as *mut JsJSXElement;
            if(element.opening.tagName.kind != JsNodeKind.Identifier) return false;
            const tagName = (element.opening.tagName as *mut JsIdentifier).value;

            const myPath = build_child_path(builder, parentPath, *offset);

            if(is_native_tag(tagName)) {
                if(converter.target == BufferType.HTML) {
                    converter.str.append('<');
                    converter.str.append_view(tagName);
                    converter.str.append(' ');
                    converter.put_chain_in();

                    if(!element.opening.attributes.empty()) {
                        const attrs = converter.build_ssr_attributes(element);
                        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                        var call = builder.make_function_call_node(builder.make_identifier(std::string_view("renderHtmlAttrs"), converter.support.renderHtmlAttrs, false, location), converter.parent, location);
                        call.get_args().push(pageId);
                        call.get_args().push(builder.make_addr_of_value(attrs, location));
                        converter.vec.push(call );
                    }

                    converter.str.append('>');
                    converter.put_chain_in();

                    var childOffset = 0u;
                    for(var i : uint = 0; i < element.children.size(); i++) {
                        render_universal_jsx(builder, element.children.get(i), myPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter, childOffset);
                    }

                    converter.str.append_view("</");
                    converter.str.append_view(tagName);
                    converter.str.append('>');
                    converter.put_chain_in();
                } else {
                    converter.str.append_view("$_ur.createElement('");
                    converter.str.append_view(tagName);
                    converter.str.append_view("', {");
                    // ... props ...
                    converter.str.append_view("}");
                    var childOffset = 0u;
                    for(var i : uint = 0; i < element.children.size(); i++) {
                        converter.str.append_view(",");
                        render_universal_jsx(builder, element.children.get(i), myPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter, childOffset);
                    }
                    converter.str.append_view(")");
                }
                *offset += 1;
                return true;
            } else {
                const signature = element.componentSignature;
                if(signature != null) {
                    if(converter.target == BufferType.HTML) {
                        const attrs = converter.build_ssr_attributes(element);
                        var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                        var call = builder.make_function_call_node(builder.make_identifier(signature.name, signature.functionNode , false, location), converter.parent, location);
                        call.get_args().push(pageId);
                        call.get_args().push(builder.make_addr_of_value(attrs, location));

                        if(element.children.empty()) {
                            const ssrTextStructVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
                            ssrTextStructVal.add_value("data", builder.make_null_value(location));
                            ssrTextStructVal.add_value("size", builder.make_ubigint_value(0, location));
                            call.get_args().push(ssrTextStructVal);
                        } else {
                            var getSizeCall = builder.make_function_call_value(
                                builder.make_access_chain(std::span<*mut Value>([ pageId, builder.make_identifier(std::string_view("get_html_size"), converter.support.getHtmlSizeFn, false, location) ]), location),
                                location
                            );
                            var startIdxNameStr = std::string();
                            startIdxNameStr.append_view("startIdx_");
                            startIdxNameStr.append_integer(location as bigint);
                            var startIdxName = builder.allocate_view(startIdxNameStr.to_view());

                            var startIdxVar = builder.make_varinit_stmt(false, false, startIdxName, builder.get_u64_type(), getSizeCall, AccessSpecifier.Internal, converter.parent, location);
                            converter.vec.push(startIdxVar);

                            var childrenHtmlNameStr = std::string();
                            childrenHtmlNameStr.append_view("childrenHtml_");
                            childrenHtmlNameStr.append_integer(location as bigint);
                            var childrenHtmlName = builder.allocate_view(childrenHtmlNameStr.to_view());

                            var childrenHtmlVar = builder.make_varinit_stmt(false, false, childrenHtmlName, null,
                                builder.make_function_call_value(builder.make_identifier(view("std::string"), converter.support.stringNodeMake, false, location), location),
                                AccessSpecifier.Internal, converter.parent, location);
                            converter.vec.push(childrenHtmlVar);

                            var childOffset = 0u;
                            for(var i : uint = 0; i < element.children.size(); i++) {
                                render_universal_jsx(builder, element.children.get(i), myPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter, childOffset);
                            }

                            converter.put_chain_in();

                            var childrenHtmlId = builder.make_identifier(childrenHtmlName, childrenHtmlVar, false, location);
                            var pageHtmlId = builder.make_identifier(std::string_view("pageHtml"), converter.support.pageHtmlNode, false, location);
                            var pageHtmlAccess = builder.make_access_chain(std::span<*mut Value>([ pageId, pageHtmlId]), location);

                            var appendCall = builder.make_function_call_node(
                                builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("append_with_len"), converter.support.appendWithLenFn, false, location) ]), location),
                                converter.parent,
                                location
                            );
                            var startIdxId = builder.make_identifier(startIdxName, startIdxVar, false, location);
                            var dataCall = builder.make_function_call_value(
                                builder.make_access_chain(std::span<*mut Value>([ pageId, pageHtmlId, builder.make_identifier(view("data"), converter.support.dataFn, false, location) ]), location),
                                location
                            );
                            var sizeCall = builder.make_function_call_value(
                                builder.make_access_chain(std::span<*mut Value>([ pageId, pageHtmlId, builder.make_identifier(view("size"), converter.support.sizeFn, false, location) ]), location),
                                location
                            );

                            appendCall.get_args().push(builder.make_expression_value(dataCall, startIdxId, Operation.Addition, dataCall.getType(), location));
                            appendCall.get_args().push(builder.make_expression_value(sizeCall, startIdxId, Operation.Subtraction, sizeCall.getType(), location));
                            converter.vec.push(appendCall);

                            var truncateCall = builder.make_function_call_node(
                                builder.make_access_chain(std::span<*mut Value>([ pageId, builder.make_identifier(view("truncate_html"), converter.support.truncateHtmlFn, false, location) ]), location),
                                converter.parent,
                                location
                            );
                            truncateCall.get_args().push(startIdxId);
                            converter.vec.push(truncateCall);

                            const ssrTextStructVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
                            var dataCall2 = builder.make_function_call_value(
                                builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("data"), converter.support.dataFn, false, location) ]), location),
                                location
                            );
                            var sizeCall2 = builder.make_function_call_value(
                                builder.make_access_chain(std::span<*mut Value>([ childrenHtmlId, builder.make_identifier(view("size"), converter.support.sizeFn, false, location) ]), location),
                                location
                            );
                            ssrTextStructVal.add_value("data", dataCall2);
                            ssrTextStructVal.add_value("size", sizeCall2);
                            call.get_args().push(ssrTextStructVal);
                        }
                        converter.vec.push(call );
                    } else {
                        converter.str.append_view("$_ur.createElement(");
                        get_module_scoped_name(signature.functionNode , signature.name, converter.str);
                        converter.str.append_view(", ");
                        build_nested_props_expr(converter, element, states);
                        var childOffset = 0u;
                        for(var i : uint = 0; i < element.children.size(); i++) {
                            converter.str.append_view(",");
                            render_universal_jsx(builder, element.children.get(i), myPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter, childOffset);
                        }
                        converter.str.append_view(")");
                    }
                    if(signature.rootNodeCount > 0) {
                        *offset += signature.rootNodeCount;
                    } else {
                        *offset += 1u
                    }
                    return true;
                }
            }
            return false;
        }
        JsNodeKind.JSXFragment => {
            const frag = node as *mut JsJSXFragment;
            for(var i : uint = 0; i < frag.children.size(); i++) {
                render_universal_jsx(builder, frag.children.get(i), parentPath, states, textBindings, eventBindings, propBindings, nestedBindings, propsName, converter, offset);
            }
            return true;
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
                converter.convertJsNode(container.expression);
            }
            *offset += 1;
            return true;
        }
        default => return false;
    }
    return false;
}
