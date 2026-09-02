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
            if (c1 == '`' as uint) {
                str.append_view("\\`");
            } else if (c1 == '\\' as uint) {
                str.append_view("\\\\");
            } else if (c1 == '\n' as uint) {
                str.append_view("\\n");
            } else if (c1 == '\r' as uint) {
                str.append_view("\\r");
            } else if (c1 == '\t' as uint) {
                str.append_view("\\t");
            } else if (c1 == '\"' as uint) {
                str.append_view("\\\"");
            } else if (c1 == '\'' as uint) {
                str.append_view("\\'");
            } else if (c1 == '$' as uint && i + 1 < text.size() && ((text.data()[i+1] as uint) & 0xFF) == '{' as uint) {
                str.append_view("\\$");
                str.append('{');
                i += 2;
                continue;
            } else {
                str.append(c1 as char);
            }
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

func (converter : &mut JsConverter) is_reactive_var(name : std::string_view) : bool {
    for(var i : uint = 0; i < converter.state_vars.size(); i++) {
        if(converter.state_vars.get(i).equals(&name)) {
            return true;
        }
    }
    for(var i : uint = 0; i < converter.computed_vars.size(); i++) {
        if(converter.computed_vars.get(i).equals(&name)) {
            return true;
        }
    }
    return false;
}

// True when `name` is a local bound to createContext/useContext (a context var).
func (converter : &mut JsConverter) is_context_var(name : std::string_view) : bool {
    return converter.find_context_var(name) != null;
}

// Returns the recorded context var for `name`, or null. The name/default
// expressions let SSR resolve `ctx.value` to the static default.
func (converter : &mut JsConverter) find_context_var(name : std::string_view) : *JsContextVar {
    for(var i : uint = 0; i < converter.context_vars.size(); i++) {
        if(converter.context_vars.get_ptr(i).name.equals(&name)) {
            return converter.context_vars.get_ptr(i);
        }
    }
    return null;
}

// Records `name` as a context var bound to a createContext/useContext call.
// `nameExpr`/`defaultExpr` are the call's registry-key and default-value
// expressions (default may be null for useContext).
func (converter : &mut JsConverter) add_context_var(name : std::string_view, nameExpr : *mut JsNode, defaultExpr : *mut JsNode) {
    converter.context_vars.push(JsContextVar {
        name : name,
        nameExpr : nameExpr,
        defaultExpr : defaultExpr
    });
}

func (converter : &mut JsConverter) expr_references_reactive_var(node : *mut JsNode) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Identifier => {
            return converter.is_reactive_var((node as *mut JsIdentifier).value);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier) {
                const objName = (mem.object as *mut JsIdentifier).value;
                if(converter.is_context_var(objName)) return true;
                if(mem.property.equals(view("value"))) {
                    return converter.is_reactive_var(objName);
                }
            }
            return converter.expr_references_reactive_var(mem.object);
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            return converter.expr_references_reactive_var(idx.object) || converter.expr_references_reactive_var(idx.index);
        }
        JsNodeKind.UnaryOp => {
            return converter.expr_references_reactive_var((node as *mut JsUnaryOp).operand);
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            return converter.expr_references_reactive_var(bin.left) || converter.expr_references_reactive_var(bin.right);
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            return converter.expr_references_reactive_var(tern.condition) ||
                converter.expr_references_reactive_var(tern.consequent) ||
                converter.expr_references_reactive_var(tern.alternate);
        }
        JsNodeKind.FunctionCall => {
            const call = node as *mut JsFunctionCall;
            if(converter.expr_references_reactive_var(call.callee)) return true;
            for(var i : uint = 0; i < call.args.size(); i++) {
                if(converter.expr_references_reactive_var(call.args.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ArrayLiteral, JsNodeKind.ArrayDestructuring => {
            const arr = node as *mut JsArrayLiteral;
            for(var i : uint = 0; i < arr.elements.size(); i++) {
                if(converter.expr_references_reactive_var(arr.elements.get(i))) return true;
            }
            return false;
        }
        JsNodeKind.ObjectLiteral => {
            const obj = node as *mut JsObjectLiteral;
            for(var i : uint = 0; i < obj.properties.size(); i++) {
                if(converter.expr_references_reactive_var(obj.properties.get(i).value)) return true;
            }
            return false;
        }
        JsNodeKind.Paren => {
            return converter.expr_references_reactive_var((node as *mut JsParen).expression);
        }
        default => return false
    }
}

func (converter : &mut JsConverter) is_component_props_name(name : std::string_view) : bool {
    return !converter.component_props_name.empty() && converter.component_props_name.equals(&name);
}

func (converter : &mut JsConverter) is_component_props_root(node : *mut JsNode) : bool {
    return node != null &&
        node.kind == JsNodeKind.Identifier &&
        converter.is_component_props_name((node as *mut JsIdentifier).value);
}

func (converter : &mut JsConverter) is_component_props_read(node : *mut JsNode) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Identifier => {
            return converter.is_component_props_name((node as *mut JsIdentifier).value);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(converter.is_component_props_root(mem.object)) return true;
            return converter.is_component_props_read(mem.object);
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            return converter.is_component_props_read(idx.object);
        }
        default => return false
    }
}

func (converter : &mut JsConverter) append_component_prop_value(node : *mut JsNode) {
    converter.str.append_view("window.$__uni_value(");
    if(!append_js_node_text(node, &mut converter.str)) {
        converter.convertJsNode(node);
    }
    converter.str.append(')');
}

func (converter : &mut JsConverter) make_require_component_call(hash : size_t) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var value = builder.make_ubigint_value(hash, location)
    const support = converter.support;
    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("require_component"), support.requireComponentFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
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

    var id = builder.make_identifier(&name, fnPtr, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendHeadJsCharPtrFn, converter.support.appendHtmlCharPtrFn)
}

// Appends a char pointer escaped for embedding inside a JS string literal.
// Only valid for the JavaScript target (the HTML target has no escaping here).
func (converter : &mut JsConverter) make_escaped_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_js_escaped_char_ptr"), converter.support.appendHeadJsEscapedCharPtrFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
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

    var id = builder.make_identifier(&name, fnPtr, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
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
    const nameVal = converter.make_ssr_text(&propName, location);
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

    var id = builder.make_identifier(&fnName, fnPtr, false, location);
    var call = builder.make_function_call_node(id, converter.parent, location)
    call.get_args().push(pageId);
    call.get_args().push(value);
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;

    const location = intrinsics::get_raw_location();
    const str_view = converter.builder.allocate_view(converter.str.to_view());
    const val = converter.builder.make_string_value(&str_view, location);
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

    var id = converter.builder.make_identifier(&name, fnPtr, false, location);
    const chain = converter.builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
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

func append_utf8_codepoint(str : &mut std::string, cp : uint32_t) {
    if(cp < 0x80) {
        str.append(cp as char);
    } else if(cp < 0x800) {
        str.append((0xC0 | (cp >> 6)) as char);
        str.append((0x80 | (cp & 0x3F)) as char);
    } else if(cp < 0x10000) {
        str.append((0xE0 | (cp >> 12)) as char);
        str.append((0x80 | ((cp >> 6) & 0x3F)) as char);
        str.append((0x80 | (cp & 0x3F)) as char);
    } else {
        str.append((0xF0 | (cp >> 18)) as char);
        str.append((0x80 | ((cp >> 12) & 0x3F)) as char);
        str.append((0x80 | ((cp >> 6) & 0x3F)) as char);
        str.append((0x80 | (cp & 0x3F)) as char);
    }
}

func lookup_named_entity(name : std::string_view) : uint32_t {
    if(name.equals("amp")) return 38;
    if(name.equals("lt")) return 60;
    if(name.equals("gt")) return 62;
    if(name.equals("quot")) return 34;
    if(name.equals("apos")) return 39;
    if(name.equals("nbsp")) return 160;
    if(name.equals("iexcl")) return 161;
    if(name.equals("cent")) return 162;
    if(name.equals("pound")) return 163;
    if(name.equals("curren")) return 164;
    if(name.equals("yen")) return 165;
    if(name.equals("brvbar")) return 166;
    if(name.equals("sect")) return 167;
    if(name.equals("uml")) return 168;
    if(name.equals("copy")) return 169;
    if(name.equals("ordf")) return 170;
    if(name.equals("laquo")) return 171;
    if(name.equals("not")) return 172;
    if(name.equals("shy")) return 173;
    if(name.equals("reg")) return 174;
    if(name.equals("macr")) return 175;
    if(name.equals("deg")) return 176;
    if(name.equals("plusmn")) return 177;
    if(name.equals("sup2")) return 178;
    if(name.equals("sup3")) return 179;
    if(name.equals("acute")) return 180;
    if(name.equals("micro")) return 181;
    if(name.equals("para")) return 182;
    if(name.equals("middot")) return 183;
    if(name.equals("cedil")) return 184;
    if(name.equals("sup1")) return 185;
    if(name.equals("ordm")) return 186;
    if(name.equals("raquo")) return 187;
    if(name.equals("frac14")) return 188;
    if(name.equals("frac12")) return 189;
    if(name.equals("frac34")) return 190;
    if(name.equals("iquest")) return 191;
    if(name.equals("times")) return 215;
    if(name.equals("divide")) return 247;
    if(name.equals("ndash")) return 8211;
    if(name.equals("mdash")) return 8212;
    if(name.equals("lsquo")) return 8216;
    if(name.equals("rsquo")) return 8217;
    if(name.equals("sbquo")) return 8218;
    if(name.equals("ldquo")) return 8220;
    if(name.equals("rdquo")) return 8221;
    if(name.equals("bdquo")) return 8222;
    if(name.equals("hellip")) return 8230;
    if(name.equals("larr")) return 8592;
    if(name.equals("uarr")) return 8593;
    if(name.equals("rarr")) return 8594;
    if(name.equals("darr")) return 8595;
    if(name.equals("harr")) return 8596;
    if(name.equals("bull")) return 8226;
    if(name.equals("trade")) return 8482;
    if(name.equals("euro")) return 8364;
    if(name.equals("lsaquo")) return 8249;
    if(name.equals("rsaquo")) return 8250;
    if(name.equals("le")) return 8804;
    if(name.equals("ge")) return 8805;
    if(name.equals("ne")) return 8800;
    if(name.equals("equiv")) return 8801;
    if(name.equals("forall")) return 8704;
    if(name.equals("part")) return 8706;
    if(name.equals("exist")) return 8707;
    if(name.equals("empty")) return 8709;
    if(name.equals("nabla")) return 8711;
    if(name.equals("isin")) return 8712;
    if(name.equals("notin")) return 8713;
    if(name.equals("ni")) return 8715;
    if(name.equals("prod")) return 8719;
    if(name.equals("sum")) return 8721;
    if(name.equals("minus")) return 8722;
    if(name.equals("lowast")) return 8727;
    if(name.equals("radic")) return 8730;
    if(name.equals("prop")) return 8733;
    if(name.equals("infin")) return 8734;
    if(name.equals("ang")) return 8736;
    if(name.equals("and")) return 8743;
    if(name.equals("or")) return 8744;
    if(name.equals("cap")) return 8745;
    if(name.equals("cup")) return 8746;
    if(name.equals("int")) return 8747;
    if(name.equals("there4")) return 8756;
    if(name.equals("sim")) return 8764;
    if(name.equals("cong")) return 8773;
    if(name.equals("asymp")) return 8776;
    if(name.equals("sub")) return 8834;
    if(name.equals("sup")) return 8835;
    if(name.equals("nsub")) return 8836;
    if(name.equals("sube")) return 8838;
    if(name.equals("supe")) return 8839;
    if(name.equals("oplus")) return 8853;
    if(name.equals("otimes")) return 8855;
    if(name.equals("perp")) return 8869;
    if(name.equals("sdot")) return 8901;
    return 0;
}

func decode_html_entities(text : std::string_view) : std::string {
    var result = std::string();
    var i = 0u;
    while(i < text.size()) {
        if(text.data()[i] == '&') {
            var semicolon = i + 1;
            while(semicolon < text.size() && text.data()[semicolon] != ';') semicolon++;
            if(semicolon < text.size()) {
                var entity_body = std::string_view(text.data() + i + 1, semicolon - i - 1);
                if(entity_body.size() > 0 && entity_body.data()[0] == '#') {
                    var num_part = std::string_view(entity_body.data() + 1, entity_body.size() - 1);
                    var is_hex = false;
                    if(num_part.size() > 0 && (num_part.data()[0] == 'x' || num_part.data()[0] == 'X')) {
                        is_hex = true;
                        num_part = std::string_view(num_part.data() + 1, num_part.size() - 1);
                    }
                    var cp : uint32_t = 0;
                    if(is_hex) {
                        for(var j = 0u; j < num_part.size(); j++) {
                            const c = num_part.data()[j];
                            if(c >= '0' && c <= '9') cp = cp * 16 + (c as uint32_t - '0' as uint32_t);
                            else if(c >= 'a' && c <= 'f') cp = cp * 16 + (c as uint32_t - 'a' as uint32_t + 10);
                            else if(c >= 'A' && c <= 'F') cp = cp * 16 + (c as uint32_t - 'A' as uint32_t + 10);
                            else { cp = 0; break; }
                        }
                    } else {
                        for(var j = 0u; j < num_part.size(); j++) {
                            const c = num_part.data()[j];
                            if(c >= '0' && c <= '9') cp = cp * 10 + (c as uint32_t - '0' as uint32_t);
                            else { cp = 0; break; }
                        }
                    }
                    if(cp > 0) {
                        append_utf8_codepoint(&mut result, cp);
                        i = semicolon + 1;
                        continue;
                    }
                } else {
                    var cp = lookup_named_entity(entity_body);
                    if(cp > 0) {
                        append_utf8_codepoint(&mut result, cp);
                        i = semicolon + 1;
                        continue;
                    }
                }
            }
        }
        result.append(text.data()[i]);
        i++;
    }
    return result;
}

func append_js_node_text(node : *mut JsNode, out : &mut std::string) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Literal => {
            out.append_view(&(node as *mut JsLiteral).value);
            return true;
        }
        JsNodeKind.Identifier => {
            out.append_view(&(node as *mut JsIdentifier).value);
            return true;
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(!append_js_node_text(mem.object, out)) return false;
            out.append('.');
            out.append_view(&mem.property);
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
                out.append_view(&unary.operator);
                if(unary.operator.size() > 2 && isalpha(unary.operator.get(0) as int)) {
                    out.append(' ');
                }
                return append_js_node_text(unary.operand, out);
            }
            if(!append_js_node_text(unary.operand, out)) return false;
            out.append_view(&unary.operator);
            return true;
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            if(bin.left != null && bin.left.kind == JsNodeKind.Ternary) {
                out.append('(');
                if(!append_js_node_text(bin.left, out)) return false;
                out.append(')');
            } else {
                if(!append_js_node_text(bin.left, out)) return false;
            }
            out.append(' ');
            out.append_view(&bin.op);
            out.append(' ');
            if(bin.right != null && bin.right.kind == JsNodeKind.Ternary) {
                out.append('(');
                if(!append_js_node_text(bin.right, out)) return false;
                out.append(')');
                return true;
            }
            return append_js_node_text(bin.right, out);
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            out.append('(');
            if(!append_js_node_text(tern.condition, out)) return false;
            out.append_view(" ? ");
            if(!append_js_node_text(tern.consequent, out)) return false;
            out.append_view(" : ");
            const final = append_js_node_text(tern.alternate, out);
            out.append(')');
            return final;
        }
        JsNodeKind.Paren => {
            const paren = node as *mut JsParen;
            out.append('(');
            if(!append_js_node_text(paren.expression, out)) return false;
            out.append(')');
            return true;
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
                if(prop.value != null && prop.value.kind == JsNodeKind.Spread) {
                    if(!append_js_node_text(prop.value, out)) return false;
                } else {
                    out.append_view(&prop.key);
                    out.append_view(": ");
                    if(!append_js_node_text(prop.value, out)) return false;
                }
            }
            out.append('}');
            return true;
        }
        JsNodeKind.Spread => {
            const spread = node as *mut JsSpread;
            out.append_view("...");
            return append_js_node_text(spread.argument, out);
        }
        default => return false
    }
}

