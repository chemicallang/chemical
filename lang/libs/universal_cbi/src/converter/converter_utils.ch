func (converter : &mut JsConverter) append_hex(val : uint) {
    const hex = "0123456789ABCDEF"
    if (val == 0) {
        converter.str.append('0');
        return;
    }
    var buf : [16]char;
    var bi = 0;
    while(val > 0) {
        buf[bi++] = hex[val & 0xF]
        val >>= 4;
    }
    while(bi > 0) {
        converter.str.append(buf[--bi])
    }
}

func (converter : &mut JsConverter) escapeJs(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            str.append(c1 as char);
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const codepoint = ((c1 & 0x1F as uint) << 6u) | (c2 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & 0x0F as uint) << 12u) | ((c2 & 0x3F as uint) << 6u) | (c3 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 3;
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const c4 = (text.data()[i+3] as uint) & 0xFF;
                const codepoint = ((c1 & 0x07 as uint) << 18u) | ((c2 & 0x3F as uint) << 12u) | ((c3 & 0x3F as uint) << 6u) | (c4 & 0x3F as uint);
                str.append_view("\\u{");
                converter.append_hex(codepoint);
                str.append('}');
                i += 4;
            } else { i++; }
        } else {
            i++;
        }
    }
}

func (converter : &mut JsConverter) next_t() : std::string {
    converter.t_counter++
    var res = std::string()
    res.append_view("$c_t")
    res.append_integer(converter.t_counter as bigint)
    return res
}

func (converter : &mut JsConverter) is_state_var(name : std::string_view) : bool {
    for(var i : uint = 0; i < converter.state_vars.size(); i++) {
        if(converter.state_vars.get(i).equals(name)) {
            return true;
        }
    }
    return false;
}

func (converter : &mut JsConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_set_component_hash_call(hash : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    return converter.make_value_call_with(value, std::string_view("set_component_hash"), converter.support.setComponentHashFn, converter.support.setComponentHashFn)
}


func (converter : &mut JsConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, jsFnPtr : *mut ASTNode, htmlFnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);

    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = fn_name
        fnPtr = jsFnPtr
    } else {
        // Map append_js_... to append_html_...
        if(fn_name.equals(view("append_js_char_ptr"))) name = view("append_html_char_ptr");
        else if(fn_name.equals(view("append_js_char"))) name = view("append_html_char");
        else if(fn_name.equals(view("append_js_integer"))) name = view("append_html_integer");
        else if(fn_name.equals(view("append_js_uinteger"))) name = view("append_html_uinteger");
        else if(fn_name.equals(view("append_js_float"))) name = view("append_html_float");
        else if(fn_name.equals(view("append_js_double"))) name = view("append_html_double");
        else name = fn_name; // fallback

        fnPtr = htmlFnPtr
    }

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendHeadJsCharPtrFn, converter.support.appendHtmlCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char"), converter.support.appendHeadJsCharFn, converter.support.appendHtmlCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendHeadJsIntFn, converter.support.appendHtmlIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendHeadJsUIntFn, converter.support.appendHtmlUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_float"), converter.support.appendHeadJsFloatFn, converter.support.appendHtmlFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_double"), converter.support.appendHeadJsDoubleFn, converter.support.appendHtmlDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);

    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = std::string_view("append_js")
        fnPtr = converter.support.appendHeadJsFn
    } else {
        name = std::string_view("append_html")
        fnPtr = converter.support.appendHtmlFn
    }

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)

    // len arg
    const len_val = builder.make_ubigint_value(len as ubigint, location);
    args.push(len_val)

    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;

    const location = intrinsics::get_raw_location();
    const str_view = converter.builder.allocate_view(converter.str.to_view());
    const val = converter.builder.make_string_value(str_view, location);
    const call = converter.make_value_call(val, converter.str.size());
    converter.vec.push(call as *mut ASTNode);
    converter.str.clear();
}

func (converter : &mut JsConverter) put_char_chain(value : char) {
    const location = intrinsics::get_raw_location();
    var base = converter.builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);

    var name : std::string_view
    var fnPtr : *mut ASTNode
    if(converter.target == BufferType.JavaScript) {
        name = std::string_view("append_js_char")
        fnPtr = converter.support.appendHeadJsCharFn
    } else {
        name = std::string_view("append_html_char")
        fnPtr = converter.support.appendHtmlCharFn
    }

    var id = converter.builder.make_identifier(name, fnPtr, false, location);
    const chain = converter.builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = converter.builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = converter.builder.make_char_value(value, location);
    args.push(char_val)
    converter.vec.push(call as *mut ASTNode)
}

