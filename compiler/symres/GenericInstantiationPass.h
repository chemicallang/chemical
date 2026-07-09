// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymbolResolver;

/**
 * This pass runs after the link signature pass.
 * It visits only generic type declarations and finalizes their instantiations.
 * It does NOT visit function bodies, just like TopLevelLinkSignature.
 */
class GenericInstantiationPass : public RecursiveVisitor<GenericInstantiationPass> {
public:

    SymbolResolver& linker;

    /**
     * tracks the current type's source location
     */
    SourceLocation type_location;

    /**
     * constructor
     */
    explicit GenericInstantiationPass(SymbolResolver& linker) : linker(linker), type_location(0) {

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
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.getLocation();
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }

    // traversal into container nodes
    void VisitScope(Scope* node);
    void VisitNamespaceDecl(Namespace* node);
    void VisitIfStmt(IfStatement* node);

    // generic type resolution (instantiation)
    void VisitGenericType(GenericType* type);

    // generic instantiation finalization
    void VisitGenericTypeDecl(GenericTypeDecl* node);

    inline void VisitGenericFuncDecl(GenericFuncDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }
    inline void VisitGenericStructDecl(GenericStructDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }
    inline void VisitGenericUnionDecl(GenericUnionDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }
    inline void VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }
    inline void VisitGenericVariantDecl(GenericVariantDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }
    inline void VisitGenericImplDecl(GenericImplDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }

    // visit type signatures but skip bodies
    void VisitFunctionDecl(FunctionDeclaration* node);
    void VisitVarInitStmt(VarInitStatement* node);

};

/**
 * run the generic instantiation pass on a scope
 */
void sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope);
