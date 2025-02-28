// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class BaseGenericDecl : public ASTNode {
public:

    std::vector<GenericTypeParameter*> generic_params;

    inline constexpr BaseGenericDecl(ASTNodeKind k, SourceLocation location) : ASTNode(k, location) {

    }

};