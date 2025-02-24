// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class UnreachableStmt : public ASTNode {
public:

    ASTNode* parent_node;

    /**
     * Construct a new ContinueStatement object.
     */
    UnreachableStmt(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnreachableStmt, location), parent_node(parent_node) {

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

};