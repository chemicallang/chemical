// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <unordered_set>

#include "preprocess/visitors/RecursiveVisitor.h"
#include <unordered_map>
#include <string>
#include <vector>

class CoreNodes;

class TypeVerifier : public RecursiveVisitor<TypeVerifier> {
public:

    /**
     * implementations index allows us to verify impl declarations
     */
    ImplementationsIndex& index;

    /**
     * job-global core interfaces (Copy, Clone, Default, ...)
     */
    CoreNodes& coreNodes;

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

    /**
     * Lifetime parameters in scope while checking a declaration. `'static` and
     * `'self` are always accepted; other names must be declared.
     */
    std::vector<chem::string_view> lifetime_scope;

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

    /**
     * constructor
     * the allocator must be an ast allocator
     */
    TypeVerifier(
        ImplementationsIndex& index,
        CoreNodes& coreNodes,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser
    ) : index(index), coreNodes(coreNodes), allocator(allocator), diagnoser(diagnoser) {

    }

    // -------- Module-scope initialization check --------

    VarInitStatement* da_root_module_var(Value* v);
    bool da_type_has_destructor(VarInitStatement* v);
    void da_report_uninit(VarInitStatement* v, const char* action, SourceLocation loc);

    // ------------- Decls ------------

    void VisitStructDecl(StructDefinition* def);

    // Reject moving a destructor-bearing element out of an array by index: the
    // array still runs the element destructor, so the moved copy would be freed
    // twice. Copy elements and field reads remain allowed.
    void check_index_move(Value* value);

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
    void VisitFunctionType(FunctionType* type);
    void VisitReferenceType(ReferenceType* type);

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
    void VisitZeroedValue(ZeroedValue* value);

    void VisitAwaitExpression(AwaitExpression* value);

    void VisitLambdaFunction(LambdaFunction *func);

    void VisitPlacementNewValue(PlacementNewValue *value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);


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