
struct JsConverter {
    var builder : *mut ASTBuilder
    var support : *mut SymResSupport
    var vec : *mut VecRef<ASTNode>
    var parent : *mut ASTNode
    var str : std::string
}


func (converter : &mut JsConverter) make_char_chain(value : char) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    
    var name = std::string_view("append_js_char")
    var fnPtr = converter.support.appendJsCharFn

    var id = builder.make_identifier(name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    const char_val = builder.make_char_value(value, location);
    args.push(char_val)
    return call;
}

func (converter : &mut JsConverter) make_value_call_with(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut JsConverter) make_char_ptr_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char_ptr"), converter.support.appendJsCharPtrFn)
}

func (converter : &mut JsConverter) make_char_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_char"), converter.support.appendJsCharFn)
}

func (converter : &mut JsConverter) make_integer_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_integer"), converter.support.appendJsIntFn)
}

func (converter : &mut JsConverter) make_uinteger_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_uinteger"), converter.support.appendJsUIntFn)
}

func (converter : &mut JsConverter) make_float_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_float"), converter.support.appendJsFloatFn)
}

func (converter : &mut JsConverter) make_double_value_call(value : *mut Value) : *mut FunctionCallNode {
    return converter.make_value_call_with(value, std::string_view("append_js_double"), converter.support.appendJsDoubleFn)
}

func (converter : &mut JsConverter) make_value_call(value : *mut Value, len : size_t) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var name = std::string_view("append_js")
    
    var id = builder.make_identifier(name, converter.support.appendJsFn, false, location);
    const chain = builder.make_access_chain(std::span<*mut ChainValue>([ base, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(value)
    args.push(builder.make_ubigint_value(len, location));
    return call;
}

func (converter : &mut JsConverter) put_chain_in() {
    if(converter.str.empty()) return;
    
    const location = intrinsics::get_raw_location();
    const builder = converter.builder;
    const value = builder.make_string_value(builder.allocate_view(converter.str.to_view()), location)
    const size = converter.str.size()
    converter.str.clear();
    
    const call = converter.make_value_call(value, size);
    converter.vec.push(call);
}

func (converter : &mut JsConverter) put_wrapping(value : *mut Value) {
    const wrapped = converter.builder.make_value_wrapper(value, converter.parent)
    converter.vec.push(wrapped);
}

func (converter : &mut JsConverter) put_wrapped_chemical_value_in(value : *mut Value) {
    const chain = converter.make_char_ptr_value_call(value)
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_char_value_in(value : *mut Value) {
    var chain = converter.make_char_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_integer_value_in(value : *mut Value) {
    var chain = converter.make_integer_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_uinteger_value_in(value : *mut Value) {
    var chain = converter.make_uinteger_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_float_value_in(value : *mut Value) {
    var chain = converter.make_float_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_wrapped_chemical_double_value_in(value : *mut Value) {
    var chain = converter.make_double_value_call(value);
    converter.vec.push(chain)
}

func (converter : &mut JsConverter) put_by_type(type : *mut BaseType, value : *mut Value) {
    switch(type.getKind()) {
        BaseTypeKind.Void => {
            converter.put_wrapping(value);
        }
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            const kind = intN.get_intn_type_kind()
            if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                converter.put_wrapped_chemical_char_value_in(value)
            } else if(kind <= IntNTypeKind.Int128) {
                // signed
                converter.put_wrapped_chemical_integer_value_in(value)
            } else {
                // unsigned
                converter.put_wrapped_chemical_uinteger_value_in(value)
            }
        }
        BaseTypeKind.Float => {
            converter.put_wrapped_chemical_float_value_in(value)
        }
        BaseTypeKind.Double => {
            converter.put_wrapped_chemical_double_value_in(value)
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut JsConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}

func (converter : &mut JsConverter) convertJsNode(node : *mut JsNode) {
    switch(node.kind) {
        JsNodeKind.VarDecl => {
            var varDecl = node as *mut JsVarDecl
            converter.str.append_view("var ")
            converter.str.append_view(varDecl.name)
            if(varDecl.value != null) {
                converter.str.append_view(" = ")
                converter.convertJsNode(varDecl.value)
            }
            converter.str.append_view(";")
        }
        JsNodeKind.Literal => {
            var literal = node as *mut JsLiteral
            converter.str.append_view(literal.value)
        }
        JsNodeKind.Identifier => {
            var id = node as *mut JsIdentifier
            converter.str.append_view(id.value)
        }
        JsNodeKind.ChemicalValue => {
            var chem = node as *mut JsChemicalValue
            converter.put_chain_in()
            converter.put_chemical_value_in(chem.value)
        }
        JsNodeKind.FunctionCall => {
            var call = node as *mut JsFunctionCall
            converter.convertJsNode(call.callee)
            converter.str.append_view("(")
            var i = 0u
            while(i < call.args.size()) {
                if(i > 0) converter.str.append_view(", ")
                converter.convertJsNode(call.args.get(i))
                i++
            }
            converter.str.append_view(")")
        }
        JsNodeKind.Block => {
            var block = node as *mut JsBlock
            converter.str.append_view("{")
            var i = 0u
            while(i < block.statements.size()) {
                converter.convertJsNode(block.statements.get(i))
                i++
            }
            converter.str.append_view("}")
        }
        JsNodeKind.If => {
            var ifStmt = node as *mut JsIf
            converter.str.append_view("if(")
            converter.convertJsNode(ifStmt.condition)
            converter.str.append_view(")")
            converter.convertJsNode(ifStmt.thenBlock)
            if(ifStmt.elseBlock != null) {
                converter.str.append_view(" else ")
                converter.convertJsNode(ifStmt.elseBlock)
            }
        }
        JsNodeKind.Return => {
            var ret = node as *mut JsReturn
            converter.str.append_view("return")
            if(ret.value != null) {
                converter.str.append_view(" ")
                converter.convertJsNode(ret.value)
            }
            converter.str.append_view(";")
        }
        JsNodeKind.BinaryOp => {
            var binOp = node as *mut JsBinaryOp
            converter.convertJsNode(binOp.left)
            converter.str.append_view(" ")
            converter.str.append_view(binOp.op)
            converter.str.append_view(" ")
            converter.convertJsNode(binOp.right)
        }
        JsNodeKind.FunctionDecl => {
            var func = node as *mut JsFunctionDecl
            converter.str.append_view("function ")
            converter.str.append_view(func.name)
            converter.str.append_view("(")
            var i = 0u
            while(i < func.params.size()) {
                if(i > 0) converter.str.append_view(", ")
                converter.str.append_view(func.params.get(i))
                i++
            }
            converter.str.append_view(")")
            converter.convertJsNode(func.body)
        }
    }
}

func (converter : &mut JsConverter) convertJsRoot(root : *mut JsRoot) {
    var i = 0u
    while(i < root.statements.size()) {
        converter.convertJsNode(root.statements.get(i))
        i++
    }
    converter.put_chain_in()
}
