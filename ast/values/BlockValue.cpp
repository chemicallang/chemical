// Copyright (c) Qinetik 2025.

#include "BlockValue.h"
#include "compiler/SymbolResolver.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/types/VoidType.h"

BaseType* BlockValue::create_type(ASTAllocator &allocator) {
    if(calculated_value) {
        return calculated_value->create_type(allocator);
    } else {
        return new (allocator.allocate<VoidType>()) VoidType(scope.encoded_location());
    }
}

bool BlockValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(scope.nodes.empty()) {
        linker.error("empty block value not allowed", this);
        return false;
    } else {
        scope.link_sequentially(linker);
    }
    const auto lastNode = scope.nodes.back();
    const auto lastKind = lastNode->kind();
    if(lastKind != ASTNodeKind::ValueWrapper) {
        linker.error("block doesn't contain a value wrapper as last node", this);
        return false;
    }
    const auto lastValNode = lastNode->as_value_wrapper_unsafe();
    calculated_value = lastValNode->value;
    // TODO return appropriate result by getting it from scope
    return true;
}