func build_js_node_text_view(builder : *mut ASTBuilder, node : *mut JsNode) : std::string_view {
    var text = std::string();
    if(!append_js_node_text(node, &mut text)) return std::string_view();
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
        if(init.name.equals(&name)) return init.init;
    }
    return view("");
}

func ssr_js_eval_equals(left : SsrJsExprEval, right : SsrJsExprEval) : bool {
    if(!left.valid || !right.valid) return false;
    if(left.kind == right.kind) {
        switch(left.kind) {
            1 => return left.boolValue == right.boolValue
            2 => return left.numberValue == right.numberValue
            3 => return left.textValue.equals(&right.textValue)
            default => return false
        }
    }
    if(left.kind == 2 && right.kind == 1) return left.numberValue == (if(right.boolValue) 1 else 0);
    if(left.kind == 1 && right.kind == 2) return (if(left.boolValue) 1 else 0) == right.numberValue;
    return left.textValue.equals(&right.textValue);
}

func ssr_js_eval_from_text(text : std::string_view) : SsrJsExprEval {
    // Trim surrounding whitespace (array literals split on commas keep the
    // separating spaces, e.g. `["a", "b"]` produces `" "b"`).
    var t = text
    while(t.size() > 0 && (t.get(0) == ' ' || t.get(0) == '\n' || t.get(0) == '\t' || t.get(0) == '\r')) t = t.skip(1)
    while(t.size() > 0 && (t.get(t.size() - 1) == ' ' || t.get(t.size() - 1) == '\n' || t.get(t.size() - 1) == '\t' || t.get(t.size() - 1) == '\r')) t = t.subview(0, t.size() - 1)
    if(t.empty()) return ssr_js_eval_invalid();
    if(t.equals(view("true"))) return ssr_js_eval_bool(true);
    if(t.equals(view("false"))) return ssr_js_eval_bool(false);
    const stripped = strip_js_string_quotes(t);
    if(stripped.size() < t.size()) return ssr_js_eval_text(stripped);
    var num : bigint = 0;
    if(parse_ssr_bigint(t, &mut num)) return ssr_js_eval_number(num, t);
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
            // While evaluating a `.filter()` predicate over a static array,
            // the callback parameter is bound to the current element.
            if(converter.ssr_bound_param_valid && id.value.equals(&converter.ssr_bound_param)) {
                return converter.ssr_bound_param_value;
            }
            // The second `.map()` callback parameter (`index`) bound during
            // compile-time unrolling of static array sources.
            if(converter.ssr_index_param_valid && id.value.equals(&converter.ssr_index_param)) {
                return converter.ssr_index_param_value;
            }
            if(converter.is_reactive_var(id.value)) {
                return ssr_js_eval_from_text(converter.find_state_init_text(id.value));
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                const id = mem.object as *mut JsIdentifier;
                if(converter.is_reactive_var(id.value)) {
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
            if(bin.op.equals(view("+"))) {
                if(left.kind == 2 && right.kind == 2) {
                    // numeric addition
                    var sumText = std::string();
                    sumText.append_integer(left.numberValue + right.numberValue);
                    const sumView = converter.builder.allocate_view(sumText.to_view());
                    return ssr_js_eval_number(left.numberValue + right.numberValue, sumView);
                }
                // string concatenation (JS: number + string coerces to string)
                if(left.kind == 3 || right.kind == 3 || left.kind == 2 || right.kind == 2) {
                    var cat = std::string();
                    cat.append_view(&left.textValue);
                    cat.append_view(&right.textValue);
                    const catView = converter.builder.allocate_view(cat.to_view());
                    return ssr_js_eval_text(catView);
                }
            }
            // Arithmetic: -, *, / (whole-number results) so static JSX
            // expressions like {1 + 2 * 3} evaluate numerically at SSR.
            if(left.kind == 2 && right.kind == 2) {
                if(bin.op.equals(view("-"))) {
                    var subText = std::string();
                    subText.append_integer(left.numberValue - right.numberValue);
                    const subView = converter.builder.allocate_view(subText.to_view());
                    return ssr_js_eval_number(left.numberValue - right.numberValue, subView);
                }
                if(bin.op.equals(view("*"))) {
                    var mulText = std::string();
                    mulText.append_integer(left.numberValue * right.numberValue);
                    const mulView = converter.builder.allocate_view(mulText.to_view());
                    return ssr_js_eval_number(left.numberValue * right.numberValue, mulView);
                }
                if(bin.op.equals(view("/")) && right.numberValue != 0 && left.numberValue % right.numberValue == 0) {
                    var divText = std::string();
                    divText.append_integer(left.numberValue / right.numberValue);
                    const divView = converter.builder.allocate_view(divText.to_view());
                    return ssr_js_eval_number(left.numberValue / right.numberValue, divView);
                }
            }
            return ssr_js_eval_invalid();
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            const cond = converter.eval_ssr_js_expr(tern.condition);
            if(!cond.valid || cond.kind != 1) return ssr_js_eval_invalid();
            return converter.eval_ssr_js_expr(if(cond.boolValue) tern.consequent else tern.alternate);
        }
        JsNodeKind.Paren => {
            return converter.eval_ssr_js_expr((node as *mut JsParen).expression);
        }
        default => return ssr_js_eval_invalid()
    }
}

func (converter : &mut JsConverter) jsx_expr_needs_reactive_wrapper(node : *mut JsNode) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Identifier => {
            const name = (node as *mut JsIdentifier).value;
            if(converter.is_reactive_var(name)) return true;
            return converter.in_jsx_attribute && converter.is_component_props_name(name);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier) {
                const objName = (mem.object as *mut JsIdentifier).value;
                if(converter.is_context_var(objName)) return true;
                if(mem.property.equals(view("value"))) {
                    return converter.is_reactive_var(objName);
                }
            }
            if(converter.in_jsx_attribute && converter.is_component_props_read(node)) return true;
            return converter.jsx_expr_needs_reactive_wrapper(mem.object);
        }
        JsNodeKind.IndexAccess => {
            const idx = node as *mut JsIndexAccess;
            if(converter.in_jsx_attribute && converter.is_component_props_read(node)) return true;
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
        JsNodeKind.Paren => {
            return converter.jsx_expr_needs_reactive_wrapper((node as *mut JsParen).expression);
        }
        default => return false
    }
}

func (converter : &mut JsConverter) convert_jsx_runtime_expr(node : *mut JsNode) {
    if(node == null) {
        converter.str.append_view("undefined");
        return;
    }

    if(converter.is_component_props_read(node)) {
        if(converter.is_props_children(node)) {
            converter.convertJsNode(node);
            return;
        }
        append_js_node_text(node, &mut converter.str);
        return;
    }

    if(node.kind == JsNodeKind.Identifier) {
        const id = node as *mut JsIdentifier;
        if(converter.is_reactive_var(id.value)) {
            converter.str.append_view(&id.value);
            return;
        }
    } else if(node.kind == JsNodeKind.MemberAccess) {
        const mem = node as *mut JsMemberAccess;
        if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
            const id = mem.object as *mut JsIdentifier;
            if(converter.is_reactive_var(id.value)) {
                converter.str.append_view(&id.value);
                return;
            }
        }
    }
    if(converter.jsx_expr_needs_reactive_wrapper(node)) {
        converter.str.append_view("$_ucs(() => ");
        if(node.kind == JsNodeKind.ObjectLiteral) {
            converter.str.append('(');
            converter.convertJsNode(node);
            converter.str.append(')');
        } else {
            converter.convertJsNode(node);
        }
        converter.str.append_view(")");
    } else {
        converter.convertJsNode(node);
    }
}

