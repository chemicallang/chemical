// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymResLinkBody : public RecursiveVisitor<SymResLinkBody> {
public:

    SymbolResolver& resolver;

    /**
     * this is set before visiting type
     */
    SourceLocation type_location = 0;

    /**
     * constructor
     */
    SymResLinkBody(SymbolResolver& resolver) : resolver(resolver) {

    }

    template<typename T>
    inline void visit(T* ptr) {
        VisitByPtrTypeNoNullCheck(ptr);
    }
    inline void visit(ASTNode* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(BaseDefMember* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(ChainValue* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.encoded_location();
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

};