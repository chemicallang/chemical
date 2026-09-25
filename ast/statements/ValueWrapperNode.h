// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"

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
        // `f().member` destroys the call's temporary (and with it the member)
        // while the value is loaded, so the value is an alias into that
        // destroyed temporary and must not be destroyed a second time
        if(value->val_kind() == ValueKind::AccessChain && value->as_access_chain_unsafe()->is_alias_into_destroyed_temp()) {
            return;
        }
        value->llvm_destruct(gen, inst);
    }

#endif

};