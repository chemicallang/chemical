struct AttrValueConverter {

    var pageNode : *mut ASTNode

    var ssrAttributeValueNode : *mut ASTNode

    var multipleAttributeValueNode : *mut ASTNode

    var parent : *mut ASTNode

}

func (converter : &mut AttrValueConverter) wrapArgAttrValueVariantCall(builder : *mut ASTBuilder, name : &std::string_view, value : *mut Value) : *mut Value {
    const child = converter.ssrAttributeValueNode.child(name)
    const location = intrinsics::get_raw_location();
    var base = builder.make_identifier(std::string_view("SsrAttributeValue"), converter.ssrAttributeValueNode, false, location);
    var id = builder.make_identifier(name, child, false, location);
    const chain = builder.make_access_chain(std::span<*mut Value>([ base, id ]), location)
    var call = builder.make_function_call_value(chain, location)
    var args = call.get_args();
    args.push(value)
    return call;
}

func (converter : &mut AttrValueConverter) convert_node_attr_value(builder : *mut ASTBuilder, type : *mut BaseType, node : *mut ASTNode, value : *mut Value) : *mut Value {
    switch(node.getKind()) {
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            var fnName = std::string_view("getWritableValue")
            const writeFn = node.child(fnName)
            if(writeFn == null) {
                return converter.convert_to_attr_value(builder, type, value)
            }
            if(writeFn.getKind() != ASTNodeKind.FunctionDecl) {
                return converter.convert_to_attr_value(builder, type, value)
            }
            const location = intrinsics::get_raw_location();
            var base = builder.make_identifier(std::string_view("page"), converter.pageNode, false, location);
            var id = builder.make_identifier(fnName, writeFn, false, location);
            const chain = builder.make_access_chain(std::span<*mut Value>([ value as *mut Value, id ]), location)
            var call = builder.make_function_call_value(chain, location)
            var args = call.get_args();
            args.push(base)
            return chain;

        }
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            return converter.convert_to_attr_value(builder, actual_type, value);
        }
        default => {
            return converter.convert_to_attr_value(builder, type, value);
        }
    }
}

func (converter : &mut AttrValueConverter) convert_ref_node_attr(builder : *mut ASTBuilder, node : *mut ASTNode, type : *mut BaseType, value : *mut Value) : *mut Value {
    switch(node.getKind()) {
        ASTNodeKind.TypealiasStmt => {
            const stmt = node as *mut TypealiasStatement
            const actual_type = stmt.getActualType();
            return converter.convert_attr_ref_child_type(builder, actual_type, value);
        }
        ASTNodeKind.StructDecl, ASTNodeKind.UnionDecl, ASTNodeKind.VariantDecl => {
            return converter.convert_node_attr_value(builder, type, node, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = builder.make_dereference_value(value, type, loc);
            return converter.convert_to_attr_value(builder, type, deref);
        }
    }
}

func (converter : &mut AttrValueConverter) convert_attr_ref_child_type(builder : *mut ASTBuilder, childType : *mut BaseType, value : *mut Value) : *mut Value {
    switch(childType.getKind()) {
        BaseTypeKind.Linked => {
            const linked = childType as *mut LinkedType;
            const node = linked.getLinkedNode();
            return converter.convert_ref_node_attr(builder, node, childType, value)
        }
        BaseTypeKind.Generic => {
            const generic = childType as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            return converter.convert_ref_node_attr(builder, node, childType, value)
        }
        default => {
            const loc = intrinsics::get_raw_location();
            const deref = builder.make_dereference_value(value, childType, loc);
            return converter.convert_to_attr_value(builder, childType, deref);
        }
    }
}

func (converter : &mut AttrValueConverter) convert_multiple_attr_values(
    builder : *mut ASTBuilder,
    start : **mut Value,
    size : ubigint
) : *mut Value {
    // construct an array value, array of SsrAttributeValue
    var location = intrinsics::get_raw_location();
    var attrValueType = builder.make_linked_type("SsrAttributeValue", converter.ssrAttributeValueNode, location)
    var ssrAttrValArr = builder.make_array_value(attrValueType, location)
    var arrValues = ssrAttrValArr.get_values();

    const end = start + size;
    while(start != end) {
        const ptr = *start;
        arrValues.push(converter.convert_to_attr_value(builder, ptr.getType(), ptr))
        start++;
    }

    // construct the struct of MultipleAttributeValues
    const multiAttrValuesNode = builder.make_linked_type("MultipleAttributeValues", converter.multipleAttributeValueNode, location);
    const multiAttrStructVal = builder.make_struct_value(multiAttrValuesNode, converter.parent, location)

    multiAttrStructVal.add_value("data", ssrAttrValArr)
    multiAttrStructVal.add_value("size", builder.make_ubigint_value(size, location))

    return converter.wrapArgAttrValueVariantCall(builder, "Multiple", multiAttrStructVal);
}

func (converter : &mut AttrValueConverter) convert_to_attr_value(builder : *mut ASTBuilder, type : *mut BaseType, value : *mut Value) : *mut Value {
    switch(type.getKind()) {
        BaseTypeKind.Bool => {
            // bool value
            return converter.wrapArgAttrValueVariantCall(builder, "Bool", value);
        }
        BaseTypeKind.IntN => {
            const intN = type as *mut IntNType;
            const kind = intN.get_intn_type_kind()
            if(kind == IntNTypeKind.Char || kind == IntNTypeKind.UChar) {
                // char value
                return converter.wrapArgAttrValueVariantCall(builder, "Char", value);
            } else if(kind <= IntNTypeKind.Int128) {
                // signed integer
                return converter.wrapArgAttrValueVariantCall(builder, "Integer", value);
            } else {
                // unsigned integer
                return converter.wrapArgAttrValueVariantCall(builder, "UInteger", value);
            }
        }
        BaseTypeKind.Float => {
            // this has a precision arg, but we don't need to provide it
            return converter.wrapArgAttrValueVariantCall(builder, "Double", value);
        }
        BaseTypeKind.Double => {
            // this has a precision arg, but we don't need to provide it
            return converter.wrapArgAttrValueVariantCall(builder, "Double", value);
        }
        BaseTypeKind.ExpressiveString => {
            if(value.getKind() == ValueKind.ExpressiveString) {
                const exprString = value as *mut ExpressiveString
                const values = exprString.getValues()
                return converter.convert_multiple_attr_values(builder, values.data(), values.size())
            } else {
                // TODO: error out, cannot handle comptime functions that return expressive strings yet !
                return null;
            }
        }
        BaseTypeKind.Linked => {
            const linked = type as *mut LinkedType;
            const node = linked.getLinkedNode();
            return converter.convert_node_attr_value(builder, type, node, value);
        }
        BaseTypeKind.Generic => {
            const generic = type as *mut GenericType;
            const linked = generic.getLinkedType();
            const node = linked.getLinkedNode();
            return converter.convert_node_attr_value(builder, type, node, value);
        }
        BaseTypeKind.Reference => {
            const refType = type as *mut ReferenceType
            const childType = refType.getChildType()
            return converter.convert_attr_ref_child_type(builder, childType, value)
        }
        default => {
            // TODO: check this branch
            return value;
        }
    }
}