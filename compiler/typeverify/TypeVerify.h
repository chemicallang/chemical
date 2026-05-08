// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class TypeVerifier : public RecursiveVisitor<TypeVerifier> {
public:

    /**
     * implementations index allows us to verify impl declarations
     */
    ImplementationsIndex& index;

    /**
     * the allocator which allows to allocate memory for all instantiations
     */
    ASTAllocator& allocator;

    /**
     * the diagnoser to report errors
     */
    ASTDiagnoser& diagnoser;

    /**
     * is used to verify return statement values
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    TypeVerifier(
        ImplementationsIndex& index,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser
    ) : index(index), allocator(allocator), diagnoser(diagnoser) {

    }

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitStructValue(StructValue *val);

    void VisitArrayValue(ArrayValue *val);

    void VisitFunctionCall(FunctionCall *call);

    void VisitImplDecl(ImplDefinition* def);

    void VisitIfStmt(IfStatement *stmt);

    void VisitReturnStmt(ReturnStatement *stmt);

    void VisitFunctionDecl(FunctionDeclaration *decl);

    // Values

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitPlacementNewValue(PlacementNewValue *value);

    // -------- Generic Declarations ------------
    // -------- Only Template is visited --------

    void VisitGenericFuncDecl(GenericFuncDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericTypeDecl(GenericTypeDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericStructDecl(GenericStructDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericUnionDecl(GenericUnionDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericVariantDecl(GenericVariantDecl* node) {
        visit(node->master_impl);
    }

    void VisitGenericImplDecl(GenericImplDecl* node) {
        visit(node->master_impl);
    }


};