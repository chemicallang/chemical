
func (converter : &mut ASTConverter) emit_append_js_call(value : *mut Value, len : size_t) {
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const support = converter.support

    var base = builder.make_identifier(std::string_view("page"), support.pageNode, false, location)
    var id = builder.make_identifier(std::string_view("append_js"), support.appendJsFn, false, location)
    const chain = builder.make_access_chain(&std::span<*mut Value>([ base as *mut Value, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)

    var args = call.get_args()
    args.push(value)
    args.push(builder.make_ubigint_value(len, location))

    converter.vec.push(call as *mut ASTNode)
}

func (converter : &mut ASTConverter) emit_append_js_from_str(s : &mut std::string) {
    if (s.empty()) return;
    const builder = converter.builder
    const location = intrinsics::get_raw_location()
    const size = s.size()
    const value = builder.make_string_value(builder.allocate_view(s.view()), location)
    s.clear()
    converter.emit_append_js_call(value, size)
}

func (converter : &mut ASTConverter) make_js_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
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

func (converter : &mut ASTConverter) make_js_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendJsCharPtrFn)
}

func (converter : &mut ASTConverter) make_js_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_char"), converter.support.appendJsCharFn)
}

func (converter : &mut ASTConverter) make_js_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendJsIntFn)
}

func (converter : &mut ASTConverter) make_js_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendJsUIntFn)
}

func (converter : &mut ASTConverter) make_js_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_float"), converter.support.appendJsFloatFn)
}

func (converter : &mut ASTConverter) make_js_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_js_value_call_with(value, std::string_view("append_js_double"), converter.support.appendJsDoubleFn)
}

func (converter : &mut ASTConverter) put_js_error(val : &std::string_view) {
    const value = converter.builder.make_string_value(converter.builder.allocate_view(val), intrinsics::get_raw_location())
    converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);
}

func (converter : &mut ASTConverter) put_by_node_js(type : *mut BaseType, node : *mut ASTNode, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            var fnName = std::string_view("writeToPageJs")
            const writeFn = node.child(&fnName)
            if(writeFn == null) {
                converter.put_by_type(type, value)
                return;
            }
            if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                converter.put_by_type(type, value)
                return;
            }
            var bundleName = std::string_view("pageJs")
            const fn = writeFn as *mut FunctionDeclaration;
            const chain = converter.make_write_to_page_call(value, fnName, writeFn, &bundleName);
            converter.vec.push(chain)
        }
        ASTNodeKind.TypealiasStmt => {
            // const stmt = node as *mut TypealiasStatement
            // const actual_type = stmt.getActualType();
            // converter.put_by_type_js(actual_type, value);
            // TODO:
        }
        default => {
            converter.put_js_error(std::string_view("Unsupported value cant be written to js bundle"))
        }
    }
}

func (converter : &mut ASTConverter) put_js_value_in(value : *mut Value) {
    const type = value.getType();
    switch(type.getKind()) {
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            const kind = intN.get_intn_type_kind();
            if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                converter.vec.push(converter.make_js_char_value_call(value) as *mut ASTNode);
            } else if(kind <= IntNTypeKind.Int128) {
                converter.vec.push(converter.make_js_integer_value_call(value) as *mut ASTNode);
            } else {
                converter.vec.push(converter.make_js_uinteger_value_call(value) as *mut ASTNode);
            }
        }
        BaseTypeKind.Float => {
            converter.vec.push(converter.make_js_float_value_call(value) as *mut ASTNode);
        }
        BaseTypeKind.Double => {
            converter.vec.push(converter.make_js_double_value_call(value) as *mut ASTNode);
        }
        BaseTypeKind.Bool => {
            converter.vec.push(converter.make_js_integer_value_call(value) as *mut ASTNode);
        }
        BaseTypeKind.String, BaseTypeKind.Pointer => {
            converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);
        }
        BaseTypeKind.Linked => {
            const linked = type as *mut LinkedType;
            const node = linked.getLinkedNode();
            converter.put_by_node_js(type, node, value);
        }
        BaseTypeKind.Generic => {
            const generic = type as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            converter.put_by_node_js(type, node, value);
        }
        default => {
            converter.vec.push(converter.make_js_char_ptr_value_call(value) as *mut ASTNode);
        }
    }
}