// Copyright (c) Chemical Language Foundation 2025.

#include "UnionType.h"

BaseType* UnionTypeCopy::copy(ASTAllocator &allocator) {
    const auto type = (UnionType*) this;
    return new (allocator.allocate<UnionType>()) UnionType(type->name, type->parent(), type->encoded_location());
}