// Copyright (c) Chemical Language Foundation 2025.

#include "NewValue.h"
#include "PlacementNewValue.h"
#include "NewTypedValue.h"
#include "ast/types/PointerType.h"

bool NewTypedValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    return type->link(linker);
}

bool NewValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    return value->link(linker, value, nullptr);
}

bool PlacementNewValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    const auto a = pointer->link(linker, pointer, nullptr);
    const auto b = value->link(linker, value, nullptr);
    return a && b;
}

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