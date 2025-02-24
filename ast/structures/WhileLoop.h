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
    ASTNode* parent_node;

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(
        Value* condition,
        Scope body,
        ASTNode* parent_node,
        SourceLocation location
    ) : LoopASTNode(std::move(body), ASTNodeKind::WhileLoopStmt, location), condition(condition), parent_node(parent_node) {}


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void stopInterpretation() final;

};