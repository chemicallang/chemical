// Copyright (c) Chemical Language Foundation 2025.

#include "NullValue.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "ast/types/NullPtrType.h"

BaseType* NullValue::create_type(ASTAllocator &allocator) {
    return new (allocator.allocate<NullPtrType>()) NullPtrType();
}

BaseType* NullValue::known_type() {
    return (NullPtrType*) &NullPtrType::instance;
}