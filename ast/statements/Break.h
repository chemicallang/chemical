// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class BreakStatement : public ASTNode {
private:
    LoopASTNode *node;
public:

    ASTNode* parent_node;
    /**
     * sometimes a break statement can break with a value, in cases where break is within
     * a loop block, that is itself a value, so this value is assigned to the variable owning the loop
     * block and then the loop is broken
     */
    Value* value;

    /**
     * @brief Construct a new Break statement object.
     */
    BreakStatement(
        LoopASTNode *node,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::BreakStmt, location), node(node), parent_node(parent_node), value(nullptr) {

    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};