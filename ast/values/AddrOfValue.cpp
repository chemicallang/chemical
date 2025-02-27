// Copyright (c) Chemical Language Foundation 2025.

#include "AddrOfValue.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/ASTNode.h"
#include "ast/types/ReferenceType.h"
#include "ast/structures/BaseFunctionParam.h"

bool AddrOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    auto res = value->link(linker, value);
    if(res) {

        // reporting parameters that their address has been taken
        // which allows them to generate a variable and store themselves onto it
        // if they are of integer types
        // before the taking of address, the variables act immutable
        const auto linked = value->linked_node();
        if(linked) {
            switch (linked->kind()) {
                case ASTNodeKind::FunctionParam:
                case ASTNodeKind::ExtensionFuncReceiver:
                    linked->as_base_func_param_unsafe()->set_has_address_taken(true);
                    break;
                default:
                    break;
            }
        }

        _ptr_type.type = value->known_type();
    }
    is_value_mutable = value->check_is_mutable(linker.allocator, true);
    return res;
}

BaseType* AddrOfValue::create_type(ASTAllocator& allocator) {
    auto elem_type = value->create_type(allocator);
    const auto elem_type_kind = elem_type->kind();
    if(elem_type_kind == BaseTypeKind::Reference) {
        elem_type = ((ReferenceType*) elem_type)->type;
    }
    return new (allocator.allocate<PointerType>()) PointerType(elem_type, encoded_location(), is_value_mutable);
}

AddrOfValue *AddrOfValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<AddrOfValue>()) AddrOfValue(
            value->copy(allocator),
            encoded_location()
    );
}