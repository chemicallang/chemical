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
    std::unique_ptr<Value> value;

    /**
     * @brief Construct a new Break statement object.
     */
    BreakStatement(LoopASTNode *node, ASTNode* parent_node);

    ASTNodeKind kind() override {
        return ASTNodeKind::BreakStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) override;

    void interpret(InterpretScope &scope) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};