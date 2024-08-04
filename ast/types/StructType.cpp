// Copyright (c) Qinetik 2024.

#include "ReferencedStructType.h"
#include "ast/structures/VariablesContainer.h"
#include "ast/structures/StructDefinition.h"

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

ReferencedStructType::ReferencedStructType(StructDefinition* definition, int16_t iteration) : definition(definition), generic_iteration(iteration) {

}

VariablesContainer *ReferencedStructType::variables_container() {
    return definition;
}

std::string ReferencedStructType::struct_name() {
    return definition->name;
}

BaseType *ReferencedStructType::copy() const {
    return new ReferencedStructType(definition, generic_iteration);
}

ASTNode *ReferencedStructType::linked_node() {
    return definition;
}