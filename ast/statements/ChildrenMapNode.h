// Copyright (c) Chemical Language Foundation 2026.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include <unordered_map>

/**
 * node solely used to contain children, we store the pointer to this node
 * in another map for caching
 */
class ChildrenMapNode final : public ASTNode {
public:

    /**
     * symbols are stored in this map
     */
    std::unordered_map<chem::string_view, ASTNode*> symbols;

    /**
     * constructor
     */
    ChildrenMapNode(
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ChildrenMapNode, parent_node, location) {}

};