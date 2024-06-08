// Copyright (c) Qinetik 2024.

#include "Negative.h"
#include "ast/base/BaseType.h"

uint64_t NegativeValue::byte_size(bool is64Bit) const {
// TODO check this out
    return value->byte_size(is64Bit);
}

void NegativeValue::link(SymbolResolver &linker) {
    value->link(linker);
}

std::unique_ptr<BaseType> NegativeValue::create_type() const {
    return value->create_type();
}

bool NegativeValue::primitive() {
    return false;
}

std::string NegativeValue::representation() const {
    std::string rep;
    rep.append(1, '-');
    rep.append(value->representation());
    return rep;
}