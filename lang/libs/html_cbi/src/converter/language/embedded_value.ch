func (converter : &mut ASTConverter) put_by_node(type : *mut BaseType, node : *mut ASTNode, value : *mut Value) {
    switch(node.getKind()) {
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            var fnName = if(converter.in_head) std::string_view("writeToPageHead") else std::string_view("writeToPageBody")
            const writeFn = node.child(fnName)
            if(writeFn == null) {
                converter.put_by_type(type, value)
                return;
            }
            if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                converter.put_by_type(type, value)
                return;
            }
            const fn = writeFn as *mut FunctionDeclaration;
            const chain = converter.make_call_inside(value, fnName, writeFn);
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