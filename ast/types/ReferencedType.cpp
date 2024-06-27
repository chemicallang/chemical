// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"
#include "ast/statements/Typealias.h"
#include "compiler/SymbolResolver.h"

uint64_t ReferencedType::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

ValueType ReferencedType::value_type() const {
    return linked->value_type();
}

bool ReferencedType::satisfies(ValueType value_type) const {
    if(linked->as_typealias() != nullptr) {
        return ((TypealiasStatement*) linked)->actual_type->satisfies(value_type);
    } else {
        return linked->create_value_type()->satisfies(value_type);
    };
}

void ReferencedType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find referenced type " + type);
    }
}

ASTNode *ReferencedType::linked_node() {
    return linked;
}