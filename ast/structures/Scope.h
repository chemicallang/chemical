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
    CSTToken* token;

    /**
     * empty constructor
     */
    Scope(ASTNode* parent_node, CSTToken* token) : parent_node(parent_node), token(token) {

    }

    /**
     * constructor
     */
    Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node, CSTToken* token);

    /**
     * deleted copy constructor
     */
    Scope(const Scope& other) = delete;

    Scope(Scope&& other) = default;

    Scope& operator=(Scope&&) = default;

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::Scope;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    /**
     * throws an error in debug mode, shouldn't be called
     */
    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    /**
     * throws an error in debug mode, shouldn't be called
     */
    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    /**
     * when nodes are to be declared and used sequentially, so node can be referenced
     * after it is declared, this method should be called
     * for example, this code, i should be declared first then incremented
     * var i = 0;
     * i++; <--- i is declared above (if it's below it shouldn't be referencable)
     */
    void link_sequentially(SymbolResolver &linker);

    /**
     * when nodes are to be declared and used asynchronously, so node can be referenced
     * before it is declared, this method should be called
     * for example, this code, function can be referenced before it's declared
     * func sum_twice() = sum() * 2;
     * func sum(); <--- sum is declared below (or after) sum_twice however still referencable
     */
    void link_asynchronously(SymbolResolver &linker);

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    void code_gen(Codegen &gen, unsigned destruct_begin);

#endif

    void interpret(InterpretScope &scope) override;

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    virtual void stopInterpretOnce();

};