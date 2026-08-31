struct ASTConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
}

// ─── CssEmitter implementation ──────────────────────────────────────────────

impl CssEmitter for ASTConverter {
    func emit_text(&mut self, text : &std::string_view) { self.str.append_view(text) }
    func emit_char(&mut self, c : char) { self.str.append(c) }
    func emit_integer(&mut self, v : bigint) { self.str.append_integer(v) }
    func emit_uinteger(&mut self, v : ubigint) { self.str.append_uinteger(v) }
    func flush(&mut self) {
        if(self.str.empty()) return
        const location = intrinsics::get_raw_location()
        const value = self.builder.make_string_value(self.builder.allocate_view(self.str.to_view()), location)
        const size = self.str.size()
        self.str.clear()
        const chain = self.make_append_css_value_chain(value, size)
        self.vec.push(chain)
    }
    func emit_chemical_value(&mut self, value : *mut Value) {
        self.flush()
        self.put_chemical_value_in(value)
    }
}

// ─── Emitter pointer helper ─────────────────────────────────────────────────

func (converter : &mut ASTConverter) as_emitter() : *mut CssEmitter {
    return converter as *mut CssEmitter
}

// ─── CBI-specific methods (AST building) ────────────────────────────────────

func (converter : &mut ASTConverter) make_char_call(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_css_char"), converter.support.appendCssCharFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(builder.make_char_value(value, location))
    return call;
}

func (converter : &mut ASTConverter) make_append_css_value_chain(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_css"), converter.support.appendCssFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut ASTConverter) make_value_chain(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(std::string_view("append_css_nh"), converter.support.appendCssFn, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut ASTConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(&fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut ASTConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_char_ptr"), converter.support.appendCssCharPtrFn)
}

func (converter : &mut ASTConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_integer"), converter.support.appendCssIntFn)
}

func (converter : &mut ASTConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_uinteger"), converter.support.appendCssUIntFn)
}

func (converter : &mut ASTConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_float"), converter.support.appendCssFloatFn)
}

func (converter : &mut ASTConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_css_double"), converter.support.appendCssDoubleFn)
}

func (converter : &mut ASTConverter) make_chain_of_view(view : &std::string_view) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    return converter.make_value_chain(builder.make_string_value(builder.allocate_view(view), location), view.size());
}

func (converter : &mut ASTConverter) put_char_chain(value : char) { converter.vec.push(converter.make_char_call(value)); }

func (converter : &mut ASTConverter) put_view_chain(view : &std::string_view) { converter.vec.push(converter.make_chain_of_view(view)); }

func (converter : &mut ASTConverter) put_append_css_value_chain(view : &std::string_view) {
    const location = intrinsics::get_raw_location();
    const builder = converter.builder
    const value = builder.make_string_value(builder.allocate_view(view), location)
    converter.vec.push(converter.make_append_css_value_chain(value, view.size()));
}

func (converter : &mut ASTConverter) put_chain_in() {
    if(converter.str.empty()) return
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    const value = builder.make_string_value(builder.allocate_view(converter.str.to_view()), location)
    const size = converter.str.size()
    converter.str.clear();
    converter.vec.push(converter.make_value_chain(value, size));
}

func (converter : &mut ASTConverter) put_wrapping(value : *mut Value) {
    converter.vec.push(converter.builder.make_value_wrapper(value, converter.parent))
}

func is_func_call_ret_void(builder : *mut ASTBuilder, call : *mut FunctionCall) : bool {
    return call.getType().getKind() == BaseTypeKind.Void;
}

func (converter : &mut ASTConverter) put_chemical_value_in(value_ptr : *mut Value) {
    const builder = converter.builder
    var value = value_ptr;
    const kind = value.getKind();
    if(kind == ValueKind.AccessChain) {
        const chain = value as *mut AccessChain
        const values = chain.get_values();
        const last = values.get(values.size() - 1)
        if(last.getKind() == ValueKind.FunctionCall && is_func_call_ret_void(builder, last as *mut FunctionCall)) {
            converter.put_wrapping(value);
        } else {
            converter.vec.push(converter.make_char_ptr_value_call(value))
        }
    } else if(kind == ValueKind.FunctionCall && is_func_call_ret_void(builder, value as *mut FunctionCall)) {
        converter.put_wrapping(value);
    } else {
        converter.vec.push(converter.make_char_ptr_value_call(value))
    }
}

// ─── CSS value writing (via shared standalone functions) ─────────────────────

func (converter : &mut ASTConverter) writeValue(value : &mut CSSValue) {
    writeCssValueToBuffer(value, &mut converter.str, converter.as_emitter())
}

func (converter : &mut ASTConverter) convertDeclaration(decl : *mut CSSDeclaration) {
    css_write_declaration_text(decl, &mut converter.str, converter.as_emitter())
}

