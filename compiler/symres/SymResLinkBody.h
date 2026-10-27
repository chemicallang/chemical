// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/NonRecursiveVisitor.h"
#include "SymbolTable.h"
#include "compiler/generics/GenInstantiatorAPI.h"

/**
 * when a variable is outside evaluation scope, for example an identifier lambda referencing a variable above lambda
 * this determines what behavior to used
 */
enum class OutScopeResBehavior : uint8_t {
    Error,
};

class SymResLinkBody : public NonRecursiveVisitor<SymResLinkBody> {
public:

    SymbolResolver& linker;

    /**
     * own diagnoser — diagnostics collected here instead of on the shared resolver
     */
    ASTDiagnoser diagnoser;

    /**
     * per-file local symbol table — holds file-private symbols
     */
    SymbolTable table;

    /**
     * own generic instantiator
     */
    GenericInstantiatorAPI generic_instantiator;

    /**
     * expected type used by values to coerce
     */
    BaseType* expected_type = nullptr;

    /**
     * this is set before visiting type
     */
    SourceLocation type_location = 0;

    /**
     * per-file copies of shared resolver state — avoids mutating the shared resolver
     */
    FunctionTypeBody* current_func_type = nullptr;

    /**
     * safe context, when user writes unsafe block, we turn to false inside the block
     */
    bool safe_context = true;

    /**
     * similar to unsafe, comptime blocks toggle this
     */
    bool comptime_context = false;

    /**
     * generic context, inside generics we toggle this flag
     */
    bool generic_context = false;

    /**
     * the start index of the current lambda scope, only set when in_lambda_scope is true
     * this can be used to verify symbols are inside a lambda scope
     */
    unsigned long lambda_scope_start = 0;

    /**
     * turned on when symbol resolving a lambda body
     */
    bool in_lambda_scope = false;

    /**
     * resolution behavior for variables outside evaluation scope
     */
    OutScopeResBehavior out_scope_res_behavior = OutScopeResBehavior::Error;

    /**
     * constructor
     */
    SymResLinkBody(SymbolResolver& resolver) : linker(resolver),
        diagnoser(resolver.loc_man),
        generic_instantiator(
            resolver.controller, resolver.binder, resolver.child_resolver,
            resolver.instContainer, resolver.coreNodes, resolver.implsIndex,
            resolver.generic_inst_reg_mutex,
            *resolver.ast_allocator, diagnoser,
            resolver.comptime_scope.typeBuilder, resolver.comptime_scope.target_data
        )
    {

    }

    // -------------- const accessors for shared state ----------------

    inline const TypeBuilder& getTypeBuilder() const {
        return linker.comptime_scope.typeBuilder;
    }

    inline const CoreNodes& getCoreNodes() const {
        return linker.coreNodes;
    }

    inline const ImplementationsIndex& getImplsIndex() const {
        return linker.implsIndex;
    }

    inline const TargetData& getTargetData() const {
        return linker.comptime_scope.target_data;
    }

    inline const ChildResolver* getChildResolver() const {
        return &linker.child_resolver;
    }

    inline ASTAllocator& getAstAllocator() {
        return *linker.ast_allocator;
    }

    inline ASTAllocator& getModAllocator() {
        return *linker.mod_allocator;
    }

    inline ASTAllocator& getAllocator() {
        return linker.allocator;
    }

    inline SymbolResolver& getResolver() {
        return linker;
    }

    inline const CompilerBinder& getCompilerBinder() const {
        return linker.binder;
    }

    inline UnresolvedDecl* get_unresolved_decl() {
        return linker.get_unresolved_decl();
    }

    inline GlobalInterpretScope& getComptimeScope() {
        return linker.comptime_scope;
    }

    inline ASTNode* tld_find(const chem::string_view& name) {
        auto* result = table.resolve(name);
        if(result) return result;
        return linker.find(name);
    }

    inline const BucketSymbol* tld_resolve_bucket(const chem::string_view& name) {
        auto* result = table.resolve_bucket(name);
        if(result) return result;
        return linker.getSymbolTable().resolve_bucket(name);
    }

    void declare_no_shadow(const chem::string_view& name, ASTNode* node);

    void declare_local_var(const chem::string_view &name, ASTNode *node);

