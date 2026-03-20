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

func (converter : &mut JsConverter) make_ssr_prop_v_call(propName : std::string_view) : *mut Value {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    const params = converter.current_func.get_params();
    const propsParam = params.get(1);
    if(propsParam == null) return builder.make_null_value(location);

    const propsId = builder.make_identifier("attrs", propsParam, false, location);
    const propsType = propsParam.getType();
    const derefProps = builder.make_dereference_value(propsId, (propsType as *mut PointerType).getChildType(), location);
    const nameVal = converter.make_ssr_text(propName, location);
    const call = builder.make_function_call_value(builder.make_identifier("getSsrAttributeValue", support.getSsrAttributeValueFn, false, location), location);
    call.get_args().push(derefProps);
    call.get_args().push(nameVal);
    return call;
}

func (converter : &mut JsConverter) render_ssr_value_call(value : *mut Value) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    var pageId = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var fnPtr = if(converter.target == BufferType.HTML) support.renderHtmlAttrValueFn else support.renderJsAttrValueFn;
    var fnName = if(converter.target == BufferType.HTML) view("renderHtmlAttrValue") else view("renderJsAttrValue");

    var id = builder.make_identifier(fnName, fnPtr, false, location);
    var call = builder.make_function_call_node(id, converter.parent, location)
    call.get_args().push(pageId);
    call.get_args().push(value);
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;

    const location = intrinsics::get_raw_location();
    const str_view = converter.builder.allocate_view(converter.str.to_view());
    const val = converter.builder.make_string_value(str_view, location);
    const call = converter.make_value_call(val, converter.str.size());
    converter.vec.push(call);
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
    converter.vec.push(call)
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

func make_ssr_text_val(builder : *mut ASTBuilder, val : &std::string_view, textNode : *mut ASTNode, location : ubigint) : *mut Value {
    const structVal = builder.make_struct_value(textNode, location);
    structVal.add_value(std::string_view("data"), builder.make_string_value(val, location));
    structVal.add_value(std::string_view("size"), builder.make_ubigint_value(val.size(), location));
    return structVal;
}

