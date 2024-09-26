// Copyright (c) Qinetik 2024.

#include "LinkedType.h"
#include "ast/statements/Typealias.h"
#include "StructType.h"
#include "compiler/SymbolResolver.h"

uint64_t LinkedType::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

ValueType LinkedType::value_type() const {
    return linked->value_type();
}

BaseType* LinkedType::pure_type() {
    if(linked) {
        return linked->known_type();
    } else {
        return nullptr;
    }
}

bool LinkedType::satisfies(ValueType value_type) {
    if(linked->as_typealias() != nullptr) {
        return ((TypealiasStatement*) linked)->actual_type->satisfies(value_type);
    } else {
        return linked->value_type() == value_type;
    };
}

bool LinkedType::satisfies(BaseType *other) {
    const auto other_kind = other->kind();
    if(other_kind == BaseTypeKind::Linked) {
        return linked == ((LinkedType*) other)->linked;
    } else if(other_kind == BaseTypeKind::Struct) {
        return linked == ((StructType*) other)->linked_node();
    }
    auto known = linked->known_type();
    if(known != this) {
        return known->satisfies(other);
    }
    return false;
}

void LinkedType::link(SymbolResolver &linker) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find referenced type " + type, this);
    }
}

ASTNode *LinkedType::linked_node() {
    return linked;
}