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

func (converter : &mut JsConverter) expr_references_reactive_var(node : *mut JsNode) : bool {
    if(node == null) return false;
    switch(node.kind) {
        JsNodeKind.Identifier => {
            return converter.is_reactive_var((node as *mut JsIdentifier).value);
        }
        JsNodeKind.MemberAccess => {
            const mem = node as *mut JsMemberAccess;
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                return converter.is_reactive_var((mem.object as *mut JsIdentifier).value);
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
    if(text.empty()) return ssr_js_eval_invalid();
    if(text.equals(view("true"))) return ssr_js_eval_bool(true);
    if(text.equals(view("false"))) return ssr_js_eval_bool(false);
    const stripped = strip_js_string_quotes(text);
    if(stripped.size() < text.size()) return ssr_js_eval_text(stripped);
    var num : bigint = 0;
    if(parse_ssr_bigint(text, &mut num)) return ssr_js_eval_number(num, text);
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
            if(mem.object != null && mem.object.kind == JsNodeKind.Identifier && mem.property.equals(view("value"))) {
                return converter.is_reactive_var((mem.object as *mut JsIdentifier).value);
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
                    const mem = node as *mut JsMemberAccess;
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
        JsNodeKind.Literal, JsNodeKind.Identifier => {
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

func (converter : &mut JsConverter) convert_js_expr_to_ssr_bool_value(node : *mut JsNode) : *mut Value {
    if(node == null) return null;

    const builder = converter.builder;
    const location = intrinsics::get_raw_location();
    const support = converter.support;

    if(node.kind == JsNodeKind.MemberAccess) {
        if(converter.is_component_props_read(node)) {
            const mem = node as *mut JsMemberAccess;
            const v = converter.make_ssr_prop_v_call(mem.property);
            const truthyCall = builder.make_function_call_value(builder.make_identifier("isSsrAttributeValueTruthy", support.isSsrAttributeValueTruthyFn, false, location), location);
            truthyCall.get_args().push(v);
            return truthyCall as *mut Value;
        }
    }

    if(node.kind == JsNodeKind.Literal) {
        var lit = node as *mut JsLiteral;
        if(lit.value.equals(view("true"))) return builder.make_bool_value(true, location) as *mut Value;
        if(lit.value.equals(view("false"))) return builder.make_bool_value(false, location) as *mut Value;
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
    
    const text = strip_js_string_quotes(val);
    return attrValConv.wrapArgAttrValueVariantCall(builder, "Text", converter.make_ssr_text(&text, location));
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
                        } else if(container.expression.kind == JsNodeKind.MemberAccess) {
                            const mem = container.expression as *mut JsMemberAccess;
                            if(mem.object.kind == JsNodeKind.Identifier && (mem.object as *mut JsIdentifier).value.equals("props")) {
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
