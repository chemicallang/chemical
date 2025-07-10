// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/base/LoopASTNode.h"

class WhileLoop : public LoopASTNode {
public:

    Value* condition;
    bool stoppedInterpretation = false;

    /**
     * constructor
     */
    constexpr WhileLoop(
            Value* condition,
            ASTNode* parent_node,
            SourceLocation location
    ) : LoopASTNode(ASTNodeKind::WhileLoopStmt, parent_node, location), condition(condition) {

    }

    WhileLoop* copy(ASTAllocator &allocator) override {
        const auto loop = new (allocator.allocate<WhileLoop>()) WhileLoop(
            condition->copy(allocator), parent(), encoded_location()
        );
        body.copy_into(loop->body, allocator, loop);
        return loop;
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void stopInterpretation() final;

};