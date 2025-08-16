// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/VariableIdentifier.h"

class ExtendableMembersContainerNode;

class DeallocStmt : public ASTNode {
public:

    /**
     * the pointer to deallocate
     */
    Value* ptr;

    /**
     * constructor
     */
    constexpr DeallocStmt(
        Value* value,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::DeallocStmt, parent_node, location), ptr(value) {

    }

    DeallocStmt* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<DeallocStmt>()) DeallocStmt(
            ptr->copy(allocator),
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};