func (converter : &mut JsConverter) convert_jsx_ssr_expression(node : *mut JsNode) {
    if(node == null) return;

    switch(node.kind) {
        JsNodeKind.BinaryOp => {
            var bin = node as *mut JsBinaryOp;
            if(bin.op.equals(view("&&"))) {
                const leftVal = converter.convert_js_expr_to_ssr_bool_value(bin.left);
                if(leftVal != null) {
                    const builder = converter.builder;
                    const location = intrinsics::get_raw_location();

                    const ifStmt = builder.make_if_stmt(leftVal, converter.parent, location);
                    const oldVec = converter.vec;
                    converter.vec = ifStmt.get_body();

                    converter.convert_jsx_ssr_expression(bin.right);

                    converter.vec = oldVec;
                    converter.vec.push(ifStmt as *mut ASTNode);
                    return;
                }
            } else if(bin.op.equals(view("||"))) {
                const leftBoolVal = converter.convert_js_expr_to_ssr_bool_value(bin.left);
                if(leftBoolVal != null) {
                    const builder = converter.builder;
                    const location = intrinsics::get_raw_location();

                    const ifStmt = builder.make_if_stmt(leftBoolVal, converter.parent, location);
                    const oldVec = converter.vec;
                    
                    // If left is truthy, render left
                    converter.vec = ifStmt.get_body();
                    converter.convert_jsx_ssr_expression(bin.left);
                    
                    // Else, render right
                    converter.vec = ifStmt.add_else_body();
                    converter.convert_jsx_ssr_expression(bin.right);

                    converter.vec = oldVec;
                    converter.vec.push(ifStmt as *mut ASTNode);
                    return;
                }
            }
            // Other binary operations (`+`, comparisons): evaluate statically
            // when possible (state values, bound map params, literals), then
            // fall back to a runtime expression rendered as a child value.
            const eval = converter.eval_ssr_js_expr(node);
            if(eval.valid) {
                converter.append_ssr_eval(eval);
                return;
            }
            var attrValConv = converter.make_attr_value_converter();
            const v = converter.convert_ssr_attr_value_expr(node, &mut attrValConv);
            if(v != null) {
                const builder = converter.builder;
                const location = intrinsics::get_raw_location();
                var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                var call = builder.make_function_call_node(
                    builder.make_identifier("renderHtmlChildValue", converter.support.renderHtmlChildValueFn, false, location),
                    converter.parent,
                    location
                );
                call.get_args().push(pageId);
                call.get_args().push(v);
                converter.vec.push(call as *mut ASTNode);
                return;
            }
        }
        JsNodeKind.Ternary => {
            var tern = node as *mut JsTernary;
            const condVal = converter.convert_js_expr_to_ssr_bool_value(tern.condition);
            if(condVal != null) {
                const builder = converter.builder;
                const location = intrinsics::get_raw_location();

                const ifStmt = builder.make_if_stmt(condVal, converter.parent, location);
                const oldVec = converter.vec;

                // Then block
                converter.vec = ifStmt.get_body();
                converter.convert_jsx_ssr_expression(tern.consequent);

                // Else block
                converter.vec = ifStmt.add_else_body();
                converter.convert_jsx_ssr_expression(tern.alternate);

                converter.vec = oldVec;
                converter.vec.push(ifStmt as *mut ASTNode);
                return;
            }
        }
        JsNodeKind.JSXElement, JsNodeKind.JSXFragment => {
            converter.convertJsNode(node);
        }
        JsNodeKind.Paren => {
            var paren = node as *mut JsParen;
            converter.convert_jsx_ssr_expression(paren.expression);
        }
        JsNodeKind.ChemicalValue => {
            converter.convertChemicalValue(node as *mut JsChemicalValue);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            // Array `.length`/`.size` reads (`props.items.length`, `items.length`)
            // render the element count as text, matching the hydrated DOM.
            if(mem.property.equals(view("length")) || mem.property.equals(view("size"))) {
                if(converter.emit_ssr_array_count(mem.object)) {
                    return;
                }
            }
            if(converter.is_component_props_read(node)) {
                if(converter.is_props_children(node)) {
                    // children handled specially in JSXExpressionContainer or here?
                    // if it's props.children, we want to append it to page
                    const builder = converter.builder;
                    const location = intrinsics::get_raw_location();
                    var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                    var childrenId = builder.make_identifier(std::string_view("children"), converter.support.childrenParamNode, false, location);

                    const appendHtmlId = builder.make_identifier(std::string_view("append_html"), converter.support.appendHtmlFn, false, location)
                    var appendCall = builder.make_function_call_node(
                        builder.make_access_chain(&std::span<*mut Value>([ pageId, appendHtmlId ]), location),
                        converter.parent,
                        location
                    );
                    const dataIdNode = converter.support.childrenParamNode.child("data");
                    const sizeIdNode = converter.support.childrenParamNode.child("size");
                    const dataId = builder.make_identifier(view("data"), dataIdNode, false, location)
                    const childrenDataAccess = builder.make_access_chain(&std::span<*mut Value>([ childrenId, dataId ]), location);
                    const sizeId = builder.make_identifier(view("size"), sizeIdNode, false, location)
                    const childrenSizeAccess = builder.make_access_chain(&std::span<*mut Value>([ childrenId, sizeId ]), location);
                    const appendCallParams = appendCall.get_args();
                    appendCallParams.push(childrenDataAccess);
                    appendCallParams.push(childrenSizeAccess);

                    converter.vec.push(appendCall as *mut ASTNode);
                } else {
                    const v = converter.make_ssr_prop_v_call(mem.property);
                    
                    const builder = converter.builder;
                    const location = intrinsics::get_raw_location();
                    var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
                    
                    var call = builder.make_function_call_node(
                        builder.make_identifier("renderHtmlChildValue", converter.support.renderHtmlChildValueFn, false, location),
                        converter.parent,
                        location
                    );
                    call.get_args().push(pageId);
                    call.get_args().push(v);
                    
                    converter.vec.push(call as *mut ASTNode);
                }
            }
        }
        JsNodeKind.FunctionCall => {
            // `createPortal(<jsx/>)`: SSR renders the children inline (there is
            // no body on the server). The client-side hydration moves the
            // SSR'd nodes into a document.body container afterwards.
            const call = node as *mut JsFunctionCall;
            if(call.callee != null && call.callee.kind == JsNodeKind.Identifier) {
                const id = call.callee as *mut JsIdentifier;
                if(id.value.equals(view("createPortal")) && call.args.size() >= 1 && call.args.get(0) != null) {
                    converter.convert_jsx_ssr_expression(call.args.get(0));
                    return;
                }
            }
            // `.map()` over props/state arrays: render each element through the
            // callback, matching the client's initial render.
            if(call.callee != null && call.callee.kind == JsNodeKind.MemberAccess) {
                const mem = call.callee as *mut JsMemberAccess;
                if(mem.property.equals(view("map")) && call.args.size() >= 1 && call.args.get(0) != null && call.args.get(0).kind == JsNodeKind.ArrowFunction) {
                    converter.emit_ssr_map_children(call);
                    return;
                }
            }
        }
        JsNodeKind.Identifier => {
            const id = node as *mut JsIdentifier;
            const builder = converter.builder;
            const location = intrinsics::get_raw_location();

        // Reference to a `.map()` callback parameter bound to a static
        // element (compile-time unrolled map): render the element value.
        if(converter.ssr_bound_param_valid && id.value.equals(&converter.ssr_bound_param)) {
            converter.append_ssr_eval(converter.ssr_bound_param_value);
            return;
        }

        // Reference to the second `.map()` callback parameter (`index`) while
        // a static source is unrolled: render the element index.
        if(converter.ssr_index_param_valid && id.value.equals(&converter.ssr_index_param)) {
            converter.append_ssr_eval(converter.ssr_index_param_value);
            return;
        }

            // Reference to a `state`/computed variable: SSR renders the initial
            // state value as text (matching the client's first render). Only
            // literal init texts are appended directly; anything else falls
            // through to the SSR-local path so `props.x || default` style inits
            // don't leak raw JS into the HTML.
            if(converter.is_reactive_var(id.value)) {
                const initText = converter.find_state_init_text(id.value);
                const eval = ssr_js_eval_from_text(initText);
                if(eval.valid) {
                    converter.append_ssr_eval(eval);
                    return;
                }
            }

            // Reference to a local variable declared in the component body:
            // render it as a child value at SSR runtime. Also the fallback for
            // computed vars whose init text is not a static literal.
            const local = converter.find_ssr_local(id.value);
            if(local == null) return;

            const localRef = builder.make_identifier(&local.name, local.varInit, false, location);
            var pageId = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);

            var call = builder.make_function_call_node(
                builder.make_identifier("renderHtmlChildValue", converter.support.renderHtmlChildValueFn, false, location),
                converter.parent,
                location
            );
            call.get_args().push(pageId);
            call.get_args().push(localRef);

            converter.vec.push(call as *mut ASTNode);
        }
        JsNodeKind.Literal => {
             // For literals or other expressions, we might want to render them
             // but only if they are not props.
             // Actually, if it's a literal string, we should append it to HTML.
             if(node.kind == JsNodeKind.Literal) {
                 var lit = node as *mut JsLiteral;
                 if(lit.value.size() >= 2 && (lit.value.get(0) == '"' || lit.value.get(0) == '\'' || lit.value.get(0) == '`')) {
                     converter.str.append_view(strip_js_string_quotes(lit.value));
                     converter.put_chain_in();
                     return;
                 }
             }
             // For other types, we might need a general way to render them to HTML
             // But for now let's stick to what we have.
        }
        default => {}
    }
}

// Splits a JS array literal text (`["One","Two"]`, `[1, 2, 3]`) into its
// top-level element texts, respecting quotes and nested brackets. Returns false
// when the text is not a well-formed array literal.
func split_js_array_elements(text : std::string_view, out : &mut std::vector<std::string_view>) : bool {
    var start : size_t = 0
    while(start < text.size() && (text.get(start) == ' ' || text.get(start) == '(' || text.get(start) == '\n' || text.get(start) == '\r' || text.get(start) == '\t')) start++
    if(start >= text.size() || text.get(start) != '[') return false
    var end : size_t = text.size()
    while(end > start && (text.get(end - 1) == ' ' || text.get(end - 1) == ')' || text.get(end - 1) == '\n' || text.get(end - 1) == '\r' || text.get(end - 1) == '\t')) end--
    if(end <= start || text.get(end - 1) != ']') return false
    start++
    end--
    var i = start
    var depth : int = 0
    var curStart = start
    while(i < end) {
        const c = text.get(i)
        if(c == '"' || c == '\'' || c == '`') {
            const quote = c
            i++
            while(i < end && text.get(i) != quote) {
                if(text.get(i) == '\\') i++
                i++
            }
            if(i < end) i++
            continue
        }
        if(c == '[' || c == '{') depth++
        else if(c == ']' || c == '}') { if(depth > 0) depth-- }
        else if(c == ',' && depth == 0) {
            out.push(text.subview(curStart, i))
            i++
            curStart = i
            continue
        }
        i++
    }
    if(curStart < end) out.push(text.subview(curStart, end))
    return true
}

