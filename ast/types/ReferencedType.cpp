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

BaseType* ReferencedType::pure_type() {
    return linked->known_type();
}

bool ReferencedType::satisfies(ValueType value_type) {
    if(linked->as_typealias() != nullptr) {
        return ((TypealiasStatement*) linked)->actual_type->satisfies(value_type);
    } else {
        return linked->create_value_type()->satisfies(value_type);
    };
}

bool ReferencedType::satisfies(BaseType *type) {
    const auto value_type = linked->get_value_type();
    return value_type->satisfies(type);
}

void ReferencedType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find referenced type " + type, this);
    }
}

ASTNode *ReferencedType::linked_node() {
    return linked;
}