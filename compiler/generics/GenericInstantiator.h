// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "compiler/symres/SymbolTable.h"
#include "compiler/generics/InstantiationsContainer.h"

class GenericInstantiator : public RecursiveVisitor<GenericInstantiator> {
public:

    /**
     * instantiations container contains types for each instantiation
     */
    InstantiationsContainer& container;

    /**
     * the allocator which allows to allocate memory for all instantiations
     */
    ASTAllocator* allocator_ptr;

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
        InstantiationsContainer& container,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser
    ) : container(container), allocator_ptr(&allocator), diagnoser(diagnoser), table() {

    }

    inline ASTAllocator& getAllocator() {
        return *allocator_ptr;
    }

    inline void setAllocator(ASTAllocator& allocator) {
        allocator_ptr = &allocator;
    }

    void make_gen_type_concrete(BaseType*& type);

    bool relink_identifier(VariableIdentifier* identifier) const;

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
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(ChainValue* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(BaseType*& type_ref) {
        GenericInstantiator::make_gen_type_concrete(type_ref);
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(TypeLoc& type) {
        auto changed = const_cast<BaseType*>(type.getType());
        visit(changed);
        if(changed != type.getType()) {
            type = { changed, type.getLocation() };
        }
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    void VisitLinkedType(LinkedType* type);

    void VisitGenericType(GenericType* type);

    void VisitStructValue(StructValue *val);

    void VisitScope(Scope* node) {
        table.scope_start();
        RecursiveVisitor<GenericInstantiator>::VisitScope(node);
        table.scope_end();
    }

    void VisitVarInitStmt(VarInitStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitVarInitStmt(node);
        if(!node->type) {
            node->type = {node->value->create_type(getAllocator()), node->type.getLocation()};
        }
        table.declare(node->name_view(), node);
    }

    void VisitTypealiasStmt(TypealiasStatement* node) {
        RecursiveVisitor<GenericInstantiator>::VisitTypealiasStmt(node);
        table.declare(node->name_view(), node);
    }

    void VisitSwitchStmt(SwitchStatement* node);

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        relink_identifier(value);
    }

    void VisitAccessChain(AccessChain* value);

    void VisitFunctionCall(FunctionCall *call);

    void activateIteration(BaseGenericDecl* gen_decl, size_t itr);

    void FinalizeSignature(TypealiasStatement* decl);

    void FinalizeSignature(GenericTypeDecl* gen_decl, TypealiasStatement* decl, size_t itr);

    /**
     * this function allows to finalize the signature of a non generic function that is inside
     * a generic struct / variable / union
     */
    void FinalizeSignature(FunctionDeclaration* decl);

    void FinalizeSignature(GenericFuncDecl* gen_decl, FunctionDeclaration* decl, size_t itr);

    void FinalizeBody(GenericFuncDecl* gen_decl, FunctionDeclaration* decl, size_t itr);

    void FinalizeSignature(GenericStructDecl* gen_decl, StructDefinition* decl, size_t itr);

    void FinalizeBody(GenericStructDecl* gen_decl, StructDefinition* decl, size_t itr);

    void FinalizeSignature(GenericUnionDecl* gen_decl, UnionDef* decl, size_t itr);

    void FinalizeBody(GenericUnionDecl* gen_decl, UnionDef* decl, size_t itr);

    void FinalizeSignature(GenericInterfaceDecl* gen_decl, InterfaceDefinition* decl, size_t itr);

    void FinalizeBody(GenericInterfaceDecl* gen_decl, InterfaceDefinition* decl, size_t itr);

    void FinalizeSignature(GenericVariantDecl* gen_decl, VariantDefinition* decl, size_t itr);

    void FinalizeBody(GenericVariantDecl* gen_decl, VariantDefinition* decl, size_t itr);

    void FinalizeSignature(GenericImplDecl* gen_decl, ImplDefinition* decl, size_t itr);

    void FinalizeBody(GenericImplDecl* gen_decl, ImplDefinition* decl, size_t itr);

    /**
     * clears the symbols from tables, from previous finalization
     */
    void Clear();

    void FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations);

    void FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    void FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations);

    void FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    void FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations);

    void FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    void FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations);

    void FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    void FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations);

    void FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    void FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations);

    void FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);

    void FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations);


};