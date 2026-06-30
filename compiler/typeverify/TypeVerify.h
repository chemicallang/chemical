// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include <unordered_map>
#include <string>

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
     * using this we check, if non-retained generics are being instantiated in public generic declarations
     */
    bool is_generic_public_context = false;

    /**
     * using this we check, if a public comptime function is calling a non-retained function
     */
    bool is_public_comptime_context = false;

    /**
     * is lifetime check enabled
     */
    bool is_no_lifetime_check = false;

    /**
     * is inside an unsafe block
     */
    bool is_unsafe = false;

    /**
     * when set, IndexOperator destructible check is skipped
     * used when IndexOperator is on LHS of assignment or wrapped in AddrOfValue/ReferenceOfValue
     */
    bool disable_index_destructible_check = false;

    /**
     * when set, the type checker knows we are in interpretation mode
     * and can skip argument verification for comptime functions
     * this flag is toggled by contracts on functions like intrinsics::is_interpretation()
     */
    bool is_interpretation_mode = false;

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

    // ------------- Decls ------------

    void VisitStructDecl(StructDefinition* def);

    void VisitUnionDecl(UnionDef* def);

    void VisitVariantDecl(VariantDefinition* def);

    void VisitInterfaceDecl(InterfaceDefinition* interface);

    // ------------ Statements -------------

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitImplDecl(ImplDefinition* def);

    void VisitIfStmt(IfStatement *stmt);

    void VisitReturnStmt(ReturnStatement *stmt);

    void VisitFunctionDecl(FunctionDeclaration *decl);

    void VisitDeleteStmt(DestructStmt* stmt);

    // Types

    void VisitLinkedType(LinkedType* type);

    // Values

    void VisitStructValue(StructValue *val);

    void VisitArrayValue(ArrayValue *val);

    void VisitFunctionCall(FunctionCall *call);

    void VisitUnsafeBlock(UnsafeBlock* block);

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitPlacementNewValue(PlacementNewValue *value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitReferenceOfValue(ReferenceOfValue* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitRuntimeValue(RuntimeValue* value);

    // -------- Generic Declarations ------------
    // -------- Only Template is visited --------

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);


};