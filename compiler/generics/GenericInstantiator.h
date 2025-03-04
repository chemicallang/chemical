// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "compiler/symres/SymbolTable.h"

class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
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
     * the symbol table to use for declaring and resolving nodes
     */
    SymbolTable table;

    /**
     * this points to the node being instantiated
     * this allows us to check self-referential pointers to generic decls
     */
    BaseGenericDecl* current_gen = nullptr;

    /**
     * allows relinking generic type pointers to it's new implementation
     */
    ASTNode* current_impl_ptr = nullptr;

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    GenericInstantiator(
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser
    ) : allocator(allocator), diagnoser(diagnoser), table() {

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
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    void VisitLinkedType(LinkedType* type);

    void VisitGenericType(GenericType* type);

    void VisitScope(Scope* node) {
        table.scope_start();
        RecursiveVisitor<GenericInstantiator>::VisitScope(node);
        table.scope_end();
    }

    void VisitVarInitStmt(VarInitStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitVarInitStmt(node);
        table.declare(node->name_view(), node);
    }

    void VisitTypealiasStmt(TypealiasStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitTypealiasStmt(node);
        table.declare(node->name_view(), node);
    }

    void VisitAccessChain(AccessChain* value);

    void VisitFunctionCall(FunctionCall *call);

    FunctionDeclaration* Instantiate(GenericFuncDecl* decl, size_t itr);

    StructDefinition* Instantiate(GenericStructDecl* decl, size_t itr);

};