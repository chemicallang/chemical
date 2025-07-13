// Copyright (c) Chemical Language Foundation 2025.

#include "ast/structures/VariablesContainer.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/symres/LinkSignatureAPI.h"

bool StructType::equals(StructType *type) {
    auto& elem_types = variables();
    auto& other_elem_types = type->variables();
    if (elem_types.size() != other_elem_types.size()) return false;
    unsigned i = 0;
    const auto total = elem_types.size();
    while(i < total) {
        const auto first = elem_types[i]->known_type();
        const auto second = other_elem_types[i]->known_type();
        if(!first->is_same(second)) {
            return false;
        }
        i++;
    }
    return true;
}

bool StructType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::Struct:
            return equals(type->as_struct_type_unsafe());
        case BaseTypeKind::Linked:
            return type->as_linked_type_unsafe()->linked == this;
        default:
            return false;
    }
}