// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class ValueWrapperNode : public ASTNode {
public:

    Value* value;

    constexpr ValueWrapperNode(
        Value* value,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::ValueWrapper, parent_node, value->encoded_location()), value(value) {

    }

    ValueWrapperNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(
            value->copy(allocator),
            parent()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        const auto inst = value->llvm_value(gen);
        value->llvm_destruct(gen, inst);
    }

#endif

};