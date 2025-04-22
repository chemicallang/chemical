// Copyright (c) Chemical Language Foundation 2025.

#include "RetStructParamValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

BaseType* RetStructParamValue::create_type(ASTAllocator& allocator) {
    return new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(), false);
}