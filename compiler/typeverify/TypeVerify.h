// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class TypeVerifier : public RecursiveVisitor<TypeVerifier> {
public:

    /**
     * the allocator which allows to allocate memory for all instantiations
     */
    ASTAllocator& allocator;

    /**
     * the diagnoser to report errors
     */
    ASTDiagnoser& diagnoser;

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    TypeVerifier(
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser
    ) : allocator(allocator), diagnoser(diagnoser) {

    }

    void VisitStructValue(StructValue *val);

    void VisitArrayValue(ArrayValue *val);

    void VisitFunctionCall(FunctionCall *call);


};