func (converter : &mut JsConverter) make_ssr_text(val : &std::string_view, location : ubigint) : *mut Value {
    return make_ssr_text_val(converter.builder, val, converter.support.ssrTextLinkedNode, location);
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

struct SsrJsExprEval {
    var valid : bool
    var kind : int
    var boolValue : bool
    var numberValue : bigint
    var textValue : std::string_view
}

func ssr_js_eval_invalid() : SsrJsExprEval {
    return SsrJsExprEval {
        valid : false,
        kind : 0,
        boolValue : false,
        numberValue : 0,
        textValue : view("")
    };
}

func ssr_js_eval_bool(value : bool) : SsrJsExprEval {
    return SsrJsExprEval {
        valid : true,
        kind : 1,
        boolValue : value,
        numberValue : if(value) 1 else 0,
        textValue : if(value) view("true") else view("false")
    };
}

func ssr_js_eval_number(value : bigint, text : std::string_view) : SsrJsExprEval {
    return SsrJsExprEval {
        valid : true,
        kind : 2,
        boolValue : value != 0,
        numberValue : value,
        textValue : text
    };
}

func ssr_js_eval_text(text : std::string_view) : SsrJsExprEval {
    return SsrJsExprEval {
        valid : true,
        kind : 3,
        boolValue : !text.empty(),
        numberValue : 0,
        textValue : text
    };
}

func parse_ssr_bigint(text : std::string_view, outValue : &mut bigint) : bool {
    if(text.empty()) return false;
    var idx : uint = 0;
    var sign : bigint = 1;
    if(text.get(0) == '-') {
        sign = -1;
        idx = 1;
        if(idx >= text.size()) return false;
    }
    var value : bigint = 0;
    while(idx < text.size()) {
        const c = text.get(idx);
        if(c < '0' || c > '9') return false;
        value = value * 10 + ((c - '0') as bigint);
        idx++
    }
    *outValue = value * sign;
    return true;
}

func (converter : &mut JsConverter) find_state_init_text(name : std::string_view) : std::string_view {
    for(var i : uint = 0; i < converter.state_inits.size(); i++) {
        const init = converter.state_inits.get(i);
        if(init.name.equals(name)) return init.init;
    }
    return view("");
}

func ssr_js_eval_equals(left : SsrJsExprEval, right : SsrJsExprEval) : bool {
    if(!left.valid || !right.valid) return false;
    if(left.kind == right.kind) {
        switch(left.kind) {
            1 => return left.boolValue == right.boolValue
            2 => return left.numberValue == right.numberValue
            3 => return left.textValue.equals(right.textValue)
            default => return false
        }
    }
    if(left.kind == 2 && right.kind == 1) return left.numberValue == (if(right.boolValue) 1 else 0);
    if(left.kind == 1 && right.kind == 2) return (if(left.boolValue) 1 else 0) == right.numberValue;
    return left.textValue.equals(right.textValue);
}

func ssr_js_eval_from_text(text : std::string_view) : SsrJsExprEval {
    if(text.empty()) return ssr_js_eval_invalid();
    if(text.equals(view("true"))) return ssr_js_eval_bool(true);
    if(text.equals(view("false"))) return ssr_js_eval_bool(false);
    const stripped = strip_js_string_quotes(text);
    if(stripped.size() < text.size()) return ssr_js_eval_text(stripped);
    var num : bigint = 0;
    if(parse_ssr_bigint(text, num)) return ssr_js_eval_number(num, text);
    return ssr_js_eval_invalid();
}

func (converter : &mut JsConverter) eval_ssr_js_expr(node : *mut JsNode) : SsrJsExprEval {
    if(node == null) return ssr_js_eval_invalid();
    switch(node.kind) {
        JsNodeKind.Literal => {
            return ssr_js_eval_from_text((node as *mut JsLiteral).value);
        }
        JsNodeKind.Identifier => {
            const id = node as *mut JsIdentifier;
            if(converter.is_state_var(id.value)) {
                return ssr_js_eval_from_text(converter.find_state_init_text(id.value));
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                const id = mem.object as *mut JsIdentifier;
                if(converter.is_state_var(id.value)) {
                    return ssr_js_eval_from_text(converter.find_state_init_text(id.value));
                }
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.UnaryOp => {
            const unary = node as *mut JsUnaryOp;
            if(unary.operator.equals(view("!"))) {
                const operand = converter.eval_ssr_js_expr(unary.operand);
                if(operand.valid && operand.kind == 1) return ssr_js_eval_bool(!operand.boolValue);
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            const left = converter.eval_ssr_js_expr(bin.left);
            const right = converter.eval_ssr_js_expr(bin.right);
            if(!left.valid || !right.valid) return ssr_js_eval_invalid();
            if(bin.op.equals(view("==")) || bin.op.equals(view("==="))) {
                return ssr_js_eval_bool(ssr_js_eval_equals(left, right));
            }
            if(bin.op.equals(view("!=")) || bin.op.equals(view("!=="))) {
                return ssr_js_eval_bool(!ssr_js_eval_equals(left, right));
            }
            if(bin.op.equals(view("&&")) && left.kind == 1 && right.kind == 1) {
                return ssr_js_eval_bool(left.boolValue && right.boolValue);
            }
            if(bin.op.equals(view("||")) && left.kind == 1 && right.kind == 1) {
                return ssr_js_eval_bool(left.boolValue || right.boolValue);
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            const cond = converter.eval_ssr_js_expr(tern.condition);
            if(!cond.valid || cond.kind != 1) return ssr_js_eval_invalid();
            return converter.eval_ssr_js_expr(if(cond.boolValue) tern.consequent else tern.alternate);
        }
        default => return ssr_js_eval_invalid()
    }
}

func (converter : &mut JsConverter) jsx_expr_needs_reactive_wrapper(node : *mut JsNode) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Identifier => {
            return converter.is_state_var((node as *mut JsIdentifier).value);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                return converter.is_state_var((mem.object as *mut JsIdentifier).value);
            }
            return converter.jsx_expr_needs_reactive_wrapper(mem.object);
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            return converter.jsx_expr_needs_reactive_wrapper(idx.object) || converter.jsx_expr_needs_reactive_wrapper(idx.index);
        }
        JsNodeKind.UnaryOp => {
            return converter.jsx_expr_needs_reactive_wrapper((node as *mut JsUnaryOp).operand);
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            return converter.jsx_expr_needs_reactive_wrapper(bin.left) || converter.jsx_expr_needs_reactive_wrapper(bin.right);
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            return converter.jsx_expr_needs_reactive_wrapper(tern.condition) ||
                converter.jsx_expr_needs_reactive_wrapper(tern.consequent) ||
                converter.jsx_expr_needs_reactive_wrapper(tern.alternate);
        }
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall;
            if(converter.jsx_expr_needs_reactive_wrapper(call.callee)) return true;
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(converter.jsx_expr_needs_reactive_wrapper(call.args.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            const arr = node as *mut JsArrayLiteral;
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(converter.jsx_expr_needs_reactive_wrapper(arr.elements.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ObjectLiteral => {
            const obj = node as *mut JsObjectLiteral;
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(converter.jsx_expr_needs_reactive_wrapper(obj.properties.get(i).value)) return true;
            }
            return false;
        }
        default => return false
    }
}

func (converter : &mut JsConverter) convert_jsx_runtime_expr(node : *mut JsNode) {
    if(node == null) {
        converter.str.append_view("undefined");
        return;
    }
    if(node.kind == JsNodeKind.Identifier) {
        const id = node as *mut JsIdentifier;
        if(converter.is_state_var(id.value)) {
            converter.str.append_view(id.value);
            return;
        }
    } else if(node.kind == JsNodeKind.MemberAccess) {
        const mem = node as *mut JsMemberAccess;
        if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
            const id = mem.object as *mut JsIdentifier;
            if(converter.is_state_var(id.value)) {
                converter.str.append_view(id.value);
                return;
            }
        }
    }
    if(converter.jsx_expr_needs_reactive_wrapper(node)) {
        converter.str.append_view("$_ucs(() => ");
        converter.convertJsNode(node);
        converter.str.append_view(")");
    } else {
        converter.convertJsNode(node);
    }
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
                    var handled = false;
                    if(container.expression != null) {
                        if(container.expression.kind == JsNodeKind.ChemicalValue) {
                            const chem = container.expression as *mut JsChemicalValue;
                            attrStructVal.add_value(std::string_view("value"), attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.Literal) {
                            attrStructVal.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(container.expression as *mut JsLiteral, attrValConv, location));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.ObjectLiteral) {
                            var objText = std::string_view();
                            if(attr.name.equals("style")) {
                                objText = build_js_node_text_view_style_attr(builder, container.expression as *mut JsObjectLiteral)
                            } else {
                                objText = build_js_node_text_view(builder, container.expression)
                            }
                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(objText, location)));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                            const mem = container.expression as *mut JsMemberAccess;
                            if(mem.object.kind == JsNodeKind.Identifier && (mem.object as *mut JsIdentifier).value.equals("props")) {
                                const params = converter.current_func.get_params();
                                const propsParam = params.get(1);
                                if(propsParam != null) {
                                    const propsId = builder.make_identifier("attrs", propsParam, false, location);
                                    const propsType = propsParam.getType();
                                    const derefProps = builder.make_dereference_value(propsId, (propsType as *mut PointerType).getChildType(), location);
                                    const nameVal = converter.make_ssr_text(mem.property, location);
                                    const call = builder.make_function_call_value(builder.make_identifier("getSsrAttributeValue", support.getSsrAttributeValueFn, false, location), location);
                                    call.get_args().push(derefProps);
                                    call.get_args().push(nameVal);
                                    attrStructVal.add_value(std::string_view("value"), call);
                                    handled = true
                                }
                            }
                            if(!handled) {
                                const evaluated = converter.eval_ssr_js_expr(container.expression);
                                if(evaluated.valid) {
                                    if(evaluated.kind == 1) {
                                        if(!evaluated.boolValue) continue;
                                        const boolVal = builder.make_bool_value(true, location);
                                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                                    } else {
                                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(evaluated.textValue, location)));
                                    }
                                    handled = true;
                                }
                            }
                        } else {
                            const evaluated = converter.eval_ssr_js_expr(container.expression);
                            if(evaluated.valid) {
                                if(evaluated.kind == 1) {
                                    if(!evaluated.boolValue) continue;
                                    const boolVal = builder.make_bool_value(true, location);
                                    attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                                } else {
                                    attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(evaluated.textValue, location)));
                                }
                                handled = true;
                            }
                        }
                    } else {
                        const boolVal = builder.make_bool_value(true, location);
                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                        handled = true;
                    }

                    if(!handled) {
                        continue;
                    }
                }
                attrValues.push(attrStructVal);
                pushedCount++;
            } else if(attrNode.kind == JsNodeKind.JSXSpreadAttribute){
                const attr = attrNode as *mut JsJSXSpreadAttribute
                // TODO: currently we only support spreading props (the real argument passed)
                //  we automatically spread props when user spreads any object
                //  we should check whether its a javascript object, or the props being passed from the current function
                const params = converter.current_func.get_params()
                const propsParam = params.get(1)
                const spread_props = builder.make_identifier("attrs", propsParam, false, location);
                const deref_spread_props = builder.make_dereference_value(spread_props, spread_props.getType(), location)
                const attrStructVal = builder.make_struct_value(support.ssrAttrLinkedNode, location);

                attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text("spread", location));
                attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Spread"), deref_spread_props));
                attrValues.push(attrStructVal);
                pushedCount++;
            }
        }

        listStruct.add_value(std::string_view("data"), arrayValue);
        listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(pushedCount, location));
    }

    return listStruct as *mut Value;
}
