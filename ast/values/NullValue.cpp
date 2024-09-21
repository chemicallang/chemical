// Copyright (c) Qinetik 2024.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"

BaseType* NullValue::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<PointerType>()) PointerType(new (allocator.allocate<VoidType>()) VoidType(nullptr), nullptr);
}

BaseType* NullValue::known_type() {
    return (BaseType*) &PointerType::void_ptr_instance;
}