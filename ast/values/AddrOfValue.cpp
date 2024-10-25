// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferenceType.h"


AddrOfValue::AddrOfValue(
    Value* value,
    SourceLocation location
) : value(value), location(location), _ptr_type(nullptr, location) {

}

bool AddrOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    auto res = value->link(linker, value);
    if(res) {
        _ptr_type.type = value->known_type();
    }
    is_value_mutable = value->check_is_mutable(linker.current_func_type, linker, true);
    return res;
}

BaseType* AddrOfValue::create_type(ASTAllocator& allocator) {
    auto elem_type = value->create_type(allocator);
    const auto elem_type_kind = elem_type->kind();
    if(elem_type_kind == BaseTypeKind::Reference) {
        elem_type = ((ReferenceType*) elem_type)->type;
    }
    return new (allocator.allocate<PointerType>()) PointerType(elem_type, location, is_value_mutable);
}

AddrOfValue *AddrOfValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<AddrOfValue>()) AddrOfValue(
            value->copy(allocator),
            location
    );
}