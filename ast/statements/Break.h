// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class BreakStatement : public ASTNode {
public:

    /**
     * sometimes a break statement can break with a value, in cases where break is within
     * a loop block, that is itself a value, so this value is assigned to the variable owning the loop
     * block and then the loop is broken
     */
    Value* value;

    /**
     * constructor
     */
    constexpr BreakStatement(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::BreakStmt, parent_node, location), value(nullptr) {

    }

    /**
     * constructor
     */
    constexpr BreakStatement(
            Value* value,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::BreakStmt, parent_node, location), value(value) {

    }

    ASTNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<BreakStatement>()) BreakStatement(
            value ? value->copy(allocator) : nullptr,
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};