// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "ast/structures/GenericFuncDecl.h"

class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
public:

    ASTAllocator& allocator;

    /**
     * the replacement pointer is the pointer to the location where the holder of the current
     * type is, we can replace the holder of the type with a new shallow clone that contains a different
     * type, this is how we change types of nodes
     */
    ASTAny** replacement_pointer = nullptr;

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    GenericInstantiator(ASTAllocator& allocator) : allocator(allocator) {

    }

    /**
     * we override the visit method, non recursive visitor calls this method
     * this then calls appropriate method according to type to visit this type
     */
    template<typename T>
    inline void visit(T*& ptr) {
        const auto prev_ptr = replacement_pointer;
        replacement_pointer = ((ASTAny**) (&ptr));
        RecursiveVisitor<GenericInstantiator>::visit(ptr);
        replacement_pointer = prev_ptr;
    }

    void VisitFunctionParam(FunctionParam *param);

    FunctionDeclaration* Instantiate(GenericFuncDecl* decl, std::vector<BaseType*>& gen_args);

};