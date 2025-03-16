// Copyright (c) Chemical Language Foundation 2025.

#include "StringType.h"
#include "CharType.h"

BaseType* StringType::create_child_type(ASTAllocator& allocator) const {
    return new (allocator.allocate<CharType>()) CharType(encoded_location());
}

BaseType* StringType::known_child_type() {
    return (BaseType*) &CharType::instance;
}