// Copyright (c) Qinetik 2024.

#include "ast/base/BaseType.h"
#include "NotValue.h"

void NotValue::link(SymbolResolver &linker) {
    value->link(linker);
}

bool NotValue::primitive() {
    return false;
}

std::unique_ptr<BaseType> NotValue::create_type() const {
    return value->create_type();
}

std::string NotValue::representation() const {
    std::string rep;
    rep.append(1, '!');
    rep.append(value->representation());
    return rep;
}