// Builds a call to a `ssrMake*` runtime helper that returns an SsrAttributeValue.
// These ordinary function calls resolve their return type via the function
// declaration (unlike inline `SsrAttributeValue.X(...)` variant-constructor
// calls, which leave the call type unresolved when nested as reference-param
// arguments of other generated calls).
func (converter : &mut JsConverter) make_ssr_make_call(fnPtr : *mut ASTNode, fnName : std::string_view, arg : *mut Value) : *mut Value {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const call = builder.make_function_call_value(builder.make_identifier(&fnName, fnPtr, false, location), location)
    if(arg != null) call.get_args().push(arg)
    return call
}

// Converts a static JS element text (from a state array literal) into an
// SsrAttributeValue expression: quoted strings become Text, true/false become
// Boolean, integers become UInteger/Integer, and null/undefined become None.
func (converter : &mut JsConverter) build_ssr_element_value_from_text(text : std::string_view) : *mut Value {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    var t = text
    while(t.size() > 0 && (t.get(0) == ' ' || t.get(0) == '\n' || t.get(0) == '\t' || t.get(0) == '\r')) t = t.skip(1)
    while(t.size() > 0 && (t.get(t.size() - 1) == ' ' || t.get(t.size() - 1) == '\n' || t.get(t.size() - 1) == '\t' || t.get(t.size() - 1) == '\r')) t = t.subview(0, t.size() - 1)
    if(t.empty()) return null

    if(t.equals(view("true"))) return converter.make_ssr_make_call(support.ssrMakeBoolValueFn, "ssrMakeBoolValue", builder.make_bool_value(true, location))
    if(t.equals(view("false"))) return converter.make_ssr_make_call(support.ssrMakeBoolValueFn, "ssrMakeBoolValue", builder.make_bool_value(false, location))
    if(t.equals(view("null")) || t.equals(view("undefined"))) {
        return converter.make_ssr_make_call(support.ssrNoneValueFn, "ssrNoneValue", null)
    }

    const stripped = strip_js_string_quotes(t)
    if(stripped.size() < t.size()) {
        return converter.make_ssr_make_call(support.ssrMakeTextValueFn, "ssrMakeTextValue", make_ssr_text_val(builder, &stripped, support.ssrTextLinkedNode, location))
    }
    var num : bigint = 0
    if(parse_ssr_bigint(t, &mut num)) {
        if(num >= 0) return converter.make_ssr_make_call(support.ssrMakeUIntegerValueFn, "ssrMakeUIntegerValue", builder.make_ubigint_value(num as ubigint, location))
        return converter.make_ssr_make_call(support.ssrMakeIntegerValueFn, "ssrMakeIntegerValue", builder.make_bigint_value(num, location))
    }
    // Fallback: render the raw text (unquoted) as a Text value
    return converter.make_ssr_make_call(support.ssrMakeTextValueFn, "ssrMakeTextValue", make_ssr_text_val(builder, &t, support.ssrTextLinkedNode, location))
}

// Builds a `Multiple` SsrAttributeValue from a static JS array literal text
// (a `state items = [...]` initializer), so `.map()`/`.length` over state
// arrays resolve at SSR time. Returns null when the text is not an array.
func (converter : &mut JsConverter) build_ssr_multiple_from_array_text(initText : std::string_view) : *mut Value {
    var elements = std::vector<std::string_view>()
    if(!split_js_array_elements(initText, &mut elements)) return null
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    var attrValueType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location)
    var ssrAttrValArr = builder.make_array_value(attrValueType, location)
    var arrValues = ssrAttrValArr.get_values()
    for(var i : uint = 0; i < elements.size(); i++) {
        const elemVal = converter.build_ssr_element_value_from_text(elements.get(i))
        if(elemVal != null) arrValues.push(elemVal)
    }
    const multiAttrStructVal = builder.make_struct_value(support.multipleAttributeValueNode, location)
    multiAttrStructVal.add_value("data", ssrAttrValArr)
    multiAttrStructVal.add_value("size", builder.make_ubigint_value(arrValues.size(), location))
    return converter.make_ssr_make_call(support.ssrMakeMultipleValueFn, "ssrMakeMultipleValue", multiAttrStructVal)
}

// Builds a `Multiple` SsrAttributeValue from an inline JS array literal node
// (`{[1, 2].map(...)}`), converting each element through the standard SSR
// expression evaluator. Returns null when no element is convertible.
func (converter : &mut JsConverter) build_ssr_multiple_from_array_node(node : *mut JsArrayLiteral) : *mut Value {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support
    var attrValConv = converter.make_attr_value_converter()

    var attrValueType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location)
    var ssrAttrValArr = builder.make_array_value(attrValueType, location)
    var arrValues = ssrAttrValArr.get_values()
    for(var i : uint = 0; i < node.elements.size(); i++) {
        const elemVal = converter.convert_ssr_attr_value_expr(node.elements.get(i), &mut attrValConv)
        if(elemVal != null) arrValues.push(elemVal)
    }
    if(arrValues.size() == 0) return null
    const multiAttrStructVal = builder.make_struct_value(support.multipleAttributeValueNode, location)
    multiAttrStructVal.add_value("data", ssrAttrValArr)
    multiAttrStructVal.add_value("size", builder.make_ubigint_value(arrValues.size(), location))
    return converter.make_ssr_make_call(support.ssrMakeMultipleValueFn, "ssrMakeMultipleValue", multiAttrStructVal)
}

// Renders the element count of an array expression (`props.items.length`,
// `items.length`, `items.size`) as text, matching the hydrated DOM. Returns
// false when the object is not a representable array source.
func (converter : &mut JsConverter) emit_ssr_array_count(object : *mut JsNode) : bool {
    if(object == null) return false
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    var sourceVal : *mut Value = null
    if(object.kind == JsNodeKind.MemberAccess && converter.is_component_props_read(object)) {
        sourceVal = converter.make_ssr_prop_v_call((object as *mut JsMemberAccess).property)
    } else if(object.kind == JsNodeKind.Identifier) {
        const id = object as *mut JsIdentifier
        if(converter.is_reactive_var(id.value)) {
            // Static state array: count the elements at compile time.
            const initText = converter.find_state_init_text(id.value)
            var elements = std::vector<std::string_view>()
            if(!initText.empty() && split_js_array_elements(initText, &mut elements)) {
                converter.vec.push(converter.make_uinteger_value_call(builder.make_ubigint_value(elements.size() as ubigint, location)))
                return true
            }
            return false
        } else {
            const local = converter.find_ssr_local(id.value)
            if(local != null) {
                sourceVal = builder.make_identifier(&local.name, local.varInit, false, location)
            }
        }
    }
    if(sourceVal == null) return false

    // page.append_html_uinteger(getMultipleAttributeValues(source).size)
    const getMultiCall = builder.make_function_call_value(builder.make_identifier("getMultipleAttributeValues", support.getMultipleAttributeValuesFn, false, location), location)
    getMultiCall.get_args().push(sourceVal)
    const getMultiVal : *mut Value = getMultiCall
    const sizeNode = support.multipleAttributeValueNode.child("size")
    const sizeId = builder.make_identifier("size", sizeNode, false, location)
    const sizeAccess = builder.make_access_chain(&std::span<*mut Value>([ getMultiVal, sizeId ]), location)
    converter.vec.push(converter.make_uinteger_value_call(sizeAccess))
    return true
}

