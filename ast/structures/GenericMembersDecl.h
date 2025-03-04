// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseGenericDecl.h"

class GenericMembersDecl : public BaseGenericDecl {
public:

    constexpr GenericMembersDecl(
        ASTNodeKind k,
        ASTNode* parent_node,
        SourceLocation location
    ) : BaseGenericDecl(k, parent_node, location) {

    }

};