// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "ast/structures/GenericFuncDecl.h"

class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
public:

    ASTAllocator& allocator;
    /**
     * constructor
     * the allocator must be an ast allocator
     */
    GenericInstantiator(ASTAllocator& allocator) : allocator(allocator) {

    }

    void VisitFunctionParam(FunctionParam *param);

    void VisitIsValue(IsValue* value);

    void VisitSizeOfValue(SizeOfValue* value);

    void VisitAlignOfValue(AlignOfValue* value);

    FunctionDeclaration* Instantiate(GenericFuncDecl* decl, size_t itr);

};