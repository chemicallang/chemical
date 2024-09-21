// Copyright (c) Qinetik 2024.

#include "UnionType.h"
#include "ast/structures/VariablesContainer.h"

uint64_t UnionType::byte_size(bool is64Bit) {
    uint64_t size = 0;
    uint64_t previous;
    auto container = variables_container();
    for (auto &mem: container->variables) {
        previous = mem.second->byte_size(is64Bit);
        if (previous > size) {
            size = previous;
        }
    }
    return size;
}

bool UnionType::satisfies(ValueType type) {
    auto container = variables_container();
    for (auto &member: container->variables) {
        auto mem_type = member.second->known_type();
        if (mem_type->satisfies(type)) {
            return true;
        }
    }
    return false;
}