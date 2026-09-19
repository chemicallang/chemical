struct ASTConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
    // `#css` is a value macro: in value position it returns the generated class
    // so the author can attach it (`class={style_button(page)}`), and scoping the
    // rules under that class is what makes component styles local. In statement
    // position (`#css { ... }` on its own line) nothing can ever receive the
    // class, so scoping the rules under it emits CSS that matches no element.
    // Such a block is a global stylesheet: selectors are emitted as written and
    // the block's own declarations go to :root.
    var is_global_block : bool = false
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

// Wrap the declarations accumulated in `str` in `:root { ... }`.
//
// Used for `#css` blocks in statement position: the macro's value is the class
// name, and a statement discards it, so declarations scoped to that class would
// be dead CSS. Declarations written at the top level of a block describe the
// document itself, so they belong to :root.
func allocate_view_with_root_selector(builder : *mut ASTBuilder, str : &mut std::string) : std::string_view {
    str.append('}')
    const prefix = std::string_view(":root{")
    const prefix_size = prefix.size()
    const total_size = str.size() + prefix_size
    const ptr = builder.allocate_str_size(total_size + 1)
    memcpy(ptr, prefix.data(), prefix_size)
    memcpy(ptr + prefix_size, str.data(), str.size())
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

// Deterministic class seed for non-hashable CSSOMs (those with dynamic values,
// media queries, nested rules, or keyframes). Previously this used rand(), which
// made generated class names differ between otherwise-identical builds. Derive
// the seed from the CSS block's source location (via its parent node) so the
// output is reproducible; distinct source locations still get distinct classes.
func cssom_stable_hash(seed : ubigint) : uint32_t {
    var hash : uint32_t = 0x811C9DC5
    var i : uint = 0
    while(i < 8) {
        hash = hash ^ (((seed >> (i * 8)) & 0xFF) as uint32_t)
        hash = hash * 0x01000193
        i++
    }
    return hash
}

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
                 if(parent_selectors.empty()) {
                     // A global block (`#css` in statement position) has no class
                     // for the ampersand to anchor to; its scope root is the
                     // document itself, which is also where the block's own
                     // declarations go. `&.blue` therefore becomes `:root.blue`.
                     var res = std::string();
                     css_serialize_complex(sel, &mut res, std::string_view(":root"));
                     current_selectors.push(res);
                 } else {
                     var p : uint = 0;
                     while(p < parent_selectors.size()) {
                         var pdf = parent_selectors.get_ptr(p);
                         var res = std::string();
                         css_serialize_complex(sel, &mut res, pdf.view());
                         current_selectors.push(res); p++;
                     }
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

// Stable structural serialization used as part of the deterministic class-name
// seed (nested-rule selectors + declaration property names + nested rules).
func cssom_seed_append_nested(nr : *mut CSSNestedRule, out : &mut std::string) {
    if(nr.selector != null) {
        var p : uint = 0
        while(p < nr.selector.selectors.size()) {
            css_serialize_complex(nr.selector.selectors.get(p), out, std::string_view("&"))
            out.append(';')
            p++
        }
    }
    var d : uint = 0
    while(d < nr.declarations.size()) { out.append_view(&nr.declarations.get(d).property.name); out.append(';'); d++ }
    var n : uint = 0
    while(n < nr.nested_rules.size()) { cssom_seed_append_nested(nr.nested_rules.get(n), out); n++ }
}

func cssom_seed_append_media(mr : *mut CSSMediaRule, out : &mut std::string) {
    var d : uint = 0
    while(d < mr.declarations.size()) { out.append_view(&mr.declarations.get(d).property.name); out.append(';'); d++ }
    var n : uint = 0
    while(n < mr.nested_rules.size()) { cssom_seed_append_nested(mr.nested_rules.get(n), out); n++ }
}

func (converter : &mut ASTConverter) convertCSSOM(om : *mut CSSOM, seed : ubigint) {
    const builder = converter.builder
    const str = &mut converter.str
    var size = om.declarations.size()
    
    if(size == 0 && om.media_queries.empty() && om.nested_rules.empty() && om.keyframes.empty()) { return; }

    const location = intrinsics::get_raw_location();

    if(!om.is_hashable()) {
        // Deterministic class seed from the CSS content when it has no dynamic
        // (Chemical) values: serialize the declarations plus nested/media/
        // keyframe structure and hash it. Dynamic-value blocks (whose values
        // interleave with the emitter) keep the location seed. Previously this
        // used rand(), which made class names differ between identical builds.
        var hash : uint32_t = cssom_stable_hash(seed)
        if(om.dyn_values.empty()) {
            var seedStr = std::string()
            var si : uint = 0
            while(si < size) { css_write_declaration_text(om.declarations.get(si), &mut seedStr, converter.as_emitter()); si++ }
            var ni : uint = 0
            while(ni < om.nested_rules.size()) { cssom_seed_append_nested(om.nested_rules.get(ni), &mut seedStr); ni++ }
            var mi : uint = 0
            while(mi < om.media_queries.size()) { cssom_seed_append_media(om.media_queries.get(mi), &mut seedStr); mi++ }
            var ki : uint = 0
            while(ki < om.keyframes.size()) { seedStr.append_view(&om.keyframes.get(ki).name); seedStr.append(';'); ki++ }
            if(!seedStr.empty()) { hash = fnv1a_hash_32(seedStr.data()) as uint32_t }
        }
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

        if(converter.is_global_block && size > 0) {
            // Statement position: nothing can attach the generated class, so
            // scoping the declarations under it would emit dead CSS. The block's
            // own declarations describe the document itself: they go to :root.
            // Nested rules, media queries and keyframes are emitted further down
            // with the selectors the author wrote.
            const rootOpen = builder.allocate_view(std::string_view(":root{"))
            converter.put_view_chain(&rootOpen)
            var gi : uint = 0
            while(gi < size) { css_write_declaration_text(om.declarations.get(gi), str, converter.as_emitter()); gi++; }
            str.append('}')
            converter.put_chain_in()
        }

        if(!converter.is_global_block && (size > 0 || !om.media_queries.empty() || !om.keyframes.empty())) {
            converter.put_view_chain(&total)
            var i : uint = 0
            while(i < size) { css_write_declaration_text(om.declarations.get(i), str, converter.as_emitter()); i++; }
            str.append('}')
            converter.put_chain_in();
        }

        var k_idx : uint = 0
        while(k_idx < om.keyframes.size()) {
            converter.writeKeyframesRule(om.keyframes.get(k_idx), &mut *str)
            if(!str.empty()) { converter.put_chain_in(); }
            k_idx++;
        }

        // Only a block's own (root) declarations create a class scope, and only
        // when something can receive that class. Without a scope, nested rules
        // are emitted with the selectors they were written with.
        var parents = std::vector<std::string>();
        if(!converter.is_global_block && (size > 0 || !om.media_queries.empty() || !om.keyframes.empty())) {
            var root_sel = std::string();
            root_sel.append('.'); root_sel.append_view(&classView);
            parents.push(root_sel);
        }

        var n_idx : uint = 0;
        while(n_idx < om.nested_rules.size()) {
            converter.generate_css_recurse(om.nested_rules.get(n_idx), &mut parents);
            n_idx++;
        }

        // Media queries are emitted after the block's own rules: the cascade
        // resolves equal-specificity conflicts by source order, so a responsive
        // override (`@media (max-width: ...) { padding: 1rem }`) only takes over
        // from the base rule it overrides when it comes after it.
        var j : uint = 0
        while(j < om.media_queries.size()) {
            if(converter.is_global_block) {
                // No root class here: declarations written directly in the media
                // query belong to :root, nested rules keep their selectors.
                const declRoot = std::string_view(":root")
                converter.writeMediaRule(om.media_queries.get(j), &mut *str, std::string_view(), declRoot)
            } else {
                converter.writeMediaRule(om.media_queries.get(j), &mut *str, classView)
            }
            if(!str.empty()) { converter.put_chain_in(); }
            j++;
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

        if(converter.is_global_block) {
            // Statement position: the declarations have no element to attach the
            // generated class to, so they belong to the document root.
            const rootView = allocate_view_with_root_selector(builder, &mut *str)
            converter.put_append_css_value_chain(&rootView)
        } else {
            const totalView = allocate_view_with_classname(builder, &mut *str, hash)
            om.className = std::string_view(totalView.data() + 1, 7u)
            converter.put_append_css_value_chain(&totalView)
        }
        
        converter.vec = oldVec;
        converter.vec.push(ifStmt);
    }
}
