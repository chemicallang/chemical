// Copyright (c) Chemical Language Foundation 2025.

#include "NewValue.h"
#include "PlacementNewValue.h"
#include "NewTypedValue.h"
#include "ast/types/PointerType.h"

BaseType* NewTypedValue::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<PointerType>()) PointerType(type, true);
}

BaseType* NewValue::create_type(ASTAllocator &allocator) {
    auto type = value->create_type(allocator);
    return new (allocator.allocate<PointerType>()) PointerType(type, true);
}

BaseType* PlacementNewValue::create_type(ASTAllocator &allocator) {
    auto type = value->create_type(allocator);
    return new (allocator.allocate<PointerType>()) PointerType(type, true);
}

BaseType* NewTypedValue::known_type() {
    ptr_type.type = type;
    return &ptr_type;
}

BaseType* NewValue::known_type() {
    auto type = value->known_type();
    ptr_type.type = type;
    return &ptr_type;
}

BaseType* PlacementNewValue::known_type() {
    auto type = value->known_type();
    ptr_type.type = type;
    return &ptr_type;
}