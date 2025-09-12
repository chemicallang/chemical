// Copyright (c) Chemical Language Foundation 2025.

#include "StringType.h"
#include "IntNType.h"

const CharType instance;

BaseType* StringType::create_child_type(ASTAllocator& allocator) const {
    return new (allocator.allocate<CharType>()) CharType();
}

BaseType* StringType::known_child_type() {
    return (BaseType*) &instance;
}