// Converts a `.map()` callback child of a component's returned JSX into SSR
// output, matching the client's initial render. Static array sources (state
// array literals, inline array literals) are unrolled at compile time with the
// callback parameter statically bound, so `{item + 1}` style expressions
// evaluate numerically. Runtime sources (props arrays, component-body locals)
// emit a Chemical for-loop that iterates the array's MultipleAttributeValues
// and renders each element through the callback body.
func (converter : &mut JsConverter) emit_ssr_map_children(call : *mut JsFunctionCall) {
    if(call == null || call.callee == null) return
    if(call.callee.kind != JsNodeKind.MemberAccess) return
    const mem = call.callee as *mut JsMemberAccess
    if(!mem.property.equals(view("map"))) return
    if(call.args.empty()) return
    const arg0 = call.args.get(0)
    if(arg0 == null || arg0.kind != JsNodeKind.ArrowFunction) return
    const arrow = arg0 as *mut JsArrowFunction
    if(arrow.params.empty()) return
    const paramName = arrow.params.get(0).name
    if(paramName.empty()) return
    // Optional second parameter (`(item, index) => ...`). Bound statically for
    // compile-time-unrolled sources, or as an SSR local for runtime loops.
    var indexParam = std::string_view("")
    if(arrow.params.size() >= 2) {
        indexParam = arrow.params.get(1).name
    }

    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    // Static element texts (state array literal / inline array literal). When
    // non-empty, the map is unrolled at compile time.
    var staticElements = std::vector<std::string_view>()
    // Runtime array source expression (props read / local variable).
    var sourceVal : *mut Value = null

    if(mem.object != null) {
        if(mem.object.kind == JsNodeKind.MemberAccess && converter.is_component_props_read(mem.object)) {
            sourceVal = converter.make_ssr_prop_v_call((mem.object as *mut JsMemberAccess).property)
        } else if(mem.object.kind == JsNodeKind.Identifier) {
            const id = mem.object as *mut JsIdentifier
            if(converter.is_reactive_var(id.value)) {
                const initText = converter.find_state_init_text(id.value)
                if(!initText.empty()) {
                    split_js_array_elements(initText, &mut staticElements)
                }
            } else {
                const local = converter.find_ssr_local(id.value)
                if(local != null) {
                    sourceVal = builder.make_identifier(&local.name, local.varInit, false, location)
                }
            }
        } else if(mem.object.kind == JsNodeKind.ArrayLiteral) {
            const arr = mem.object as *mut JsArrayLiteral
            for(var ai : uint = 0; ai < arr.elements.size(); ai++) {
                staticElements.push(build_js_node_text_view(builder, arr.elements.get(ai)))
            }
        }
    }

    // Compile-time unroll for static sources: bind the callback param to each
    // element value and convert the body. This lets {item + 1} over [1,2,3]
    // evaluate numerically (2,3,4) exactly like the hydrated DOM.
    if(!staticElements.empty()) {
        const oldParam = converter.ssr_bound_param
        const oldValid = converter.ssr_bound_param_valid
        const oldValue = converter.ssr_bound_param_value
        const oldIdxParam = converter.ssr_index_param
        const oldIdxValid = converter.ssr_index_param_valid
        const oldIdxValue = converter.ssr_index_param_value
        for(var si : uint = 0; si < staticElements.size(); si++) {
            const ev = ssr_js_eval_from_text(staticElements.get(si))
            // Unresolvable elements (object literals, expressions referencing
            // runtime values) render nothing — matching the pre-SSR behavior and
            // avoiding empty wrappers that would mismatch the hydrated DOM.
            if(!ev.valid) continue
            converter.ssr_bound_param = paramName
            converter.ssr_bound_param_valid = true
            converter.ssr_bound_param_value = ev
            if(!indexParam.empty()) {
                var idxText = std::string()
                idxText.append_uinteger(si as ubigint)
                const idxView = builder.allocate_view(idxText.to_view())
                converter.ssr_index_param = indexParam
                converter.ssr_index_param_valid = true
                converter.ssr_index_param_value = ssr_js_eval_number(si as bigint, idxView)
            }
            if(arrow.body != null) {
                if(arrow.body.kind == JsNodeKind.Block) {
                    const block = arrow.body as *mut JsBlock
                    for(var bi : uint = 0; bi < block.statements.size(); bi++) {
                        converter.emit_ssr_single_stmt(block.statements.get(bi), null)
                    }
                } else {
                    converter.convert_jsx_ssr_expression(arrow.body)
                }
            }
        }
        converter.ssr_bound_param = oldParam
        converter.ssr_bound_param_valid = oldValid
        converter.ssr_bound_param_value = oldValue
        converter.ssr_index_param = oldIdxParam
        converter.ssr_index_param_valid = oldIdxValid
        converter.ssr_index_param_value = oldIdxValue
        return
    }

    if(sourceVal == null) return

    // Runtime for-loop over the array source.
    // Unique temporary names for this map (a component may contain several).
    var srcNameS = std::string("__ssr_ms_")
    srcNameS.append_uinteger(converter.id_counter as ubigint)
    converter.id_counter++
    const srcName = builder.allocate_view(srcNameS.to_view())
    var idxNameS = std::string("__ssr_mi_")
    idxNameS.append_uinteger(converter.id_counter as ubigint)
    converter.id_counter++
    const idxName = builder.allocate_view(idxNameS.to_view())

    // var src = getMultipleAttributeValues(sourceVal)
    const getMultiCall = builder.make_function_call_value(builder.make_identifier("getMultipleAttributeValues", support.getMultipleAttributeValuesFn, false, location), location)
    getMultiCall.get_args().push(sourceVal)
    const srcType = builder.make_linked_type("MultipleAttributeValues", support.multipleAttributeValueNode, location)
    const srcVar = builder.make_varinit_stmt(false, false, &srcName, srcType, getMultiCall, AccessSpecifier.Internal, converter.parent, location)
    converter.vec.push(srcVar)

    // for(var i = 0u; i < src.size; i = i + 1)
    const iType = builder.get_u64_type()
    const iVar = builder.make_varinit_stmt(false, false, &idxName, iType, builder.make_ubigint_value(0, location), AccessSpecifier.Internal, converter.parent, location)
    const srcId = builder.make_identifier(&srcName, srcVar, false, location)
    const sizeNode = support.multipleAttributeValueNode.child("size")
    const sizeId = builder.make_identifier("size", sizeNode, false, location)
    const sizeAccess = builder.make_access_chain(&std::span<*mut Value>([ srcId, sizeId ]), location)
    const iId = builder.make_identifier(&idxName, iVar, false, location)
    const cond = builder.make_expression_value(iId, sizeAccess, Operation.LessThan, builder.make_bool_type(), location)
    const oneVal = builder.make_ubigint_value(1, location)
    const addExpr = builder.make_expression_value(iId, oneVal, Operation.Addition, iType, location)
    const incr = builder.make_assignment_stmt(iId, addExpr, Operation.Assignment, converter.parent, location)

    const forLoop = builder.make_for_loop(iVar, cond, incr, converter.parent, location)
    const oldVec = converter.vec
    converter.vec = forLoop.get_body()

    // var <param> = ssrMultipleGet(src, i)
    // A call (rather than a raw `src.data[i]` index op) so the value is fully
    // type-resolved: generated SSR function bodies bypass symres type
    // determination, which left the index operator's type null and crashed
    // LLVM codegen (the TCC path tolerated it because the explicit varinit
    // type drives the C translation).
    const getCall = builder.make_function_call_value(builder.make_identifier("ssrMultipleGet", support.ssrMultipleGetFn, false, location), location)
    getCall.get_args().push(srcId)
    getCall.get_args().push(iId)
    const elemType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location)
    const elemVar = builder.make_varinit_stmt(false, false, &paramName, elemType, getCall, AccessSpecifier.Internal, converter.parent, location)
    converter.vec.push(elemVar)

    // Bind the callback param as an SSR local while converting the body, so
    // {item} / {item + 1} references resolve to the current loop element.
    converter.ssr_locals.push(JsSsrLocal { name : paramName, varInit : elemVar })

    // Bind the optional second param (`index`) to the loop counter, wrapped as
    // an SsrAttributeValue so `active == i` comparisons resolve at SSR runtime.
    var indexVarPushed = false
    if(!indexParam.empty()) {
        const idxType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location)
        const idxMake = converter.make_ssr_make_call(support.ssrMakeUIntegerValueFn, "ssrMakeUIntegerValue", iId)
        const idxVar = builder.make_varinit_stmt(false, false, &indexParam, idxType, idxMake, AccessSpecifier.Internal, converter.parent, location)
        converter.vec.push(idxVar)
        converter.ssr_locals.push(JsSsrLocal { name : indexParam, varInit : idxVar })
        indexVarPushed = true
    }

    if(arrow.body != null) {
        if(arrow.body.kind == JsNodeKind.Block) {
            const block = arrow.body as *mut JsBlock
            for(var bi : uint = 0; bi < block.statements.size(); bi++) {
                converter.emit_ssr_single_stmt(block.statements.get(bi), null)
            }
        } else {
            converter.convert_jsx_ssr_expression(arrow.body)
        }
    }

    if(indexVarPushed) converter.ssr_locals.pop_back()
    converter.ssr_locals.pop_back()
    converter.vec = oldVec
    converter.vec.push(forLoop as *mut ASTNode)
}

// Appends a statically evaluated JS expression (SsrJsExprEval) as HTML text.
// Booleans render nothing, matching React's JSX children semantics (which the
// runtime renderHtmlChildValue also mirrors).
func (converter : &mut JsConverter) append_ssr_eval(eval : SsrJsExprEval) {
    if(!eval.valid) return
    if(eval.kind == 1) return
    converter.str.append_view(&eval.textValue)
    converter.put_chain_in()
}

// Resolves a context-member read (`ctx.value`, `ctx.mode`, ...) to the value it
// renders at SSR time. A provider's SSR function runs AFTER its children render
// (children HTML is pre-rendered and passed in), so a consumer can never observe
// a published value at SSR - reads resolve to the static createContext default
// (or None for useContext without a default). Returns null when unresolvable.
func (converter : &mut JsConverter) convert_ssr_context_member_read(mem : *mut JsMemberAccess, attrValConv : &mut AttrValueConverter) : *mut Value {
    if(mem == null || mem.object == null || mem.object.kind != JsNodeKind.Identifier) return null;
    const cv = converter.find_context_var((mem.object as *mut JsIdentifier).value);
    if(cv == null) return null;
    if(mem.property.equals(view("value")) && cv.defaultExpr != null) {
        return converter.convert_ssr_attr_value_expr(cv.defaultExpr, attrValConv);
    }
    // Non-value members and useContext vars without a default are undefined at
    // SSR, matching the pre-provider client state (undefined == anything is
    // false, so items render unpressed/unchecked).
    return converter.make_ssr_make_call(converter.support.ssrNoneValueFn, "ssrNoneValue", null);
}

func (converter : &mut JsConverter) convert_js_expr_to_ssr_bool_value(node : *mut JsNode) : *mut Value {
    if(node == null) return null;

    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    if(node.kind == JsNodeKind.Paren) {
        return converter.convert_js_expr_to_ssr_bool_value((node as *mut JsParen).expression);
    }

    if(node.kind == JsNodeKind.MemberAccess) {
        if(converter.is_component_props_read(node)) {
            const mem = node as *mut JsMemberAccess;
            const v = converter.make_ssr_prop_v_call(mem.property);
            const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
            truthyCall.get_args().push(v);
            return truthyCall as *mut Value;
        }
        const cvMem = node as *mut JsMemberAccess;
        if(cvMem.object != null && cvMem.object.kind == JsNodeKind.Identifier && converter.is_context_var((cvMem.object as *mut JsIdentifier).value)) {
            var attrValConv = converter.make_attr_value_converter();
            const v = converter.convert_ssr_context_member_read(cvMem, &mut attrValConv);
            if(v != null) {
                const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
                truthyCall.get_args().push(v);
                return truthyCall as *mut Value;
            }
            return null;
        }
    }

    if(node.kind == JsNodeKind.Literal) {
        var lit = node as *mut JsLiteral;
        if(lit.value.equals(view("true"))) return builder.make_bool_value(true, location) as *mut Value;
        if(lit.value.equals(view("false"))) return builder.make_bool_value(false, location) as *mut Value;
    }

    if(node.kind == JsNodeKind.Identifier) {
        // Reference to a local variable declared in the component body.
        const id = node as *mut JsIdentifier;
        const local = converter.find_ssr_local(id.value);
        if(local != null) {
            const localRef = builder.make_identifier(&local.name, local.varInit, false, location);
            const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
            truthyCall.get_args().push(localRef);
            return truthyCall as *mut Value;
        }
    }

    if(node.kind == JsNodeKind.UnaryOp) {
        var unary = node as *mut JsUnaryOp;
        if(unary.operator.equals(view("!"))) {
            const operandVal = converter.convert_js_expr_to_ssr_bool_value(unary.operand);
            if(operandVal != null) {
                return builder.make_not_value(operandVal, location) as *mut Value;
            }
        }
    }

    if(node.kind == JsNodeKind.BinaryOp) {
        var bin = node as *mut JsBinaryOp;
        const isEq = bin.op.equals(view("==")) || bin.op.equals(view("==="));
        const isNe = bin.op.equals(view("!=")) || bin.op.equals(view("!=="));
        if(isEq || isNe) {
            var attrValConv = AttrValueConverter {
                pageNode : support.pageNode,
                ssrTextNode : support.ssrTextLinkedNode,
                ssrAttributeValueNode : support.ssrAttributeValueNode,
                multipleAttributeValueNode : support.multipleAttributeValueNode,
                parent : converter.parent
            }
            const leftVal = converter.convert_ssr_attr_value_expr(bin.left, &mut attrValConv);
            if(leftVal == null) return null;
            // Literal right side: compare against its text (fast path).
            if(bin.right != null && bin.right.kind == JsNodeKind.Literal) {
                const litText = strip_js_string_quotes((bin.right as *mut JsLiteral).value);
                const cmpCall = builder.make_function_call_value(builder.make_identifier("ssrTextEquals", support.ssrTextEqualsFn, false, location), location);
                cmpCall.get_args().push(leftVal);
                cmpCall.get_args().push(converter.make_ssr_text(&litText, location));
                if(isNe) return builder.make_not_value(cmpCall as *mut Value, location) as *mut Value;
                return cmpCall as *mut Value;
            }
            // Value-to-value comparison (`active == index`, `page == item`):
            // both sides convert to SsrAttributeValue expressions.
            const rightVal = converter.convert_ssr_attr_value_expr(bin.right, &mut attrValConv);
            if(rightVal == null) return null;
            const eqCall = builder.make_function_call_value(builder.make_identifier("ssrValuesEqual", support.ssrValuesEqualFn, false, location), location);
            eqCall.get_args().push(leftVal);
            eqCall.get_args().push(rightVal);
            if(isNe) return builder.make_not_value(eqCall as *mut Value, location) as *mut Value;
            return eqCall as *mut Value;
        }
        // Logical && / ||: combine the operand bool values.
        if(bin.op.equals(view("&&"))) {
            const leftBool = converter.convert_js_expr_to_ssr_bool_value(bin.left);
            const rightBool = converter.convert_js_expr_to_ssr_bool_value(bin.right);
            if(leftBool == null || rightBool == null) return null;
            return builder.make_expression_value(leftBool, rightBool, Operation.LogicalAND, builder.make_bool_type(), location) as *mut Value;
        }
        if(bin.op.equals(view("||"))) {
            const leftBool = converter.convert_js_expr_to_ssr_bool_value(bin.left);
            const rightBool = converter.convert_js_expr_to_ssr_bool_value(bin.right);
            if(leftBool == null || rightBool == null) return null;
            return builder.make_expression_value(leftBool, rightBool, Operation.LogicalOR, builder.make_bool_type(), location) as *mut Value;
        }
    }

    return null;
}

