// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

    std::vector<ASTNode*> nodes;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * empty constructor
     */
    Scope(ASTNode* parent_node, SourceLocation location) : parent_node(parent_node), location(location) {

    }

    /**
     * constructor
     */
    Scope(std::vector<ASTNode*> nodes, ASTNode* parent_node, SourceLocation location);

    /**
     * deleted copy constructor
     */
    Scope(const Scope& other) = delete;

    Scope(Scope&& other) = default;

    Scope& operator=(Scope&&) = default;

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::Scope;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

    /**
     * throws an error in debug mode, shouldn't be called
     */
    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    /**
     * throws an error in debug mode, shouldn't be called
     */
    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

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

    void code_gen(Codegen &gen) final;

    void code_gen(Codegen &gen, unsigned destruct_begin);

#endif

    void interpret(InterpretScope &scope);

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    virtual void stopInterpretOnce();

};

/**
 * nodes will be deduplicated, nodes having same identifier
 * that appear later, will override the nodes that appear before
 */
void top_level_dedupe(std::vector<ASTNode*>& nodes);