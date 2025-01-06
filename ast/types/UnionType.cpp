// Copyright (c) Qinetik 2024.

#include "UnionType.h"
#include "ast/structures/VariablesContainer.h"

bool UnionType::satisfies(ValueType type) {
    for (auto &member: variables) {
        auto mem_type = member.second->known_type();
        if (mem_type->satisfies(type)) {
            return true;
        }
    }
    return false;
}