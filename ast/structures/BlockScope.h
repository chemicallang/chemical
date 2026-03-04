// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * a container that disposes symbols after the scope ends
 * this creates a scope and
 */
class BlockScope : public ASTNode {
public:
    /**
     * the nodes in the scope
     */
    std::vector<ASTNode*> nodes;
    /**
     * empty constructor
     */
    constexpr BlockScope(ASTNode* parent_node, SourceLocation location) : ASTNode(ASTNodeKind::Block, parent_node, location) {

    }
    /**
     * constructor
     */
    constexpr BlockScope(std::vector<ASTNode*> nodes, ASTNode* parent_node, SourceLocation location) : ASTNode(ASTNodeKind::Block, parent_node, location), nodes(std::move(nodes)) {

    }
    /**
     * move constructor
     */
    BlockScope(BlockScope&& other) : ASTNode(other.kind(), other.parent(), other.encoded_location()), nodes(std::move(other.nodes)) {

    };
    /**
     * move assignment
     */
    BlockScope& operator=(BlockScope&& other) noexcept {
        nodes = std::move(other.nodes);
        set_parent(other.parent());
        set_encoded_location(other.encoded_location());
        return *this;
    }
    /**
     * the copy function for block scope
     */
    BlockScope* copy(ASTAllocator &allocator) override {
        const auto scope = new (allocator.allocate<BlockScope>()) BlockScope(parent(), encoded_location());
        scope->nodes = nodes;
        for(auto& node : scope->nodes) {
            node = node->copy(allocator);
        }
        return scope;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};