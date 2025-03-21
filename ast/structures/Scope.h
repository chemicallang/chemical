// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Scope : public ASTNode {
public:

    bool stoppedInterpretOnce = false;
    std::vector<ASTNode*> nodes;


    /**
     * empty constructor
     */
    constexpr Scope(ASTNode* parent_node, SourceLocation location) : ASTNode(ASTNodeKind::Scope, parent_node, location) {

    }

    /**
     * constructor
     */
    constexpr Scope(std::vector<ASTNode*> nodes, ASTNode* parent_node, SourceLocation location) : ASTNode(ASTNodeKind::Scope, parent_node, location), nodes(std::move(nodes)) {

    }

    /**
     * move constructor
     */
    Scope(Scope&& other) : ASTNode(other.kind(), other.parent(), other.encoded_location()), nodes(std::move(other.nodes)) {

    };

    /**
     * move assignment
     */
    Scope& operator=(Scope&& other) noexcept {
        nodes = std::move(other.nodes);
        set_parent(other.parent());
        set_encoded_location(other.encoded_location());
        return *this;
    }

    inline void shallow_copy_into(Scope& other) const {
        other.nodes = nodes;
    }

    void copy_into(Scope& other, ASTAllocator& allocator, ASTNode* new_parent) const {
        other.nodes.reserve(nodes.size());
        for(auto& node : nodes) {
            const auto copied = node->copy(allocator);
            copied->set_parent(new_parent);
            other.nodes.emplace_back(copied);
        }
    }

    Scope shallow_copy() const {
        Scope copied(parent(), encoded_location());
        shallow_copy_into(copied);
        return copied;
    }

    Scope* copy(ASTAllocator &allocator) override {
        const auto scope = new (allocator.allocate<Scope>()) Scope(parent(), encoded_location());
        scope->nodes = nodes;
        for(auto& node : scope->nodes) {
            node = node->copy(allocator);
        }
        return scope;
    }

    /**
     * top level nodes declaration function
     */
    void tld_declare(SymbolResolver &linker);

    /**
     * links the signatures of all nodes
     */
    void link_signature(SymbolResolver& linker) final;

    /**
     * links everything in this scope
     */
    void declare_and_link(SymbolResolver &linker);

    /**
     * declares top level nodes
     */
    inline void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final {
        tld_declare(linker);
    }

    /**
     * links nodes
     */
    inline void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final {
        declare_and_link(linker);
    }

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
    void link_asynchronously(SymbolResolver &linker) {
        // declare all the top level symbols
        tld_declare(linker);
        // link the signatures of functions and structs
        link_signature(linker);
        // link the bodies
        declare_and_link(linker);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

    void code_gen_no_scope(Codegen &gen, unsigned destruct_begin);

    void code_gen(Codegen &gen, unsigned destruct_begin);

#endif

    /**
     * function is supposed to implemented by other scopes
     * like loop scope, which can be stopped in the middle of the loop
     */
    void stopInterpretOnce();

};

/**
 * nodes will be deduplicated, nodes having same identifier
 * that appear later, will override the nodes that appear before
 */
void top_level_dedupe(std::vector<ASTNode*>& nodes);