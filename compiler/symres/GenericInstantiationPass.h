// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include "SymbolResolver.h"

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
     * the diagnoser used to collect diagnostics
     */
    ASTDiagnoser diagnoser;

    /**
     * a generic instantiator allows us to own
     */
    GenericInstantiatorAPI generic_instantiator;

    /**
     * tracks the current type's source location
     */
    SourceLocation type_location;

    /**
     * constructor
     */
    explicit GenericInstantiationPass(SymbolResolver& resolver) : linker(resolver), diagnoser(resolver.loc_man),
        type_location(0), generic_instantiator(
        resolver.controller, resolver.binder, resolver.child_resolver,
        resolver.instContainer, resolver.coreNodes, resolver.implsIndex, resolver.generic_inst_reg_mutex,
        *resolver.ast_allocator, diagnoser, resolver.comptime_scope.typeBuilder, resolver.comptime_scope.target_data
    ) {

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

    void VisitStructValue(StructValue *val);

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
    inline void VisitGenericTypeDecl(GenericTypeDecl* node) {
        // do nothing, we must not visit generic decl or its instantiations
    }

    // visit type signatures but skip bodies
    void VisitFunctionDecl(FunctionDeclaration* node);

};