// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class BaseGenericDecl : public ASTNode {
public:

    std::vector<GenericTypeParameter*> generic_params;

    /**
     * constructor
     */
    inline constexpr BaseGenericDecl(
        ASTNodeKind k,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(k, parent_node, location) {

    }

};