func is_event_attribute_name(name : std::string_view) : bool {
    return name.size() > 2 && name.get(0) == 'o' && name.get(1) == 'n';
}

func is_client_only_attribute_name(name : std::string_view) : bool {
    // These attributes contain JS references (functions, state) and cannot be SSR'd
    return name.equals("ref") || name.equals("dangerouslySetInnerHTML");
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
    // `null`/`undefined` must not render as the literal text "null" (and the
    // no-arg None variant cannot be built with wrapArgAttrValueVariantCall).
    if(val.equals("null") || val.equals("undefined")) {
        return converter.make_ssr_make_call(converter.support.ssrNoneValueFn, "ssrNoneValue", null);
    }
    
    const text = strip_js_string_quotes(val);
    return attrValConv.wrapArgAttrValueVariantCall(builder, "Text", converter.make_ssr_text(&text, location));
}

// Builds a Chemical bool expression that evaluates a props-driven JS expression
// at SSR runtime. Returns null when the expression cannot be evaluated at SSR time.
func (converter : &mut JsConverter) convert_ssr_attr_bool_expr(node : *mut JsNode, attrValConv : &mut AttrValueConverter) : *mut Value {
    if(node == null) return null;
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    switch(node.kind) {
        JsNodeKind.Literal => {
            const lit = node as *mut JsLiteral;
            if(lit.value.equals(view("true"))) return builder.make_bool_value(true, location) as *mut Value;
            if(lit.value.equals(view("false"))) return builder.make_bool_value(false, location) as *mut Value;
            return null;
        }
        JsNodeKind.Paren => {
            return converter.convert_ssr_attr_bool_expr((node as *mut JsParen).expression, attrValConv);
        }
        JsNodeKind.UnaryOp => {
            const unary = node as *mut JsUnaryOp;
            if(unary.operator.equals(view("!"))) {
                const operand = converter.convert_ssr_attr_bool_expr(unary.operand, attrValConv);
                if(operand != null) return builder.make_not_value(operand, location) as *mut Value;
            }
            return null;
        }
        JsNodeKind.Identifier => {
            // Reference to a local variable declared in the component body.
            const id = node as *mut JsIdentifier;
            const local = converter.find_ssr_local(id.value);
            if(local != null) {
                const localRef = builder.make_identifier(&local.name, local.varInit, false, location);
                const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
                truthyCall.get_args().push(localRef);
                return truthyCall as *mut Value;
            }
            return null;
        }
        JsNodeKind.MemberAccess => {
            if(converter.is_component_props_read(node)) {
                const v = converter.make_ssr_prop_v_call((node as *mut JsMemberAccess).property);
                const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
                truthyCall.get_args().push(v);
                return truthyCall as *mut Value;
            }
            const cvMem = node as *mut JsMemberAccess;
            if(cvMem.object != null && cvMem.object.kind == JsNodeKind.Identifier && converter.is_context_var((cvMem.object as *mut JsIdentifier).value)) {
                const v = converter.convert_ssr_context_member_read(cvMem, attrValConv);
                if(v != null) {
                    const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
                    truthyCall.get_args().push(v);
                    return truthyCall as *mut Value;
                }
                return null;
            }
            return null;
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            const isEq = bin.op.equals(view("==")) || bin.op.equals(view("==="));
            const isNe = bin.op.equals(view("!=")) || bin.op.equals(view("!=="));
            if(isEq || isNe) {
                const leftVal = converter.convert_ssr_attr_value_expr(bin.left, attrValConv);
                if(leftVal == null) return null;
                // Literal right side: compare against its text (fast path).
                if(bin.right != null && bin.right.kind == JsNodeKind.Literal) {
                    const litText = strip_js_string_quotes((bin.right as *mut JsLiteral).value);
                    const cmpCall = builder.make_function_call_value(builder.make_identifier("ssrTextEquals", support.ssrTextEqualsFn, false, location), location);
                    cmpCall.get_args().push(leftVal);
                    cmpCall.get_args().push(converter.make_ssr_text(&litText, location));
                    if(isNe) return builder.make_not_value(cmpCall as *mut Value, location) as *mut Value;
                    return cmpCall as *mut Value;
                }
                // Value-to-value comparison (`active == index`, `page == item`):
                // both sides convert to SsrAttributeValue expressions.
                const rightVal = converter.convert_ssr_attr_value_expr(bin.right, attrValConv);
                if(rightVal == null) return null;
                const eqCall = builder.make_function_call_value(builder.make_identifier("ssrValuesEqual", support.ssrValuesEqualFn, false, location), location);
                eqCall.get_args().push(leftVal);
                eqCall.get_args().push(rightVal);
                if(isNe) return builder.make_not_value(eqCall as *mut Value, location) as *mut Value;
                return eqCall as *mut Value;
            }
            // Logical && / ||: combine the operand bool values.
            if(bin.op.equals(view("&&"))) {
                const leftBool = converter.convert_ssr_attr_bool_expr(bin.left, attrValConv);
                const rightBool = converter.convert_ssr_attr_bool_expr(bin.right, attrValConv);
                if(leftBool == null || rightBool == null) return null;
                return builder.make_expression_value(leftBool, rightBool, Operation.LogicalAND, builder.make_bool_type(), location) as *mut Value;
            }
            if(bin.op.equals(view("||"))) {
                const leftBool = converter.convert_ssr_attr_bool_expr(bin.left, attrValConv);
                const rightBool = converter.convert_ssr_attr_bool_expr(bin.right, attrValConv);
                if(leftBool == null || rightBool == null) return null;
                return builder.make_expression_value(leftBool, rightBool, Operation.LogicalOR, builder.make_bool_type(), location) as *mut Value;
            }
            return null;
        }
        default => return null
    }
}

// Looks up a local variable declared in the current universal component body
// (see emit_ssr_local_decl). Returns null when the name is not a known local.
func (converter : &mut JsConverter) find_ssr_local(name : std::string_view) : *JsSsrLocal {
    for(var i : uint = 0; i < converter.ssr_locals.size(); i++) {
        if(converter.ssr_locals.get_ptr(i).name.equals(&name)) {
            return converter.ssr_locals.get_ptr(i);
        }
    }
    return null;
}

// Builds an AttrValueConverter for the current converter context.
func (converter : &mut JsConverter) make_attr_value_converter() : AttrValueConverter {
    const support = converter.support;
    return AttrValueConverter {
        pageNode : support.pageNode,
        ssrTextNode : support.ssrTextLinkedNode,
        ssrAttributeValueNode : support.ssrAttributeValueNode,
        multipleAttributeValueNode : support.multipleAttributeValueNode,
        parent : converter.parent
    }
}

// Converts a component-body `var` declaration into a Chemical SsrAttributeValue
// local in the SSR function and records it in ssr_locals so JSX attribute/child
// expressions can reference it ({variant} etc). Returns false when the
// declaration cannot be represented at SSR time.
func (converter : &mut JsConverter) emit_ssr_local_decl(decl : *mut JsVarDecl) : bool {
    if(decl == null || decl.name.empty()) return false;
    if(converter.find_ssr_local(decl.name) != null) return false;

    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    var attrValConv = converter.make_attr_value_converter();

    var init : *mut Value = null;
    if(decl.value != null) {
        if(decl.value.kind == JsNodeKind.ChemicalValue) {
            const chem = decl.value as *mut JsChemicalValue;
            if(chem.value != null) {
                init = attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value);
            }
        } else {
            init = converter.convert_ssr_attr_value_expr(decl.value, &mut attrValConv);
        }
    }
    if(init == null) return false;

    const varType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location);
    const varInit = builder.make_varinit_stmt(false, false, &decl.name, varType, init, AccessSpecifier.Internal, converter.parent, location);
    converter.vec.push(varInit);

    var local = JsSsrLocal {
        name : decl.name,
        varInit : varInit
    }
    converter.ssr_locals.push(local);
    return true;
}

// Converts a component-body assignment statement (`x = expr`) into a Chemical
// assignment targeting a known SSR local. Returns false when unsupported.
func (converter : &mut JsConverter) emit_ssr_assignment_stmt(expr : *mut JsNode) : bool {
    if(expr == null || expr.kind != JsNodeKind.BinaryOp) return false;
    const bin = expr as *mut JsBinaryOp;
    if(!bin.op.equals(view("="))) return false;
    // Context publish (`ctx.value = x`, `ctx.write = fn`) is a no-op at SSR:
    // consumers render before the provider's SSR function runs, so nothing can
    // observe the published value.
    if(bin.left != null && bin.left.kind == JsNodeKind.MemberAccess) {
        const mem = bin.left as *mut JsMemberAccess;
        if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && converter.is_context_var((mem.object as *mut JsIdentifier).value)) {
            return true;
        }
    }
    if(bin.left == null || bin.left.kind != JsNodeKind.Identifier) return false;
    const id = bin.left as *mut JsIdentifier;
    const local = converter.find_ssr_local(id.value);
    if(local == null) return false;

    const builder = converter.builder;
    const location = intrinsics::get_raw_location();

    var attrValConv = converter.make_attr_value_converter();

    var rhs : *mut Value = null;
    if(bin.right != null) {
        if(bin.right.kind == JsNodeKind.ChemicalValue) {
            const chem = bin.right as *mut JsChemicalValue;
            if(chem.value != null) {
                rhs = attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value);
            }
        } else {
            rhs = converter.convert_ssr_attr_value_expr(bin.right, &mut attrValConv);
        }
    }
    if(rhs == null) return false;

    const lhs = builder.make_identifier(&local.name, local.varInit, false, location);
    const assign = builder.make_assignment_stmt(lhs, rhs, Operation.Assignment, converter.parent, location);
    converter.vec.push(assign as *mut ASTNode);
    return true;
}

// Emits a component-body `if` statement (with else/else-if chains) into the SSR
// function. Conditions and assigned bodies are converted to Chemical.
func (converter : &mut JsConverter) emit_ssr_if_stmt(ifNode : *mut JsIf, skipReturnValue : *mut JsNode) {
    if(ifNode == null) return;

    const cond = converter.convert_js_expr_to_ssr_bool_value(ifNode.condition);
    if(cond == null) return;

    const builder = converter.builder;
    const location = intrinsics::get_raw_location();

    const ifStmt = builder.make_if_stmt(cond, converter.parent, location);
    const oldVec = converter.vec;

    if(ifNode.thenBlock != null) {
        converter.vec = ifStmt.get_body();
        if(ifNode.thenBlock.kind == JsNodeKind.Block) {
            converter.emit_ssr_body_statements(ifNode.thenBlock as *mut JsBlock, skipReturnValue);
        } else {
            converter.emit_ssr_single_stmt(ifNode.thenBlock, skipReturnValue);
        }
    }

    if(ifNode.elseBlock != null) {
        converter.vec = ifStmt.add_else_body();
        if(ifNode.elseBlock.kind == JsNodeKind.If) {
            // else-if chain
            converter.emit_ssr_if_stmt(ifNode.elseBlock as *mut JsIf, skipReturnValue);
        } else if(ifNode.elseBlock.kind == JsNodeKind.Block) {
            converter.emit_ssr_body_statements(ifNode.elseBlock as *mut JsBlock, skipReturnValue);
        } else {
            converter.emit_ssr_single_stmt(ifNode.elseBlock, skipReturnValue);
        }
    }

    converter.vec = oldVec;
    converter.vec.push(ifStmt as *mut ASTNode);
}

