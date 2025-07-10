// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/LoopASTNode.h"
#include "Scope.h"

class DoWhileLoop : public LoopASTNode {
public:

    bool stoppedInterpretation = false;

    Value* condition;


    /**
     * constructor
     */
    constexpr DoWhileLoop(
            Value* condition,
            ASTNode* parent_node,
            SourceLocation location
    ) : condition(condition), LoopASTNode(ASTNodeKind::DoWhileLoopStmt, parent_node, location) {

    }

    DoWhileLoop* copy(ASTAllocator &allocator) override {
        const auto loop = new (allocator.allocate<DoWhileLoop>()) DoWhileLoop(
            condition->copy(allocator),
            parent(),
            encoded_location()
        );
        body.copy_into(loop->body, allocator, loop);
        return loop;
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void stopInterpretation() final;

};