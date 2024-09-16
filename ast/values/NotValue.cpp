// Copyright (c) Qinetik 2024.

#include "ast/base/BaseType.h"
#include "NotValue.h"

bool NotValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

bool NotValue::primitive() {
    return false;
}

std::unique_ptr<BaseType> NotValue::create_type() {
    return value->create_type();
}

hybrid_ptr<BaseType> NotValue::get_base_type() {
    return value->get_base_type();
}

BaseType* NotValue::known_type() {
    return value->known_type();
}