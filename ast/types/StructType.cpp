// Copyright (c) Qinetik 2024.

#include "StructType.h"
#include "ast/structures/VariablesContainer.h"

bool StructType::equals(StructType *type) {
    auto& elem_types = variables_container()->variables;
    auto& other_elem_types = type->variables_container()->variables;
    if (elem_types.size() != other_elem_types.size()) return false;
    unsigned i = 0;
    auto itr_first = elem_types.begin();
    auto itr_second = other_elem_types.begin();
    while(i < elem_types.size()) {
        auto first = itr_first->second->get_value_type();
        auto second = itr_second->second->get_value_type();
        if(!first->is_same(second.get())) {
            return false;
        }
        itr_first++;
        itr_second++;
        i++;
    }
    return true;
}