// ─── CBI-specific entry points ──────────────────────────────────────────────

const BASE64_CHARS : char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

func base64_encode_32bit(hash : uint32_t, out : *mut char) {
    for (var i = 0; i < 6; i++) { out[5 - i] = BASE64_CHARS[hash & 0x3F]; hash >>= 6; }
}

func put_class_name(hash : uint32_t, prefix : char, ptr : *mut char) {
    *ptr = '.'; *(ptr + 1) = prefix
    base64_encode_32bit(hash, ptr + 2)
    *(ptr + 8) = '{'
}

func allocate_view_with_classname(builder : *mut ASTBuilder, str : &mut std::string, hash : size_t) : std::string_view {
    str.append('}')
    const total_size = str.size() + 9;
    const ptr = builder.allocate_str_size(total_size + 1)
    put_class_name(hash, 'h', ptr)
    memcpy(ptr + 9, str.data(), str.size())
    *(ptr + total_size) = '\0'
    return std::string_view(ptr, total_size)
}

func (converter : &mut ASTConverter) put_class_name_chain(hash : uint32_t, prefix : char) {
    var className : char[10] = [];
    className[0] = '.'; className[1] = prefix
    base64_encode_32bit(hash, &raw mut className[2])
    className[8] = '{'; className[9] = '\0'
    converter.put_view_chain(std::string_view(&raw className[0], 9u))
}

func generate_random_32bit() : uint32_t { return (rand() as uint32_t << 16) | rand() as uint32_t; }