// Emits a `return <JSX>` statement found inside a conditional body into the
// current SSR output. `null`/empty returns render nothing. Returns true when
// the statement was handled.
func (converter : &mut JsConverter) emit_ssr_return_stmt(ret : *mut JsReturn) : bool {
    if(ret == null) return false;
    if(ret.value == null) return true;

    const value = unwrap_returned_jsx_node(ret.value);
    if(value == null) return true;
    if(value.kind == JsNodeKind.JSXElement || value.kind == JsNodeKind.JSXFragment) {
        converter.convertJsNode(value);
        return true;
    }
    // Non-JSX return value (identifier, ternary, etc.) — try the general path.
    converter.convert_jsx_ssr_expression(ret.value);
    return true;
}

// Emits a single non-block component body statement into the SSR function.
func (converter : &mut JsConverter) emit_ssr_single_stmt(stmt : *mut JsNode, skipReturnValue : *mut JsNode) {
    if(stmt == null) return;
    switch(stmt.kind) {
        JsNodeKind.VarDecl => {
            // Plain var/let/const declarations become SSR locals. `state`
            // declarations with statically- or props-driven-convertible
            // initializers are also registered (evaluated from their init) so
            // conditions like `active == index` resolve at SSR runtime.
            const decl = stmt as *mut JsVarDecl;
            if(decl.keyword.equals(view("var")) || decl.keyword.equals(view("let")) || decl.keyword.equals(view("const")) || decl.keyword.equals(view("state"))) {
                converter.emit_ssr_local_decl(stmt as *mut JsVarDecl);
            }
        }
        JsNodeKind.If => {
            converter.emit_ssr_if_stmt(stmt as *mut JsIf, skipReturnValue);
        }
        JsNodeKind.Return => {
            // The final component return is emitted separately by the caller;
            // only conditional returns (inside if/else bodies) render here.
            const ret = stmt as *mut JsReturn;
            if(skipReturnValue != null && ret.value != null) {
                const unwrapped = unwrap_returned_jsx_node(ret.value);
                if(unwrapped == skipReturnValue) return;
            }
            converter.emit_ssr_return_stmt(stmt as *mut JsReturn);
        }
        JsNodeKind.ExpressionStatement => {
            const es = stmt as *mut JsExpressionStatement;
            converter.emit_ssr_assignment_stmt(es.expression);
        }
        default => {}
    }
}

// Returns true when the `if` body ends in a `return <JSX>` — the pattern used
// for component dispatch (`if(tag == "span") { return <span/> }`). Such ifs are
// chained as if/else-if with the trailing return as the else branch, because JS
// `return` stops execution while a plain `if` does not.
func is_return_jsx_if(ifNode : *mut JsIf) : bool {
    if(ifNode == null || ifNode.elseBlock != null) return false;
    const tb = ifNode.thenBlock;
    if(tb == null) return false;
    if(tb.kind == JsNodeKind.Return) {
        return return_has_jsx(tb as *mut JsReturn);
    }
    if(tb.kind == JsNodeKind.Block) {
        const block = tb as *mut JsBlock;
        if(block.statements.empty()) return false;
        const last = block.statements.get(block.statements.size() - 1);
        return last != null && last.kind == JsNodeKind.Return && return_has_jsx(last as *mut JsReturn);
    }
    return false;
}

func return_has_jsx(ret : *mut JsReturn) : bool {
    if(ret == null || ret.value == null) return false;
    const v = unwrap_returned_jsx_node(ret.value);
    return v != null && (v.kind == JsNodeKind.JSXElement || v.kind == JsNodeKind.JSXFragment);
}

// Renders the body of a conditional-return branch (a `return <JSX>` or a block
// ending in one) into the current SSR output.
func (converter : &mut JsConverter) emit_return_jsx_body(body : *mut JsNode) {
    if(body == null) return;
    if(body.kind == JsNodeKind.Block) {
        const block = body as *mut JsBlock;
        for(var i : uint = 0; i < block.statements.size(); i++) {
            converter.emit_ssr_single_stmt(block.statements.get(i), null);
        }
    } else {
        converter.emit_ssr_single_stmt(body, null);
    }
}

// Emits an if/else-if/else chain for consecutive `if(c){ return <JSX> }`
// statements plus an optional trailing return (the else branch), so exactly one
// branch renders — matching JS `return` semantics. Falls back to independent
// ifs when a condition cannot be evaluated at SSR time.
func (converter : &mut JsConverter) emit_ssr_return_chain(ifs : &std::vector<*mut JsIf>, finalReturn : *mut JsReturn, skipReturnValue : *mut JsNode) {
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();

    if(ifs.empty()) {
        if(finalReturn != null) {
            converter.emit_ssr_return_stmt(finalReturn);
        }
        return;
    }

    // Build from the LAST if backwards so each previous if can attach the
    // already-built remainder as its else branch.
    var chainRoot : *mut IfStatement = null
    var i : int = ifs.size() as int - 1
    while(i >= 0) {
        const ifNode = ifs.get(i as uint)
        const cond = converter.convert_js_expr_to_ssr_bool_value(ifNode.condition)
        if(cond == null) {
            // Cannot SSR this condition: emit the chain built so far, then the
            // remaining ifs independently, then the trailing return.
            if(chainRoot != null) {
                converter.vec.push(chainRoot as *mut ASTNode)
            }
            for(var j : int = i; j >= 0; j--) {
                converter.emit_ssr_if_stmt(ifs.get(j as uint), skipReturnValue)
            }
            if(finalReturn != null) {
                converter.emit_ssr_return_stmt(finalReturn)
            }
            return
        }

        const ifStmt = builder.make_if_stmt(cond, converter.parent, location)
        const oldVec = converter.vec
        converter.vec = ifStmt.get_body()
        converter.emit_return_jsx_body(ifNode.thenBlock)
        if(chainRoot == null) {
            if(finalReturn != null) {
                converter.vec = ifStmt.add_else_body()
                converter.emit_ssr_return_stmt(finalReturn)
            }
        } else {
            converter.vec = ifStmt.add_else_body()
            converter.vec.push(chainRoot as *mut ASTNode)
        }
        converter.vec = oldVec
        chainRoot = ifStmt
        i--
    }
    converter.vec.push(chainRoot as *mut ASTNode)
}

// Emits all component body statements (var declarations, if/else chains,
// assignments, conditional returns) into the SSR function body, in order. This
// makes locals like `var variant = props.variant || "default"` available to the
// returned JSX and supports `if(tag == "span") { return <span/> }` dispatch.
// Returns true when the final component return was consumed as a chain else
// (the caller must not render it again).
func (converter : &mut JsConverter) emit_ssr_body_statements(block : *mut JsBlock, skipReturnValue : *mut JsNode) : bool {
    if(block == null) return false;
    var pending = std::vector<*mut JsIf>()
    var finalRendered = false
    for(var i : uint = 0; i < block.statements.size(); i++) {
        const stmt = block.statements.get(i);
        if(stmt == null) continue;
        if(stmt.kind == JsNodeKind.If && is_return_jsx_if(stmt as *mut JsIf)) {
            pending.push(stmt as *mut JsIf);
            continue;
        }
        if(stmt.kind == JsNodeKind.Return && !pending.empty()) {
            // Trailing return closes the chain as its else branch.
            converter.emit_ssr_return_chain(&pending, stmt as *mut JsReturn, skipReturnValue);
            pending.clear();
            finalRendered = true;
            continue;
        }
        // Non-chain statement: flush pending as independent ifs, then emit.
        converter.emit_ssr_return_chain(&pending, null, null);
        pending.clear();
        converter.emit_ssr_single_stmt(stmt, skipReturnValue);
    }
    converter.emit_ssr_return_chain(&pending, null, null);
    return finalRendered
}

// Builds a Chemical SsrAttributeValue expression that evaluates a props-driven
// JS expression at SSR runtime (ternary, ||, &&, + concatenation, props reads,
// local variable references). Returns null when the expression cannot be
// evaluated at SSR time.
func (converter : &mut JsConverter) convert_ssr_attr_value_expr(node : *mut JsNode, attrValConv : &mut AttrValueConverter) : *mut Value {
    if(node == null) return null;
    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    switch(node.kind) {
        JsNodeKind.Literal => {
            // Use the runtime wrapper helpers (regular calls) so the result can be
            // safely nested inside ssrPickValue and Multiple arrays.
            const lit = node as *mut JsLiteral;
            if(lit.value.equals(view("true"))) {
                const call = builder.make_function_call_value(builder.make_identifier("ssrMakeBoolValue", support.ssrMakeBoolValueFn, false, location), location);
                call.get_args().push(builder.make_bool_value(true, location));
                return call;
            }
            if(lit.value.equals(view("false"))) {
                const call = builder.make_function_call_value(builder.make_identifier("ssrMakeBoolValue", support.ssrMakeBoolValueFn, false, location), location);
                call.get_args().push(builder.make_bool_value(false, location));
                return call;
            }
            // `null`/`undefined` become None (falsy, renders nothing) so state
            // inits like `state x = null` don't render as the literal text
            // "null" and truthiness checks evaluate correctly.
            if(lit.value.equals(view("null")) || lit.value.equals(view("undefined"))) {
                const noneCall = builder.make_function_call_value(builder.make_identifier("ssrNoneValue", support.ssrNoneValueFn, false, location), location);
                return noneCall;
            }
            const text = strip_js_string_quotes(lit.value);
            const call = builder.make_function_call_value(builder.make_identifier("ssrMakeTextValue", support.ssrMakeTextValueFn, false, location), location);
            call.get_args().push(converter.make_ssr_text(&text, location));
            return call;
        }
        JsNodeKind.Paren => {
            return converter.convert_ssr_attr_value_expr((node as *mut JsParen).expression, attrValConv);
        }
        JsNodeKind.Identifier => {
            // Reference to a local variable declared earlier in the component body.
            const id = node as *mut JsIdentifier;
            const local = converter.find_ssr_local(id.value);
            if(local != null) {
                return builder.make_identifier(&local.name, local.varInit, false, location) as *mut Value;
            }
            return null;
        }
        JsNodeKind.MemberAccess => {
            if(converter.is_component_props_read(node)) {
                if(converter.is_props_children(node)) return null;
                return converter.make_ssr_prop_v_call((node as *mut JsMemberAccess).property);
            }
            const cvMem = node as *mut JsMemberAccess;
            if(cvMem.object != null && cvMem.object.kind == JsNodeKind.Identifier && converter.is_context_var((cvMem.object as *mut JsIdentifier).value)) {
                return converter.convert_ssr_context_member_read(cvMem, attrValConv);
            }
            return null;
        }
        JsNodeKind.ChemicalValue => {
            // A ${...} embed — the value is already a Chemical expression.
            const chem = node as *mut JsChemicalValue;
            if(chem.value == null) return null;
            return attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value);
        }
        JsNodeKind.BinaryOp => {
            const bin = node as *mut JsBinaryOp;
            if(bin.op.equals(view("+"))) {
                var parts = std::vector<*mut Value>();
                if(!converter.collect_ssr_concat_parts(bin as *mut JsNode, attrValConv, &mut parts)) return null;
                if(parts.empty()) return null;
                if(parts.size() == 1) return parts.get(0);

                var attrValueType = builder.make_linked_type("SsrAttributeValue", support.ssrAttributeValueNode, location)
                var partsArr = builder.make_array_value(attrValueType, location)
                var partsValues = partsArr.get_values();
                for(var i : uint = 0; i < parts.size(); i++) {
                    partsValues.push(parts.get(i));
                }

                const multiStruct = builder.make_struct_value(support.multipleAttributeValueNode, location)
                multiStruct.add_value("data", partsArr)
                multiStruct.add_value("size", builder.make_ubigint_value(parts.size() as ubigint, location))
                // Use the runtime helper (not an inline variant-constructor call) so
                // the result can be safely passed to `&SsrAttributeValue` params
                // (renderHtmlChildValue) without leaving the call type unresolved.
                return converter.make_ssr_make_call(support.ssrMakeMultipleValueFn, "ssrMakeMultipleValue", multiStruct);
            }            if(bin.op.equals(view("&&")) || bin.op.equals(view("||"))) {
                const left = converter.convert_ssr_attr_value_expr(bin.left, attrValConv);
                if(left == null) return null;
                const right = converter.convert_ssr_attr_value_expr(bin.right, attrValConv);
                if(right == null) return null;

                const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
                truthyCall.get_args().push(left);

                const noneCall = builder.make_function_call_value(builder.make_identifier("ssrNoneValue", support.ssrNoneValueFn, false, location), location);

                const pickCall = builder.make_function_call_value(builder.make_identifier("ssrPickValue", support.ssrPickValueFn, false, location), location);
                pickCall.get_args().push(truthyCall);
                if(bin.op.equals(view("||"))) {
                    // a || b : if a is truthy emit a, otherwise emit b
                    pickCall.get_args().push(left);
                    pickCall.get_args().push(right);
                } else {
                    // a && b : if a is truthy emit b, otherwise emit nothing
                    pickCall.get_args().push(right);
                    pickCall.get_args().push(noneCall);
                }
                return pickCall as *mut Value;

            }
            // Equality / inequality: return a Boolean SsrAttributeValue so
            // component-body vars like `var single = props.type == "multiple"`
            // register as SSR locals and ternaries over them evaluate at SSR
            // runtime (mirrors convert_ssr_attr_bool_expr).
            const isEq = bin.op.equals(view("==")) || bin.op.equals(view("==="));
            const isNe = bin.op.equals(view("!=")) || bin.op.equals(view("!=="));
            if(isEq || isNe) {
                const leftVal = converter.convert_ssr_attr_value_expr(bin.left, attrValConv);
                if(leftVal == null) return null;
                var cmpCall : *mut FunctionCall = null;
                if(bin.right != null && bin.right.kind == JsNodeKind.Literal) {
                    const litText = strip_js_string_quotes((bin.right as *mut JsLiteral).value);
                    cmpCall = builder.make_function_call_value(builder.make_identifier("ssrTextEquals", support.ssrTextEqualsFn, false, location), location);
                    cmpCall.get_args().push(leftVal);
                    cmpCall.get_args().push(converter.make_ssr_text(&litText, location));
                } else {
                    const rightVal = converter.convert_ssr_attr_value_expr(bin.right, attrValConv);
                    if(rightVal == null) return null;
                    cmpCall = builder.make_function_call_value(builder.make_identifier("ssrValuesEqual", support.ssrValuesEqualFn, false, location), location);
                    cmpCall.get_args().push(leftVal);
                    cmpCall.get_args().push(rightVal);
                }
                const boolVal = attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), cmpCall as *mut Value);
                if(isNe) {
                    const notVal = builder.make_not_value(cmpCall as *mut Value, location) as *mut Value;
                    return attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), notVal);
                }
                return boolVal;
            }
            return null;
        }
        JsNodeKind.Ternary => {
            const tern = node as *mut JsTernary;
            const cond = converter.convert_ssr_attr_bool_expr(tern.condition, attrValConv);
            if(cond == null) return null;
            const thenVal = converter.convert_ssr_attr_value_expr(tern.consequent, attrValConv);
            if(thenVal == null) return null;
            const elseVal = converter.convert_ssr_attr_value_expr(tern.alternate, attrValConv);
            if(elseVal == null) return null;

            const pickCall = builder.make_function_call_value(builder.make_identifier("ssrPickValue", support.ssrPickValueFn, false, location), location);
            pickCall.get_args().push(cond);
            pickCall.get_args().push(thenVal);
            pickCall.get_args().push(elseVal);
            return pickCall as *mut Value;
        }
        default => return null
    }
}

