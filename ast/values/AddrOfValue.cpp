// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"


AddrOfValue::AddrOfValue(
        std::unique_ptr<Value> value
) : value(std::move(value)) {

}

void AddrOfValue::link(SymbolResolver &linker) {
    value->link(linker);
}

Value *AddrOfValue::copy() {
    return new AddrOfValue(
            std::unique_ptr<Value>(value->copy())
    );
}