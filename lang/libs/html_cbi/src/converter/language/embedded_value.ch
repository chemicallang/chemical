// value is the self arg here
// this makes the call 'value.fn_name(page, bundle)'
func (converter : &mut ASTConverter) make_write_to_page_call(value : *mut Value, fn_name : std::string_view, fnPtr : *mut ASTNode, bundleName : &std::string_view) : *mut FunctionCallNode {
    const builder = converter.builder
    const location = intrinsics::get_raw_location();
    var pageArg = builder.make_identifier(std::string_view("page"), converter.support.pageNode, false, location);
    var id = builder.make_identifier(&fn_name, fnPtr, false, location);
    const chain = builder.make_access_chain(&std::span<*mut Value>([ value as *mut Value, id ]), location)
    var call = builder.make_function_call_node(chain, converter.parent, location)
    var args = call.get_args();
    args.push(pageArg)
    const pageHead = converter.support.pageNode.child(bundleName);
    if(pageHead == null) {
        return call;
    }
    var bundleId = builder.make_identifier(bundleName, pageHead, false, location);
    const bundleAccess = builder.make_access_chain(&std::span<*mut Value>([ pageArg, bundleId ]), location)
    args.push(bundleAccess)
    return call;
}

func (converter : &mut ASTConverter) put_html_error(val : &std::string_view) {
    const value = converter.builder.make_string_value(converter.builder.allocate_view(val), intrinsics::get_raw_location())
    converter.vec.push(converter.make_char_ptr_value_call(value) as *mut ASTNode);
}

func (converter : &mut ASTConverter) put_by_node(type : *mut BaseType, node : *mut ASTNode, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            var fnName = std::string_view("writeToPageHtml")
            const writeFn = node.child(&fnName)
            if(writeFn == null) {
                converter.put_html_error("'writeToPageHtml' not found on the object");
                return;
            }
            if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                converter.put_html_error("'writeToPageHtml' not found on the object");
                return;
            }
            var bundleName = if(converter.in_head) std::string_view("pageHead") else std::string_view("pageHtml")
            const fn = writeFn as *mut FunctionDeclaration;
            const chain = converter.make_write_to_page_call(value, fnName, writeFn, &bundleName);
            converter.vec.push(chain)
        }
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            converter.put_by_type(actual_type, value);
        }
        default => {
            converter.put_by_type(type, value);
        }
    }
}

func (converter : &mut ASTConverter) put_ref_node(node : *mut ASTNode, type : *mut BaseType, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            converter.put_ref_child_type(actual_type, value);
        }
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            converter.put_by_node(type, node, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = converter.builder.make_dereference_value(value, type, loc);
            converter.put_by_type(type, deref);
        }
    }
}

func (converter : &mut ASTConverter) put_ref_child_type(childType : *mut BaseType, value : *mut Value) {
    switch(childType.getKind()) {
        BaseTypeKind.Linked => {
            const linked = childType as *mut LinkedType;
            const node = linked.getLinkedNode();
            converter.put_ref_node(node, childType, value)
        }
        BaseTypeKind.Generic => {
            const generic = childType as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            converter.put_ref_node(node, childType, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = converter.builder.make_dereference_value(value, childType, loc);
            converter.put_by_type(childType, deref);
        }
    }
}

func (converter : &mut ASTConverter) put_by_type(type : *mut BaseType, value : *mut Value) {
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
        BaseTypeKind.ExpressiveString => {
            if(value.getKind() == ValueKind.ExpressiveString) {
                const exprString = value as *mut ExpressiveString
                const values = exprString.getValues()
                const size = values.size()
                var i = 0u;
                while(i < size) {
                    const ptr = values.get(i)
                    converter.put_by_type(ptr.getType(), ptr);
                    i++;
                }
            } else {
                // TODO: error out, cannot handle comptime functions that return expressive strings yet !
            }
        }
        BaseTypeKind.Linked => {
            const linked = type as *mut LinkedType;
            const node = linked.getLinkedNode();
            converter.put_by_node(type, node, value);
        }
        BaseTypeKind.Generic => {
            const generic = type as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            converter.put_by_node(type, node, value);
        }
        BaseTypeKind.Reference => {
            const refType = type as *mut ReferenceType
            const childType = refType.getChildType()
            converter.put_ref_child_type(childType, value)
        }
        default => {
            converter.put_wrapped_chemical_value_in(value);
        }
    }
}

func (converter : &mut ASTConverter) put_chemical_value_in(value : *mut Value) {
    converter.put_by_type(value.getType(), value)
}