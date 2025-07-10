// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"

class ForLoop : public LoopASTNode {
public:

    ASTNode* initializer;
    Value* conditionExpr;
    ASTNode* incrementerExpr;
    bool stoppedInterpretation = false;

    /**
     * @brief Construct a new ForLoop object.
     */
    constexpr ForLoop(
            ASTNode* initializer,
            Value* conditionExpr,
            ASTNode* incrementerExpr,
            ASTNode* parent_node,
            SourceLocation location
    ) : LoopASTNode(ASTNodeKind::ForLoopStmt, parent_node, location), initializer(initializer), conditionExpr(conditionExpr),
        incrementerExpr(incrementerExpr) {

    }

    ForLoop* copy(ASTAllocator &allocator) override {
        const auto copied_init = initializer->copy(allocator);
        const auto copied_incrementerExpr = incrementerExpr->copy(allocator);
        const auto loop = new (allocator.allocate<ForLoop>()) ForLoop(
            copied_init,
            conditionExpr->copy(allocator),
            copied_incrementerExpr,
            parent(),
            encoded_location()
        );
        copied_init->set_parent(loop);
        copied_incrementerExpr->set_parent(loop);
        body.copy_into(loop->body, allocator, loop);
        return loop;
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void stopInterpretation() final;

};