// Copyright (c) Qinetik 2025.

#include "BlockValue.h"
#include "compiler/SymbolResolver.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/types/VoidType.h"

BaseType* BlockValue::create_type(ASTAllocator &allocator) {
    if(calculated_value) {
        return calculated_value->create_type(allocator);
    } else {
        return new (allocator.allocate<VoidType>()) VoidType();
    }
}