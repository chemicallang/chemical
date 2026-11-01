// Copyright (c) Chemical Language Foundation 2025.

#include "DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "compiler/lab/TargetData.h"

bool DynamicType::satisfies(BaseType *type) {
    const auto type_kind = type->kind();
    if(type_kind != BaseTypeKind::Dynamic) {
        return false;
    }
    return referenced->satisfies(((DynamicType*) type)->referenced);
}

uint64_t DynamicType::byte_size(const TargetData& target) {
    // `dyn I` is always a two-word fat pointer: { data, vtable }.
    // The size follows the target pointer width, not a hard-coded 64-bit ABI.
    return target.is64Bit ? 16 : 8;
}