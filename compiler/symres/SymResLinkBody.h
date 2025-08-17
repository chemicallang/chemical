// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymResLinkBody : public NonRecursiveVisitor<SymResLinkBody> {
public:

    SymbolResolver& linker;

    /**
     * expected type used by values to coerce
     */
    BaseType* expected_type = nullptr;

    /**
     * this is set before visiting type
     */
    SourceLocation type_location = 0;

    /**
     * constructor
     */
    SymResLinkBody(SymbolResolver& linker) : linker(linker) {

    }

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container) {
        linker.scope_start();
        LinkMembersContainerNoScope(container);
        linker.scope_end();
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
        expected_type = nullptr;
        VisitValueNoNullCheck(value);
    }
    inline void visit(Value* value, BaseType* exp_type) {
        expected_type = exp_type;
        VisitValueNoNullCheck(value);
    }
    inline void visit(ChainValue* value) {
        VisitValueNoNullCheck(value);
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

    // Visitor Methods

    void VisitAssignmentStmt(AssignStatement *assign);

    void VisitUsingStmt(UsingStmt* node);

    void VisitBreakStmt(BreakStatement* node);

    void VisitDeleteStmt(DestructStmt* node);

    void VisitDeallocStmt(DeallocStmt* node);

    void VisitProvideStmt(ProvideStmt* node);

    void VisitReturnStmt(ReturnStatement* node);

    void VisitSwitchStmt(SwitchStatement *stmt);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitComptimeBlock(ComptimeBlock* node);

    void VisitDoWhileLoopStmt(DoWhileLoop* node);

    void VisitEnumMember(EnumMember* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitForLoopStmt(ForLoop* node);

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

    void VisitLoopBlock(LoopBlock* node);

    void VisitInitBlock(InitBlock* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitUnsafeBlock(UnsafeBlock* node);

    void VisitVariantCaseVariable(VariantCaseVariable* node);

    void VisitWhileLoopStmt(WhileLoop* node);

    void VisitValueNode(ValueNode* node);

    void VisitMultiFunctionNode(MultiFunctionNode* node);

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

    void VisitFunctionCall(FunctionCall* value);

    void VisitNumberValue(NumberValue* value);

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

    void VisitAddrOfValue(AddrOfValue* value);

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

    void VisitTypeInsideValue(TypeInsideValue* value);

    void VisitNewValue(NewValue* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitNotValue(NotValue* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitSizeOfValue(SizeOfValue* value);

    void VisitAlignOfValue(AlignOfValue* value);

    void VisitStringValue(StringValue* value);

    void VisitStructValue(StructValue* value);

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        // by default access is checked
        VisitVariableIdentifier(value, true);
    }

};