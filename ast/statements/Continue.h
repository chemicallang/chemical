// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class ContinueStatement : public ASTNode {
private:
    LoopASTNode *node;
public:

    ASTNode* parent_node;

    /**
     * Construct a new ContinueStatement object.
     */
    constexpr ContinueStatement(
        LoopASTNode *node,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ContinueStmt, location), node(node), parent_node(parent_node) {

    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope &scope) final;

};