// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"


AddrOfValue::AddrOfValue(
        Value* value,
        CSTToken* token
) : value(value), token(token), _ptr_type(hybrid_ptr<BaseType>{ nullptr, false}, token) {

}

bool AddrOfValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr, BaseType *expected_type) {
    auto res = value->link(linker, value);
    if(res) {
        _ptr_type.type = hybrid_ptr<BaseType> { value->known_type(), false };
    }
    return res;
}

AddrOfValue *AddrOfValue::copy() {
    return new AddrOfValue(
            std::unique_ptr<Value>(value->copy()),
            token
    );
}