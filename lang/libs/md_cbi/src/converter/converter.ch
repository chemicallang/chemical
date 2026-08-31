struct MdConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
}

// ─── MdEmitter implementation ───────────────────────────────────────────────

impl MdEmitter for MdConverter {
    func emit_text(&mut self, text : &std::string_view) { self.str.append_view(text) }
    func emit_char(&mut self, c : char) { self.str.append(c) }
    func emit_integer(&mut self, v : bigint) { self.str.append_integer(v) }
    func flush(&mut self) {
        if(self.str.empty()) return
        if(self.support.pageNode == null) return
        const location = intrinsics::get_raw_location()
        const value = self.builder.make_string_value(self.builder.allocate_view(self.str.to_view()), location)
        const size = self.str.size()
        self.str.clear()
        const call = self.make_html_value_call(value, size)
        self.vec.push(call as *mut ASTNode)
    }
    func emit_interpolation(&mut self, value : *mut Value) {
        if(self.support.pageNode == null) return
        if(value == null) { printf("[md-crash-guard] interpolation value is null\n"); fflush(null); return }
        const type = value.getType()
        if(type == null) { printf("[md-crash-guard] interpolation value type is null\n"); fflush(null); return }
        var call : *mut FunctionCallNode = null
        switch(type.getKind()) {
            BaseTypeKind.IntN => {
                const intN = type as *mut IntNType
                if(intN.get_intn_type_kind() <= IntNTypeKind.Int128) {
                    call = self.make_value_call_with(value, std::string_view("append_html_integer"), self.support.appendHtmlIntFn)
                } else {
                    call = self.make_value_call_with(value, std::string_view("append_html_uinteger"), self.support.appendHtmlUIntFn)
                }
            }
            BaseTypeKind.Float => { call = self.make_value_call_with(value, std::string_view("append_html_float"), self.support.appendHtmlFloatFn) }
            BaseTypeKind.Double => { call = self.make_value_call_with(value, std::string_view("append_html_double"), self.support.appendHtmlDoubleFn) }
            default => { call = self.make_value_call_with(value, std::string_view("append_html_char_ptr"), self.support.appendHtmlCharPtrFn) }
        }
        if(call != null) { self.vec.push(call as *mut ASTNode) }
    }
}

// ─── CBI-specific methods ───────────────────────────────────────────────────

func (converter : &mut MdConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
    var id = builder.make_identifier(&fn_name, fnPtr, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    call.get_args().push(value)
    return call
}

func (converter : &mut MdConverter) make_html_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location)
    var id = builder.make_identifier(std::string_view("append_html"), converter.support.appendHtmlFn, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args()
    args.push(value)
    args.push(builder.make_ubigint_value(len, location))
    return call
}

// ─── Conversion entry point ─────────────────────────────────────────────────

func (converter : &mut MdConverter) convertMdRoot(root : *mut MdRoot) {
    if(root != null) {
        md_convert_md_node(&mut converter.str, root as *mut MdNode, converter as *mut MdEmitter, null, null)
    }
    converter.flush()
}
