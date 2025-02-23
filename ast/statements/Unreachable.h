// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class UnreachableStmt : public ASTNode {
public:

    ASTNode* parent_node;
    SourceLocation location;

    /**
     * Construct a new ContinueStatement object.
     */
    UnreachableStmt(
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnreachableStmt), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

};