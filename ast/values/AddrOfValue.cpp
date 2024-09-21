// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"


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
    return res;
}

AddrOfValue *AddrOfValue::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<AddrOfValue>()) AddrOfValue(
            value->copy(allocator),
            token
    );
}