func (converter : &mut ASTConverter) make_func_call_with_arg(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCall {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(&fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    call.get_args().push(value)
    return call;
}

func (converter : &mut ASTConverter) make_require_css_hash_call(hash : size_t) : *mut FunctionCall {
    return converter.make_func_call_with_arg(converter.builder.make_ubigint_value(hash, intrinsics::get_raw_location()), std::string_view("require_css_hash"), converter.support.requireCssHashFn)
}

func (converter : &mut ASTConverter) make_set_css_hash_call(hash : size_t) : *mut FunctionCallNode {
    return converter.make_value_call_with(converter.builder.make_ubigint_value(hash, intrinsics::get_raw_location()), std::string_view("set_css_hash"), converter.support.setCssHashFn)
}

func (converter : &mut ASTConverter) make_require_random_css_hash_call(hash : size_t) : *mut FunctionCall {
    return converter.make_func_call_with_arg(converter.builder.make_ubigint_value(hash, intrinsics::get_raw_location()), std::string_view("require_random_css_hash"), converter.support.requireRandomCssHashFn)
}

func (converter : &mut ASTConverter) make_set_random_css_hash_call(hash : size_t) : *mut FunctionCallNode {
    return converter.make_value_call_with(converter.builder.make_ubigint_value(hash, intrinsics::get_raw_location()), std::string_view("set_random_css_hash"), converter.support.setRandomCssHashFn)
}

func (converter : &mut ASTConverter) generate_css_recurse(om : *CSSNestedRule, parent_selectors : &mut std::vector<std::string>) {
    const str = &mut converter.str
    var current_selectors = std::vector<std::string>();
    
    if(om.selector != null) {
        var i : uint = 0;
        while(i < om.selector.selectors.size()) {
             var sel = om.selector.selectors.get(i);
             if(css_has_ampersand_complex(sel)) {
                 var p : uint = 0;
                 while(p < parent_selectors.size()) {
                     var pdf = parent_selectors.get_ptr(p);
                     var res = std::string();
                     css_serialize_complex(sel, &mut res, pdf.view());
                     current_selectors.push(res); p++;
                 }
             } else if(!parent_selectors.empty()) {
                 var p : uint = 0;
                 while(p < parent_selectors.size()) {
                     var pdf = parent_selectors.get_ptr(p);
                     var res = std::string();
                     res.append_view(pdf.view()); res.append(' ');
                     css_serialize_complex(sel, &mut res, std::string_view("&"));
                     current_selectors.push(res); p++;
                 }
             } else {
                 var res = std::string();
                 css_serialize_complex(sel, &mut res, std::string_view("&"));
                 current_selectors.push(res);
             }
             i++;
        }
    } else {
        current_selectors = std::replace(parent_selectors, std::vector<std::string>());
    }
    
    if(!om.declarations.empty()) {
        var sel_str = std::string();
        var k : uint = 0;
        while(k < current_selectors.size()) {
            if(k > 0) sel_str.append_view(",");
            var sel_str_ptr = current_selectors.get_ptr(k);
            sel_str.append_view(sel_str_ptr.view()); k++;
        }
        str.append_view(sel_str.view());
        str.append_view(" { ");
        var idx : uint = 0;
        while(idx < om.declarations.size()) {
             css_write_declaration_text(om.declarations.get(idx), str, converter.as_emitter());
             idx++;
        }
        str.append_view(" }");
        converter.put_chain_in();
    }
    
    var n : uint = 0;
    while(n < om.nested_rules.size()) {
        converter.generate_css_recurse(om.nested_rules.get(n), &mut current_selectors);
        n++;
    }
}

func (converter : &mut ASTConverter) generate_css_root(om : *mut CSSOM, root_selector : std::string_view) {
     var parents = std::vector<std::string>();
     var root_sel_str = std::string()
     root_sel_str.append_view(&root_selector)
     parents.push(root_sel_str);
     
     const str = &mut converter.str
     
     if(om.declarations.size() > 0) {
         str.append_view(&root_selector);
         str.append_view(" { ");
         var i : uint = 0;
         while(i < om.declarations.size()) {
             css_write_declaration_text(om.declarations.get(i), str, converter.as_emitter());
             i++;
         }
         str.append_view(" }");
         converter.put_chain_in();
     }
     
     var j : uint = 0;
     while(j < om.nested_rules.size()) {
         converter.generate_css_recurse(om.nested_rules.get(j), &mut parents);
         j++;
     }
     
     var m : uint = 0;
     while(m < om.media_queries.size()) {
            converter.writeMediaRule(om.media_queries.get(m), &mut *str, root_selector);
            if(!str.empty()) { converter.put_chain_in(); }
            m++;
     }

     var k : uint = 0;
     while(k < om.keyframes.size()) {
            converter.writeKeyframesRule(om.keyframes.get(k), &mut *str);
            if(!str.empty()) { converter.put_chain_in(); }
            k++;
     }
}

func (converter : &mut ASTConverter) convertCSSOM(om : *mut CSSOM) {
    const builder = converter.builder
    const str = &mut converter.str
    var size = om.declarations.size()
    
    if(size == 0 && om.media_queries.empty() && om.nested_rules.empty() && om.keyframes.empty()) { return; }

    const location = intrinsics::get_raw_location();

    if(!om.is_hashable()) {
        const hash = generate_random_32bit();
        var ifStmt = builder.make_if_stmt(converter.make_require_random_css_hash_call(hash), converter.parent, location);
        var body = ifStmt.get_body();
        body.push(converter.make_set_random_css_hash_call(hash));
        var oldVec = converter.vec;
        converter.vec = body;

        var className : char[10] = [];
        className[0] = '.'; className[1] = 'r'
        base64_encode_32bit(hash, &raw mut className[2])
        className[8] = '{'; className[9] = '\0'
        const total = builder.allocate_view(std::string_view(&raw className[0], 9u));
        const classView = std::string_view(total.data() + 1, 7u);
        om.className = classView

        if(size > 0 || !om.media_queries.empty() || !om.keyframes.empty()) {
            converter.put_view_chain(&total)
            var i : uint = 0
            while(i < size) { css_write_declaration_text(om.declarations.get(i), str, converter.as_emitter()); i++; }
            str.append('}')
            converter.put_chain_in();
        }

        var j : uint = 0
        while(j < om.media_queries.size()) {
            converter.writeMediaRule(om.media_queries.get(j), &mut *str, classView)
            if(!str.empty()) { converter.put_chain_in(); }
            j++;
        }

        var k_idx : uint = 0
        while(k_idx < om.keyframes.size()) {
            converter.writeKeyframesRule(om.keyframes.get(k_idx), &mut *str)
            if(!str.empty()) { converter.put_chain_in(); }
            k_idx++;
        }

        var parents = std::vector<std::string>();
        if(size > 0 || !om.media_queries.empty() || !om.keyframes.empty()) {
            var root_sel = std::string();
            root_sel.append('.'); root_sel.append_view(&classView);
            parents.push(root_sel);
        }

        var n_idx : uint = 0;
        while(n_idx < om.nested_rules.size()) {
            converter.generate_css_recurse(om.nested_rules.get(n_idx), &mut parents);
            n_idx++;
        }
        
        converter.vec = oldVec;
        converter.vec.push(ifStmt);

    } else {
        var i : uint = 0
        while(i < size) { css_write_declaration_text(om.declarations.get(i), str, converter.as_emitter()); i++; }

        const hash = fnv1a_hash_32(str.data());
        var ifStmt = builder.make_if_stmt(converter.make_require_css_hash_call(hash), converter.parent, location);
        var body = ifStmt.get_body();
        body.push(converter.make_set_css_hash_call(hash));
        var oldVec = converter.vec;
        converter.vec = body;

        const totalView = allocate_view_with_classname(builder, &mut *str, hash)
        om.className = std::string_view(totalView.data() + 1, 7u)
        converter.put_append_css_value_chain(&totalView)
        
        converter.vec = oldVec;
        converter.vec.push(ifStmt);
    }
}
