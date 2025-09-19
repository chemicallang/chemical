// Copyright (c) Chemical Language Foundation 2025.

#include "DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"

bool DynamicType::satisfies(BaseType *type) {
    const auto type_kind = type->kind();
    if(type_kind != BaseTypeKind::Dynamic) {
        return false;
    }
    return referenced->satisfies(((DynamicType*) type)->referenced);
}