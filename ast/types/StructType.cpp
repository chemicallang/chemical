// Copyright (c) Qinetik 2024.

#include "ast/structures/VariablesContainer.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/SymbolResolver.h"

bool StructType::equals(StructType *type) {
    auto& elem_types = variables;
    auto& other_elem_types = type->variables;
    if (elem_types.size() != other_elem_types.size()) return false;
    unsigned i = 0;
    auto itr_first = elem_types.begin();
    auto itr_second = other_elem_types.begin();
    while(i < elem_types.size()) {
        auto first = itr_first->second->known_type();
        auto second = itr_second->second->known_type();
        if(!first->is_same(second)) {
            return false;
        }
        itr_first++;
        itr_second++;
        i++;
    }
    return true;
}

bool StructType::link(SymbolResolver &linker) {
    for(auto& var : variables) {
        if(!var.second->known_type()->link(linker)) {
            return false;
        }
    }
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}