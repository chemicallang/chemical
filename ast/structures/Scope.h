// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

    std::vector<std::unique_ptr<ASTNode>> nodes;
    ASTNode* parent_node;

    /**
     * empty constructor
     */
    Scope(ASTNode* parent_node) : parent_node(parent_node) {

    }

    /**
     * @brief Construct a new Scope object.
     * @param nodes All the ASTNode(s) present in the scope
     */
    Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node);

    /**
     * move constructor
     */
    Scope(Scope &&other) noexcept = default;

    /**
     * move assignment constructor
     */
    Scope& operator =(Scope &&other) noexcept = default;

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    /**
     * a scope's declare_top_level will be called to link all the nodes
     */
    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void interpret(InterpretScope &scope) override;

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    virtual void stopInterpretOnce();

};