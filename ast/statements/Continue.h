// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class ContinueStatement : public ASTNode {
public:

    /**
     * Construct a new ContinueStatement object.
     */
    constexpr ContinueStatement(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ContinueStmt, parent_node, location) {

    }

    ContinueStatement* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ContinueStatement>()) ContinueStatement(
            parent(),
            encoded_location()
        );
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope &scope) final;

};