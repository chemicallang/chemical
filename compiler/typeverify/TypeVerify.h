// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"
#include <unordered_map>
#include <string>
#include <vector>

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
     * when set, the type checker knows we are in interpretation/comptime mode
     * and can skip argument verification for comptime functions
     * this flag is toggled by contracts on functions like intrinsics::is_interpretation(),
     * intrinsics::is_comptime(), or intrinsics::is_runtime() (with inverted enable_value)
     */
    bool is_interpretation_mode = false;

    // -------- Definite Assignment --------

    /// Every local variable declaration encountered in the current function.
    std::vector<VarInitStatement*> locals;

    /// Parallel boolean array: initialized[i] == true iff locals[i] is definitely initialized.
    std::vector<bool> init_bits;

    /// Per-block stack of declaration counts for scope cleanup.
    std::vector<std::vector<VarInitStatement*>> scope_stack;

    /// True while inside an `unsafe { }` block (suppresses DA checks).
    bool da_in_unsafe = false;

    /// When false, definite-assignment checks are suppressed (e.g. inside nested functions).
    bool da_enabled = false;

    /// While true, the next identifier visit belongs to the inner of an
    /// address/reference-of and must not be reported as a standalone read.
    bool da_addr_inner = false;

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

    // -------- Definite Assignment helpers --------

    void da_push_scope();
    void da_pop_scope();
    void da_add_local(VarInitStatement* v);
    bool da_is_initialized(VarInitStatement* v);
    VarInitStatement* da_root_local_var(Value* v);
    bool da_type_has_destructor(VarInitStatement* v);
    void da_report_uninit(VarInitStatement* v, const char* action, SourceLocation loc);
    void da_intersect(const std::vector<bool>& a, const std::vector<bool>& b,
                      std::vector<bool>& out);

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

    void VisitWhileLoopStmt(WhileLoop* loop);

    void VisitDoWhileLoopStmt(DoWhileLoop* loop);

    void VisitForLoopStmt(ForLoop* loop);

    void VisitSwitchStmt(SwitchStatement* stmt);

    void VisitScope(Scope* scope);

    void VisitBlockScope(BlockScope* scope);

    // Types

    void VisitLinkedType(LinkedType* type);

    // Values

    void VisitStructValue(StructValue *val);

    void VisitArrayValue(ArrayValue *val);

    void VisitFunctionCall(FunctionCall *call);

    void VisitUnsafeBlock(UnsafeBlock* block);

    void VisitVariableIdentifier(VariableIdentifier* id);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitReferenceOfValue(ReferenceOfValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitPlacementNewValue(PlacementNewValue *value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitDereferenceValue(DereferenceValue* value);

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