    BaseType* getErroredType();

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container) {
        table.scope_start();
        LinkMembersContainerNoScope(container);
        table.scope_end();
    }

    template<typename T>
    inline void visit(T* ptr) {
        VisitByPtrTypeNoNullCheck(ptr);
    }
    inline void visit(ASTNode* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(BaseDefMember* node) {
        VisitNodeNoNullCheck((ASTNode*) node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(Value* value, BaseType* exp_type) {
        const auto prev = expected_type;
        expected_type = exp_type;
        VisitValueNoNullCheck(value);
        expected_type = prev;
    }
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(BaseType* type, SourceLocation location) {
        type_location = location;
        VisitTypeNoNullCheck(type);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.encoded_location();
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    // Special Visitor Methods

    void VisitAccessChain(AccessChain* chain, bool check_validity, bool assignment);

    void VisitVariableIdentifier(VariableIdentifier* identifier, bool check_access);

    /**
     * handles a bare generic function reference (`ident<int>`) used as a value.
     * the wrapped identifier is expected to be linked already. inside generic
     * bodies the instantiation is deferred to the generic instantiator, otherwise
     * the generic function is instantiated here and the identifier is relinked to
     * the concrete FunctionDeclaration.
     */
    void link_generic_func_reference(GenericInstIdentifier* ref);

    // Visitor Methods

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitUsingStmt(UsingStmt* node);

    void VisitExportStmt(ExportStmt* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitDeallocStmt(DeallocStmt* node);

    void VisitProvideStmt(ProvideStmt* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement *stmt);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitInlineAsmStmt(InlineAsmStatement* node);

    void VisitComptimeBlock(ComptimeBlock* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitEnumMember(EnumMember* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitForLoopStmt(ForLoop* node);

    void VisitForInLoopStmt(ForInLoop* node);

    void VisitFunctionParam(FunctionParam* node);

    void VisitGenericTypeParam(GenericTypeParameter* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitCapturedVariable(CapturedVariable* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitScope(Scope* node);

    void VisitBlockScope(BlockScope* node);

    void VisitLoopBlock(LoopBlock* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitVariantCaseVariable(VariantCaseVariable* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitValueNode(ValueNode* node);

    void VisitValueWrapper(ValueWrapperNode* node);

    void VisitAccessChainNode(AccessChainNode* node);

    void VisitIncDecNode(IncDecNode* node);

    void VisitPatternMatchExprNode(PatternMatchExprNode* node);

    void VisitPlacementNewNode(PlacementNewNode* node);

    void VisitEmbeddedNode(EmbeddedNode* node);

    // ------------------------------------
    // ----------- Values -----------------
    // ------------------------------------

    void VisitAccessChain(AccessChain *chain);

    void VisitGenericInstIdentifier(GenericInstIdentifier* ref);

    void VisitFunctionCall(FunctionCall* value);

    void VisitEmbeddedValue(EmbeddedValue* value);

    void VisitComptimeValue(ComptimeValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitVariantCase(VariantCase* value);

    void VisitArrayType(ArrayType* type);

    void VisitDynamicType(DynamicType* type);

    void VisitFunctionType(FunctionType* type);

    void VisitGenericType(GenericType* type);

    void VisitLinkedType(LinkedType* type);

    void VisitPointerType(PointerType* type);

    void VisitReferenceType(ReferenceType* type);

    void VisitCapturingFunctionType(CapturingFunctionType* type);

    void VisitStructType(StructType* type);

    void VisitUnionType(UnionType* type);

    void VisitIfType(IfType* type);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitReferenceOfValue(ReferenceOfValue* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitCastedValue(CastedValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitExpression(Expression* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitIsValue(IsValue* value);

    void VisitInValue(InValue* value);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitAwaitExpression(AwaitExpression* value);

    void VisitTypeInsideValue(TypeInsideValue* value);

    void VisitNewValue(NewValue* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitNotValue(NotValue* value);

    void VisitBitwiseNot(BitwiseNot* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitSizeOfValue(SizeOfValue* value);

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitOffsetOfValue(OffsetOfValue* value);

    void VisitIfValue(IfValue* value);

    void VisitSwitchValue(SwitchValue* value);

    void VisitLoopValue(LoopValue* value);

    void VisitStringValue(StringValue* value);

    void VisitStructValue(StructValue* value);

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        // by default access is checked
        VisitVariableIdentifier(value, true);
    }

    void VisitExpressiveString(ExpressiveString* value);

    void VisitDynamicValue(DynamicValue* value);

    void VisitZeroedValue(ZeroedValue* value);

    // Movement API

    };