// Flattens a `+` concatenation tree into its parts, converting each leaf into
// an SsrAttributeValue expression. Returns false when any leaf is unsupported.
func (converter : &mut JsConverter) collect_ssr_concat_parts(node : *mut JsNode, attrValConv : &mut AttrValueConverter, out : &mut std::vector<*mut Value>) : bool {
    if(node == null) return false;
    if(node.kind == JsNodeKind.Paren) {
        return converter.collect_ssr_concat_parts((node as *mut JsParen).expression, attrValConv, out);
    }
    if(node.kind == JsNodeKind.BinaryOp && (node as *mut JsBinaryOp).op.equals(view("+"))) {
        const bin = node as *mut JsBinaryOp;
        if(!converter.collect_ssr_concat_parts(bin.left, attrValConv, out)) return false;
        if(!converter.collect_ssr_concat_parts(bin.right, attrValConv, out)) return false;
        return true;
    }
    const partVal = converter.convert_ssr_attr_value_expr(node, attrValConv);
    if(partVal == null) return false;
    out.push(partVal);
    return true;
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
                if(is_client_only_attribute_name(attr.name)) continue;

                if(attr.value != null && attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const container = attr.value as *mut JsJSXExpressionContainer;
                    if(is_non_ssr_expression(container.expression)) continue;
                } else if(is_non_ssr_expression(attr.value)) {
                    continue;
                }

                const attrStructVal = builder.make_struct_value(support.ssrAttrLinkedNode, location);
                const isClass = attr.name.equals("className") || attr.name.equals("class")
                const attrName = if(isClass) std::string_view("class") else attr.name;
                attrStructVal.add_value(std::string_view("name"), converter.make_ssr_text(&attrName, location));

                if(attr.value == null) {
                    const boolVal = builder.make_bool_value(true, location);
                    attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), boolVal));
                } else if(attr.value.kind == JsNodeKind.Literal) {
                    attrStructVal.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(attr.value as *mut JsLiteral, &mut attrValConv, location));
                } else if(attr.value.kind == JsNodeKind.JSXExpressionContainer) {
                    const container = attr.value as *mut JsJSXExpressionContainer;
                    var handled = false;
                    if(container.expression != null) {
                        if(container.expression.kind == JsNodeKind.ChemicalValue) {
                            const chem = container.expression as *mut JsChemicalValue;
                            attrStructVal.add_value(std::string_view("value"), attrValConv.convert_to_attr_value(builder, chem.value.getType(), chem.value));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.Literal) {
                            attrStructVal.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(container.expression as *mut JsLiteral, &mut attrValConv, location));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.ObjectLiteral) {
                            var objText = std::string_view();
                            if(attr.name.equals("style")) {
                                objText = build_js_node_text_view_style_attr(builder, container.expression as *mut JsObjectLiteral)
                            } else {
                                objText = build_js_node_text_view(builder, container.expression)
                            }
                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(&objText, location)));
                            handled = true;
                        } else if(container.expression.kind == JsNodeKind.ArrayLiteral) {
                            // Inline array literal prop (`tabs={["A", "B"]}`):
                            // convert to a Multiple so `.map()`/`.length` over
                            // the prop resolve at SSR runtime.
                            const arrVal = converter.build_ssr_multiple_from_array_node(container.expression as *mut JsArrayLiteral);
                            if(arrVal != null) {
                                attrStructVal.add_value(std::string_view("value"), arrVal);
                                handled = true;
                            }
                        } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                            const mem = container.expression as *mut JsMemberAccess;
                            if(mem.object.kind == JsNodeKind.Identifier && converter.is_component_props_name((mem.object as *mut JsIdentifier).value)) {
                                const params = converter.current_func.get_params();
                                const propsParam = params.get(1);
                                if(propsParam != null) {
                                    const propsId = builder.make_identifier("attrs", propsParam, false, location);
                                    const propsType = propsParam.getType();
                                    const derefProps = builder.make_dereference_value(propsId, (propsType as *mut PointerType).getChildType(), location);
                                    const nameVal = converter.make_ssr_text(&mem.property, location);
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
                                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(&evaluated.textValue, location)));
                                    }
                                    handled = true;
                                } else {
                                    // Props-driven expression: evaluate at SSR runtime
                                    const runtimeVal = converter.convert_ssr_attr_value_expr(container.expression, &mut attrValConv);
                                    if(runtimeVal != null) {
                                        attrStructVal.add_value(std::string_view("value"), runtimeVal);
                                        handled = true;
                                    } else {
                                        const runtimeBool = converter.convert_ssr_attr_bool_expr(container.expression, &mut attrValConv);
                                        if(runtimeBool != null) {
                                            attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), runtimeBool));
                                            handled = true;
                                        }
                                    }
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
                                    attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(&evaluated.textValue, location)));
                                }
                                handled = true;
                            } else {
                                // Props-driven expression: evaluate at SSR runtime
                                const runtimeVal = converter.convert_ssr_attr_value_expr(container.expression, &mut attrValConv);
                                if(runtimeVal != null) {
                                    attrStructVal.add_value(std::string_view("value"), runtimeVal);
                                    handled = true;
                                } else {
                                    const runtimeBool = converter.convert_ssr_attr_bool_expr(container.expression, &mut attrValConv);
                                    if(runtimeBool != null) {
                                        attrStructVal.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), runtimeBool));
                                        handled = true;
                                    }
                                }
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
                const arg = attr.argument
                if(arg != null && arg.kind == JsNodeKind.Identifier) {
                    const argId = arg as *mut JsIdentifier
                    if(converter.is_component_props_name(argId.value)) {
                        // Only the component's own props parameter is SSR-spreadable.
                        // Anything else (local objects, function results) cannot be
                        // resolved at SSR time and must not silently spread `props`.
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
                } else if(arg != null && arg.kind == JsNodeKind.ObjectLiteral) {
                    // Spread of an object literal: enumerate statically-known members
                    const obj = arg as *mut JsObjectLiteral
                    for(var p : uint = 0; p < obj.properties.size(); p++) {
                        const prop = obj.properties.get(p)
                        if(prop.value != null && prop.value.kind == JsNodeKind.Spread) {
                            // Nested dynamic spread inside the literal — cannot SSR
                            continue
                        }
                        if(prop.key.empty()) continue
                        const spreadAttr = builder.make_struct_value(support.ssrAttrLinkedNode, location)
                        spreadAttr.add_value(std::string_view("name"), converter.make_ssr_text(&prop.key, location))
                        var valHandled = false
                        if(prop.value != null && prop.value.kind == JsNodeKind.Literal) {
                            spreadAttr.add_value(std::string_view("value"), converter.convert_js_literal_to_ssr_value(prop.value as *mut JsLiteral, &mut attrValConv, location))
                            valHandled = true
                        } else if(prop.value != null) {
                            const evaluated = converter.eval_ssr_js_expr(prop.value)
                            if(evaluated.valid) {
                                if(evaluated.kind == 1) {
                                    if(!evaluated.boolValue) continue
                                    spreadAttr.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Boolean"), builder.make_bool_value(true, location)))
                                } else {
                                    spreadAttr.add_value(std::string_view("value"), attrValConv.wrapArgAttrValueVariantCall(builder, std::string_view("Text"), converter.make_ssr_text(&evaluated.textValue, location)))
                                }
                                valHandled = true
                            }
                        }
                        if(valHandled) {
                            attrValues.push(spreadAttr)
                            pushedCount++
                        }
                    }
                }
                // Any other spread target cannot be resolved at SSR time; skip it.
            }
        }

        listStruct.add_value(std::string_view("data"), arrayValue);
        listStruct.add_value(std::string_view("size"), builder.make_ubigint_value(pushedCount, location));
    }

    return listStruct as *mut Value;
}
