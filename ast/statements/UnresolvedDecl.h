// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class UnresolvedDecl : public ASTNode {
public:

    VoidType* voidTy;

    UnresolvedDecl(
            ASTNode* parent,
            VoidType* voidTy,
            SourceLocation loc
    ) : ASTNode(ASTNodeKind::UnresolvedDecl, parent, loc), voidTy(voidTy) {

    }

    BaseType* known_type() override {
        return (BaseType*) voidTy;
    }

};