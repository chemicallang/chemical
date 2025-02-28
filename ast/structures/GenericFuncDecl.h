// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseGenericDecl.h"

class GenericFuncDecl : public BaseGenericDecl {
public:

    /**
     * master implementation is the first implementation we encounter
     */
    FunctionDeclaration* master_impl;

    GenericFuncDecl(SourceLocation location) : BaseGenericDecl(ASTNodeKind::GenericFuncDecl, location) {

    }

#ifdef COMPILER_BUILD



#endif

};

