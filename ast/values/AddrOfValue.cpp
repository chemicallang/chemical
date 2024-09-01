// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"


AddrOfValue::AddrOfValue(
        std::unique_ptr<Value> value,
        CSTToken* token
) : value(std::move(value)), token(token) {

}

bool AddrOfValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    return value->link(linker, value);
}

AddrOfValue *AddrOfValue::copy() {
    return new AddrOfValue(
            std::unique_ptr<Value>(value->copy()),
            token
    );
}