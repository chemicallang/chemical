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

    static BaseType* get_concrete_gen_type(BaseType* type);

    // We want to override visit, what we want is a BaseType*& so we can replace
    // every BaseType*& with the appropriate concrete implementation if it's referencing a generic type

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
    inline void visit(BaseFunctionParam* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(ChainValue* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(BaseType*& type_ref) {
        // find out concrete type if it's a generic type parameter referencing type
        const auto concrete = GenericInstantiator::get_concrete_gen_type(type_ref);
        if(concrete) {
            // then replace the type pointer ref with the concrete type
            type_ref = concrete;
        }
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    FunctionDeclaration* Instantiate(GenericFuncDecl* decl, size_t itr);

};