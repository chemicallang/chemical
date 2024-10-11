// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"
#include "compiler/SymbolResolver.h"


AddrOfValue::AddrOfValue(
        Value* value,
        CSTToken* token
) : value(value), token(token), _ptr_type(nullptr, token) {

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
    return new (allocator.allocate<PointerType>()) PointerType(value->create_type(allocator), nullptr, is_value_mutable);
}

AddrOfValue *AddrOfValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<AddrOfValue>()) AddrOfValue(
            value->copy(allocator),
            token
    );
}