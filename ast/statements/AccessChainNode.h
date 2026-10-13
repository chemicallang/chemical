// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/values/AccessChain.h"

class AccessChainNode : public ASTNode {
public:

    AccessChain chain;

    constexpr AccessChainNode(
        SourceLocation loc,
        ASTNode* parent_node
    ) : ASTNode(ASTNodeKind::AccessChainNode, parent_node, loc), chain(loc) {

    }

    AccessChainNode* copy(ASTAllocator &allocator) override {
        const auto node = new (allocator.allocate<AccessChainNode>()) AccessChainNode(
            encoded_location(),
            parent()
        );
        node->chain.setType(chain.getType());
        for(const auto value : chain.values) {
            node->chain.values.emplace_back((Value*) value->copy(allocator));
        }
        return node;
    }


};