func (converter : &mut JsConverter) is_html_entity(text : std::string_view, index : uint) : bool {
    if (index + 2 >= text.size()) return false;
    if (text.data()[index] != '&') return false;

    var i = index + 1;
    if (text.data()[i] == '#') {
        i++;
        if (i < text.size() && (text.data()[i] == 'x' || text.data()[i] == 'X')) {
            i++;
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) i++;
                else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        } else {
            var start = i;
            while (i < text.size() && i - start < 8) {
                const c = text.data()[i];
                if (c >= '0' && c <= '9') i++;
                else break;
            }
            return (i > start && i < text.size() && text.data()[i] == ';');
        }
    } else {
        var start = i;
        while (i < text.size() && i - start < 32) {
            const c = text.data()[i];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) i++;
            else break;
        }
        return (i > start && i < text.size() && text.data()[i] == ';');
    }
}

func (converter : &mut JsConverter) escapeHtml(text : std::string_view) {
    var i = 0u;
    var str = &mut converter.str
    while(i < text.size()) {
        const c1 = (text.data()[i] as uint) & 0xFF;
        if (c1 < 0x80) {
            const c = c1 as char;
            switch(c) {
                '&' => {
                    if (converter.is_html_entity(text, i)) str.append('&');
                    else str.append_view("&amp;");
                }
                '<' => str.append_view("&lt;")
                '>' => str.append_view("&gt;")
                '"' => str.append_view("&quot;")
                '\'' => str.append_view("&#39;")
                default => str.append(c)
            }
            i++;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 2;
            } else { i++; }
        } else if ((c1 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const codepoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 3;
            } else { i++; }
        } else if ((c1 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                const c2 = (text.data()[i+1] as uint) & 0xFF;
                const c3 = (text.data()[i+2] as uint) & 0xFF;
                const c4 = (text.data()[i+3] as uint) & 0xFF;
                const codepoint = ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
                str.append_view("&#");
                str.append_uinteger(codepoint as ubigint);
                str.append(';');
                i += 4;
            } else { i++; }
        } else { i++; }
    }
}

func (converter : &mut JsConverter) make_ssr_text(val : std::string_view, location : ubigint) : *mut Value {
    const builder = converter.builder;
    const structVal = builder.make_struct_value(converter.support.ssrTextLinkedNode, location);
    structVal.add_value(std::string_view("data"), builder.make_string_value(val, location));
    structVal.add_value(std::string_view("size"), builder.make_ubigint_value(val.size(), location));
    return structVal as *mut Value;
}

func (converter : &mut JsConverter) convert_js_literal_to_ssr_value(lit : *mut JsLiteral, attrValConv : &mut AttrValueConverter, location : ubigint) : *mut Value {
    const val = lit.value;
    const builder = converter.builder;
    if(val.equals("true")) return attrValConv.wrapArgAttrValueVariantCall(builder, "Boolean", builder.make_bool_value(true, location));
    if(val.equals("false")) return attrValConv.wrapArgAttrValueVariantCall(builder, "Boolean", builder.make_bool_value(false, location));
    
    const text = strip_js_string_quotes(val);
    return attrValConv.wrapArgAttrValueVariantCall(builder, "Text", converter.make_ssr_text(text, location));
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
            ssrAttributeValueNode : support.ssrAttributeValueNode,
            multipleAttributeValueNode : support.multipleAttributeValueNode,
            parent : converter.parent
        }

        const attributes = &element.opening.attributes;
        for(var i : uint = 0; i < attributes.size(); i++) {
            const attrNode = attributes.get(i);
            if(attrNode == null) continue;

            const attrStructVal = builder.make_struct_value(support.ssrAttrLinkedNode, location);

            if(attrNode.kind == JsNodeKind.JSXAttribute) {

                const attr = attrNode as *mut JsJSXAttribute;
                attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text(attr.name, location));

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
                        } else {
                            // Fallback for other expressions to avoid "value not given" error.
                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text("", location)));
                        }
                    } else {
                        const boolVal = builder.make_bool_value(true, location);
                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                    }
                }
            } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute){
                const attr = attrNode as *mut JsJSXSpreadAttribute
                // TODO: currently we only support spreading props (the real argument passed)
                //  we automatically spread props when user spreads any object
                //  we should check whether its a javascript object, or the props being passed from the current function
                const params = converter.current_func.get_params()
                const propsParam = params.get(1)
                const spread_props = builder.make_identifier("props", propsParam, false, location);

                attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text("spread", location));
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Spread"), spread_props));
            }
            attrValues.push(attrStructVal as *mut Value);
        }

        listStruct.add_value(std::string_view("data"), arrayValue as *mut Value);
        listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(attributes.size(), location));
    }

    return listStruct as *mut Value;
}
