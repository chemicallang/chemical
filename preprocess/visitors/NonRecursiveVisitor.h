// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/TypeLoc.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/base/ast_fwd.h"

template<typename Derived>
class NonRecursiveVisitor {
public:

    // ------- Common Functions -----

    // overriding this would allow you handle all nodes
    inline void VisitCommonNode(ASTNode* node) {
        // does nothing by default
    }

    // overriding this will allow you handle all nodes
    inline void VisitCommonValue(Value* value) {
        // does nothing by default
    }

    // overriding this will allow you handle every type
    inline void VisitCommonType(BaseType* type) {
        // does nothing by default
    }

    // ------- NODES --------

    inline void VisitAssignmentStmt(AssignStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitBreakStmt(BreakStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitContinueStmt(ContinueStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUnreachableStmt(UnreachableStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitDeleteStmt(DestructStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitDeallocStmt(DeallocStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitImportStmt(ImportStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitReturnStmt(ReturnStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitAliasStmt(AliasStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitSwitchStmt(SwitchStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitThrowStmt(ThrowStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitTypealiasStmt(TypealiasStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUsingStmt(UsingStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitVarInitStmt(VarInitStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitLoopBlock(LoopBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitProvideStmt(ProvideStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitComptimeBlock(ComptimeBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitWhileLoopStmt(WhileLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitDoWhileLoopStmt(DoWhileLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitForLoopStmt(ForLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitIfStmt(IfStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitTryStmt(TryCatch* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitValueNode(ValueNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitValueWrapper(ValueWrapperNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitAccessChainNode(AccessChainNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitIncDecNode(IncDecNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitPatternMatchExprNode(PatternMatchExprNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitPlacementNewNode(PlacementNewNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitEnumDecl(EnumDeclaration* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitEnumMember(EnumMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitFunctionDecl(FunctionDeclaration* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericTypeDecl(GenericTypeDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericFuncDecl(GenericFuncDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericStructDecl(GenericStructDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericUnionDecl(GenericUnionDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericVariantDecl(GenericVariantDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericImplDecl(GenericImplDecl* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitMultiFunctionNode(MultiFunctionNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitImplDecl(ImplDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitInterfaceDecl(InterfaceDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitInitBlock(InitBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitStructDecl(StructDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitStructMember(StructMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitNamespaceDecl(Namespace* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUnionDecl(UnionDef* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitVariantDecl(VariantDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitVariantMember(VariantMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUnnamedStruct(UnnamedStruct* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUnnamedUnion(UnnamedUnion* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitScope(Scope* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitUnsafeBlock(UnsafeBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitFunctionParam(FunctionParam* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitGenericTypeParam(GenericTypeParameter* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitVariantMemberParam(VariantMemberParam* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitCapturedVariable(CapturedVariable* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitVariantCaseVariable(VariantCaseVariable* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    inline void VisitEmbeddedNode(EmbeddedNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }

    // ---------- Values ----------

    inline void VisitCharValue(CharValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitShortValue(ShortValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitIntValue(IntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitLongValue(LongValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitBigIntValue(BigIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitInt128Value(Int128Value* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUCharValue(UCharValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUShortValue(UShortValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUIntValue(UIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitULongValue(ULongValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUBigIntValue(UBigIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUInt128Value(UInt128Value* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNumberValue(NumberValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitFloatValue(FloatValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitDoubleValue(DoubleValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitBoolValue(BoolValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitStringValue(StringValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitExpression(Expression* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitArrayValue(ArrayValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitStructValue(StructValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitLambdaFunction(LambdaFunction* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNewTypedValue(NewTypedValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNewValue(NewValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitPlacementNewValue(PlacementNewValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitIfValue(IfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitSwitchValue(SwitchValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitLoopValue(LoopValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitIncDecValue(IncDecValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitIsValue(IsValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitInValue(InValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitDereferenceValue(DereferenceValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitRetStructParamValue(RetStructParamValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitAccessChain(AccessChain* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitCastedValue(CastedValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitVariableIdentifier(VariableIdentifier* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitIndexOperator(IndexOperator* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitFunctionCall(FunctionCall* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNegativeValue(NegativeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNotValue(NotValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitNullValue(NullValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitSizeOfValue(SizeOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitUnsafeValue(UnsafeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitComptimeValue(ComptimeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitAlignOfValue(AlignOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitVariantCase(VariantCase* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitAddrOfValue(AddrOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitPointerValue(PointerValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitBlockValue(BlockValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitTypeInsideValue(TypeInsideValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitPatternMatchExpr(PatternMatchExpr* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitWrapValue(WrapValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitDestructValue(DestructValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitExtractionValue(ExtractionValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    inline void VisitEmbeddedValue(EmbeddedValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue((Value*) value);
    }

    // Types begin here

    inline void VisitAnyType(AnyType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitArrayType(ArrayType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitStructType(StructType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitUnionType(UnionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitBoolType(BoolType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitDoubleType(DoubleType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitFloatType(FloatType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitLongDoubleType(LongDoubleType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitComplexType(ComplexType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitFloat128Type(Float128Type* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitFunctionType(FunctionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitGenericType(GenericType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitIntNType(IntNType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitPointerType(PointerType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitReferenceType(ReferenceType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitLinkedType(LinkedType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitStringType(StringType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitLiteralType(LiteralType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitDynamicType(DynamicType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitVoidType(VoidType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitExpressionType(ExpressionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitNullPtrType(NullPtrType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    inline void VisitCapturingFunctionType(CapturingFunctionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitNodeNoNullCheck(ASTNode* node) {
        switch(node->kind()) {
            case ASTNodeKind::AssignmentStmt:
                static_cast<Derived*>(this)->VisitAssignmentStmt((AssignStatement*) node);
                return;
            case ASTNodeKind::BreakStmt:
                static_cast<Derived*>(this)->VisitBreakStmt((BreakStatement*) node);
                return;
            case ASTNodeKind::ContinueStmt:
                static_cast<Derived*>(this)->VisitContinueStmt((ContinueStatement*) node);
                return;
            case ASTNodeKind::UnreachableStmt:
                static_cast<Derived*>(this)->VisitUnreachableStmt((UnreachableStmt*) node);
                return;
            case ASTNodeKind::DeleteStmt:
                static_cast<Derived*>(this)->VisitDeleteStmt((DestructStmt*) node);
                return;
            case ASTNodeKind::DeallocStmt:
                static_cast<Derived*>(this)->VisitDeallocStmt((DeallocStmt*) node);
                return;
            case ASTNodeKind::ImportStmt:
                static_cast<Derived*>(this)->VisitImportStmt((ImportStatement*) node);
                return;
            case ASTNodeKind::ReturnStmt:
                static_cast<Derived*>(this)->VisitReturnStmt((ReturnStatement*) node);
                return;
            case ASTNodeKind::AliasStmt:
                static_cast<Derived*>(this)->VisitAliasStmt((AliasStmt*) node);
                return;
            case ASTNodeKind::SwitchStmt:
                static_cast<Derived*>(this)->VisitSwitchStmt((SwitchStatement*) node);
                return;
            case ASTNodeKind::ThrowStmt:
                static_cast<Derived*>(this)->VisitThrowStmt((ThrowStatement*) node);
                return;
            case ASTNodeKind::TypealiasStmt:
                static_cast<Derived*>(this)->VisitTypealiasStmt((TypealiasStatement*) node);
                return;
            case ASTNodeKind::UsingStmt:
                static_cast<Derived*>(this)->VisitUsingStmt((UsingStmt*) node);
                return;
            case ASTNodeKind::VarInitStmt:
                static_cast<Derived*>(this)->VisitVarInitStmt((VarInitStatement*) node);
                return;
            case ASTNodeKind::LoopBlock:
                static_cast<Derived*>(this)->VisitLoopBlock((LoopBlock*) node);
                return;
            case ASTNodeKind::ProvideStmt:
                static_cast<Derived*>(this)->VisitProvideStmt((ProvideStmt*) node);
                return;
            case ASTNodeKind::ComptimeBlock:
                static_cast<Derived*>(this)->VisitComptimeBlock((ComptimeBlock*) node);
                return;
            case ASTNodeKind::WhileLoopStmt:
                static_cast<Derived*>(this)->VisitWhileLoopStmt((WhileLoop*) node);
                return;
            case ASTNodeKind::DoWhileLoopStmt:
                static_cast<Derived*>(this)->VisitDoWhileLoopStmt((DoWhileLoop*) node);
                return;
            case ASTNodeKind::ForLoopStmt:
                static_cast<Derived*>(this)->VisitForLoopStmt((ForLoop*) node);
                return;
            case ASTNodeKind::IfStmt:
                static_cast<Derived*>(this)->VisitIfStmt((IfStatement*) node);
                return;
            case ASTNodeKind::TryStmt:
                static_cast<Derived*>(this)->VisitTryStmt((TryCatch*) node);
                return;
            case ASTNodeKind::ValueNode:
                static_cast<Derived*>(this)->VisitValueNode((ValueNode*) node);
                return;
            case ASTNodeKind::ValueWrapper:
                static_cast<Derived*>(this)->VisitValueWrapper((ValueWrapperNode*) node);
                return;
            case ASTNodeKind::AccessChainNode:
                static_cast<Derived*>(this)->VisitAccessChainNode((AccessChainNode*) node);
                return;
            case ASTNodeKind::IncDecNode:
                static_cast<Derived*>(this)->VisitIncDecNode((IncDecNode*) node);
                return;
            case ASTNodeKind::PatternMatchExprNode:
                static_cast<Derived*>(this)->VisitPatternMatchExprNode((PatternMatchExprNode*) node);
                return;
            case ASTNodeKind::PlacementNewNode:
                static_cast<Derived*>(this)->VisitPlacementNewNode((PlacementNewNode*) node);
                return;
            case ASTNodeKind::EnumDecl:
                static_cast<Derived*>(this)->VisitEnumDecl((EnumDeclaration*) node);
                return;
            case ASTNodeKind::EnumMember:
                static_cast<Derived*>(this)->VisitEnumMember((EnumMember*) node);
                return;
            case ASTNodeKind::FunctionDecl:
                static_cast<Derived*>(this)->VisitFunctionDecl((FunctionDeclaration*) node);
                return;
            case ASTNodeKind::GenericTypeDecl:
                static_cast<Derived*>(this)->VisitGenericTypeDecl((GenericTypeDecl*) node);
                return;
            case ASTNodeKind::GenericFuncDecl:
                static_cast<Derived*>(this)->VisitGenericFuncDecl((GenericFuncDecl*) node);
                return;
            case ASTNodeKind::GenericStructDecl:
                static_cast<Derived*>(this)->VisitGenericStructDecl((GenericStructDecl*) node);
                return;
            case ASTNodeKind::GenericUnionDecl:
                static_cast<Derived*>(this)->VisitGenericUnionDecl((GenericUnionDecl*) node);
                return;
            case ASTNodeKind::GenericInterfaceDecl:
                static_cast<Derived*>(this)->VisitGenericInterfaceDecl((GenericInterfaceDecl*) node);
                return;
            case ASTNodeKind::GenericVariantDecl:
                static_cast<Derived*>(this)->VisitGenericVariantDecl((GenericVariantDecl*) node);
                return;
            case ASTNodeKind::GenericImplDecl:
                static_cast<Derived*>(this)->VisitGenericImplDecl((GenericImplDecl*) node);
                return;
            case ASTNodeKind::MultiFunctionNode:
                static_cast<Derived*>(this)->VisitMultiFunctionNode((MultiFunctionNode*) node);
                return;
            case ASTNodeKind::ImplDecl:
                static_cast<Derived*>(this)->VisitImplDecl((ImplDefinition*) node);
                return;
            case ASTNodeKind::InterfaceDecl:
                static_cast<Derived*>(this)->VisitInterfaceDecl((InterfaceDefinition*) node);
                return;
            case ASTNodeKind::InitBlock:
                static_cast<Derived*>(this)->VisitInitBlock((InitBlock*) node);
                return;
            case ASTNodeKind::StructDecl:
                static_cast<Derived*>(this)->VisitStructDecl((StructDefinition*) node);
                return;
            case ASTNodeKind::StructMember:
                static_cast<Derived*>(this)->VisitStructMember((StructMember*) node);
                return;
            case ASTNodeKind::NamespaceDecl:
                static_cast<Derived*>(this)->VisitNamespaceDecl((Namespace*) node);
                return;
            case ASTNodeKind::UnionDecl:
                static_cast<Derived*>(this)->VisitUnionDecl((UnionDef*) node);
                return;
            case ASTNodeKind::VariantDecl:
                static_cast<Derived*>(this)->VisitVariantDecl((VariantDefinition*) node);
                return;
            case ASTNodeKind::VariantMember:
                static_cast<Derived*>(this)->VisitVariantMember((VariantMember*) node);
                return;
            case ASTNodeKind::UnnamedStruct:
                static_cast<Derived*>(this)->VisitUnnamedStruct((UnnamedStruct*) node);
                return;
            case ASTNodeKind::UnnamedUnion:
                static_cast<Derived*>(this)->VisitUnnamedUnion((UnnamedUnion*) node);
                return;
            case ASTNodeKind::Scope:
                static_cast<Derived*>(this)->VisitScope((Scope*) node);
                return;
            case ASTNodeKind::UnsafeBlock:
                static_cast<Derived*>(this)->VisitUnsafeBlock((UnsafeBlock*) node);
                return;
            case ASTNodeKind::FunctionParam:
                static_cast<Derived*>(this)->VisitFunctionParam((FunctionParam*) node);
                return;
            case ASTNodeKind::GenericTypeParam:
                static_cast<Derived*>(this)->VisitGenericTypeParam((GenericTypeParameter*) node);
                return;
            case ASTNodeKind::VariantMemberParam:
                static_cast<Derived*>(this)->VisitVariantMemberParam((VariantMemberParam*) node);
                return;
            case ASTNodeKind::CapturedVariable:
                static_cast<Derived*>(this)->VisitCapturedVariable((CapturedVariable*) node);
                return;
            case ASTNodeKind::VariantCaseVariable:
                static_cast<Derived*>(this)->VisitVariantCaseVariable((VariantCaseVariable*) node);
                return;
            case ASTNodeKind::StructType:
                static_cast<Derived*>(this)->VisitStructType((StructType*) node);
                return;
            case ASTNodeKind::UnionType:
                static_cast<Derived*>(this)->VisitUnionType((UnionType*) node);
                return;
            case ASTNodeKind::EmbeddedNode:
                static_cast<Derived*>(this)->VisitEmbeddedNode((EmbeddedNode*) node);
                return;
#ifdef DEBUG
            default:
                throw "UNHANDLED: node kind in non recursive visitor";
#endif
        }
    }

    inline void VisitNode(ASTNode* node) {
        if(node) VisitNodeNoNullCheck(node);
    }

    void VisitValueNoNullCheck(Value* value) {
        switch(value->kind()) {
            case ValueKind::Char:
                static_cast<Derived*>(this)->VisitCharValue((CharValue*) value);
                return;
            case ValueKind::Short:
                static_cast<Derived*>(this)->VisitShortValue((ShortValue*) value);
                return;
            case ValueKind::Int:
                static_cast<Derived*>(this)->VisitIntValue((IntValue*) value);
                return;
            case ValueKind::Long:
                static_cast<Derived*>(this)->VisitLongValue((LongValue*) value);
                return;
            case ValueKind::BigInt:
                static_cast<Derived*>(this)->VisitBigIntValue((BigIntValue*) value);
                return;
            case ValueKind::Int128:
                static_cast<Derived*>(this)->VisitInt128Value((Int128Value*) value);
                return;
            case ValueKind::UChar:
                static_cast<Derived*>(this)->VisitUCharValue((UCharValue*) value);
                return;
            case ValueKind::UShort:
                static_cast<Derived*>(this)->VisitUShortValue((UShortValue*) value);
                return;
            case ValueKind::UInt:
                static_cast<Derived*>(this)->VisitUIntValue((UIntValue*) value);
                return;
            case ValueKind::ULong:
                static_cast<Derived*>(this)->VisitULongValue((ULongValue*) value);
                return;
            case ValueKind::UBigInt:
                static_cast<Derived*>(this)->VisitUBigIntValue((UBigIntValue*) value);
                return;
            case ValueKind::UInt128:
                static_cast<Derived*>(this)->VisitUInt128Value((UInt128Value*) value);
                return;
            case ValueKind::NumberValue:
                static_cast<Derived*>(this)->VisitNumberValue((NumberValue*) value);
                return;
            case ValueKind::Float:
                static_cast<Derived*>(this)->VisitFloatValue((FloatValue*) value);
                return;
            case ValueKind::Double:
                static_cast<Derived*>(this)->VisitDoubleValue((DoubleValue*) value);
                return;
            case ValueKind::Bool:
                static_cast<Derived*>(this)->VisitBoolValue((BoolValue*) value);
                return;
            case ValueKind::String:
                static_cast<Derived*>(this)->VisitStringValue((StringValue*) value);
                return;
            case ValueKind::Expression:
                static_cast<Derived*>(this)->VisitExpression((Expression*) value);
                return;
            case ValueKind::ArrayValue:
                static_cast<Derived*>(this)->VisitArrayValue((ArrayValue*) value);
                return;
            case ValueKind::StructValue:
                static_cast<Derived*>(this)->VisitStructValue((StructValue*) value);
                return;
            case ValueKind::LambdaFunc:
                static_cast<Derived*>(this)->VisitLambdaFunction((LambdaFunction*) value);
                return;
            case ValueKind::IfValue:
                static_cast<Derived*>(this)->VisitIfValue((IfValue*) value);
                return;
            case ValueKind::SwitchValue:
                static_cast<Derived*>(this)->VisitSwitchValue((SwitchValue*) value);
                return;
            case ValueKind::LoopValue:
                static_cast<Derived*>(this)->VisitLoopValue((LoopValue*) value);
                return;
            case ValueKind::NewTypedValue:
                static_cast<Derived*>(this)->VisitNewTypedValue((NewTypedValue*) value);
                return;
            case ValueKind::NewValue:
                static_cast<Derived*>(this)->VisitNewValue((NewValue*) value);
                return;
            case ValueKind::PlacementNewValue:
                static_cast<Derived*>(this)->VisitPlacementNewValue((PlacementNewValue*) value);
                return;
            case ValueKind::IncDecValue:
                static_cast<Derived*>(this)->VisitIncDecValue((IncDecValue*) value);
                return;
            case ValueKind::IsValue:
                static_cast<Derived*>(this)->VisitIsValue((IsValue*) value);
                return;
            case ValueKind::InValue:
                static_cast<Derived*>(this)->VisitInValue((InValue*) value);
                return;
            case ValueKind::DereferenceValue:
                static_cast<Derived*>(this)->VisitDereferenceValue((DereferenceValue*) value);
                return;
            case ValueKind::RetStructParamValue:
                static_cast<Derived*>(this)->VisitRetStructParamValue((RetStructParamValue*) value);
                return;
            case ValueKind::AccessChain:
                static_cast<Derived*>(this)->VisitAccessChain((AccessChain*) value);
                return;
            case ValueKind::CastedValue:
                static_cast<Derived*>(this)->VisitCastedValue((CastedValue*) value);
                return;
            case ValueKind::Identifier:
                static_cast<Derived*>(this)->VisitVariableIdentifier((VariableIdentifier*) value);
                return;
            case ValueKind::IndexOperator:
                static_cast<Derived*>(this)->VisitIndexOperator((IndexOperator*) value);
                return;
            case ValueKind::FunctionCall:
                static_cast<Derived*>(this)->VisitFunctionCall((FunctionCall*) value);
                return;
            case ValueKind::NegativeValue:
                static_cast<Derived*>(this)->VisitNegativeValue((NegativeValue*) value);
                return;
            case ValueKind::NotValue:
                static_cast<Derived*>(this)->VisitNotValue((NotValue*) value);
                return;
            case ValueKind::NullValue:
                static_cast<Derived*>(this)->VisitNullValue((NullValue*) value);
                return;
            case ValueKind::SizeOfValue:
                static_cast<Derived*>(this)->VisitSizeOfValue((SizeOfValue*) value);
                return;
            case ValueKind::UnsafeValue:
                static_cast<Derived*>(this)->VisitUnsafeValue((UnsafeValue*) value);
                return;
            case ValueKind::ComptimeValue:
                static_cast<Derived*>(this)->VisitComptimeValue((ComptimeValue*) value);
                return;
            case ValueKind::AlignOfValue:
                static_cast<Derived*>(this)->VisitAlignOfValue((AlignOfValue*) value);
                return;
            case ValueKind::VariantCase:
                static_cast<Derived*>(this)->VisitVariantCase((VariantCase*) value);
                return;
            case ValueKind::AddrOfValue:
                static_cast<Derived*>(this)->VisitAddrOfValue((AddrOfValue*) value);
                return;
            case ValueKind::PointerValue:
                static_cast<Derived*>(this)->VisitPointerValue((PointerValue*) value);
                return;
            case ValueKind::TypeInsideValue:
                static_cast<Derived*>(this)->VisitTypeInsideValue((TypeInsideValue*) value);
                return;
            case ValueKind::PatternMatchExpr:
                static_cast<Derived*>(this)->VisitPatternMatchExpr((PatternMatchExpr*) value);
                return;
            case ValueKind::BlockValue:
                static_cast<Derived*>(this)->VisitBlockValue((BlockValue*) value);
                return;
            case ValueKind::WrapValue:
                static_cast<Derived*>(this)->VisitWrapValue((WrapValue*) value);
                return;
            case ValueKind::DestructValue:
                static_cast<Derived*>(this)->VisitDestructValue((DestructValue*) value);
                return;
            case ValueKind::ExtractionValue:
                static_cast<Derived*>(this)->VisitExtractionValue((ExtractionValue*) value);
                return;
            case ValueKind::EmbeddedValue:
                static_cast<Derived*>(this)->VisitEmbeddedValue((EmbeddedValue*) value);
                return;
#ifdef DEBUG
            default:
                throw "UNHANDLED: value kind in non recursive visitor";
#endif
        }
    }

    inline void VisitValue(Value* value) {
        if(value) VisitValueNoNullCheck(value);
    }

    void VisitTypeNoNullCheck(BaseType* type) {
        switch(type->kind()) {
            case BaseTypeKind::Any:
                static_cast<Derived*>(this)->VisitAnyType((AnyType*) type);
                return;
            case BaseTypeKind::Array:
                static_cast<Derived*>(this)->VisitArrayType((ArrayType*) type);
                return;
            case BaseTypeKind::Struct:
                static_cast<Derived*>(this)->VisitStructType((StructType*) type);
                return;
            case BaseTypeKind::Union:
                static_cast<Derived*>(this)->VisitUnionType((UnionType*) type);
                return;
            case BaseTypeKind::Bool:
                static_cast<Derived*>(this)->VisitBoolType((BoolType*) type);
                return;
            case BaseTypeKind::Double:
                static_cast<Derived*>(this)->VisitDoubleType((DoubleType*) type);
                return;
            case BaseTypeKind::Float:
                static_cast<Derived*>(this)->VisitFloatType((FloatType*) type);
                return;
            case BaseTypeKind::LongDouble:
                static_cast<Derived*>(this)->VisitLongDoubleType((LongDoubleType*) type);
                return;
            case BaseTypeKind::Complex:
                static_cast<Derived*>(this)->VisitComplexType((ComplexType*) type);
                return;
            case BaseTypeKind::Float128:
                static_cast<Derived*>(this)->VisitFloat128Type((Float128Type*) type);
                return;
            case BaseTypeKind::Function:
                static_cast<Derived*>(this)->VisitFunctionType((FunctionType*) type);
                return;
            case BaseTypeKind::Generic:
                static_cast<Derived*>(this)->VisitGenericType((GenericType*) type);
                return;
            case BaseTypeKind::IntN:
                static_cast<Derived*>(this)->VisitIntNType((IntNType*) type);
                return;
            case BaseTypeKind::Pointer:
                static_cast<Derived*>(this)->VisitPointerType((PointerType*) type);
                return;
            case BaseTypeKind::Reference:
                static_cast<Derived*>(this)->VisitReferenceType((ReferenceType*) type);
                return;
            case BaseTypeKind::Linked:
                static_cast<Derived*>(this)->VisitLinkedType((LinkedType*) type);
                return;
            case BaseTypeKind::String:
                static_cast<Derived*>(this)->VisitStringType((StringType*) type);
                return;
            case BaseTypeKind::Literal:
                static_cast<Derived*>(this)->VisitLiteralType((LiteralType*) type);
                return;
            case BaseTypeKind::Dynamic:
                static_cast<Derived*>(this)->VisitDynamicType((DynamicType*) type);
                return;
            case BaseTypeKind::Void:
                static_cast<Derived*>(this)->VisitVoidType((VoidType*) type);
                return;
            case BaseTypeKind::ExpressionType:
                static_cast<Derived*>(this)->VisitExpressionType((ExpressionType*) type);
                return;
            case BaseTypeKind::NullPtr:
                static_cast<Derived*>(this)->VisitNullPtrType((NullPtrType*) type);
                return;
            case BaseTypeKind::CapturingFunction:
                static_cast<Derived*>(this)->VisitCapturingFunctionType((CapturingFunctionType*) type);
                return;
#ifdef DEBUG
            default:
                throw "UNHANDLED: type kind in non recursive visitor";
#endif
        }
    }

    inline void VisitType(BaseType* type) {
        if(type) VisitTypeNoNullCheck(type);
    }

    // Visit By Ptr Type

    inline void VisitByPtrTypeNoNullCheck(AssignStatement* node) {
        static_cast<Derived*>(this)->VisitAssignmentStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(BreakStatement* node) {
        static_cast<Derived*>(this)->VisitBreakStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ContinueStatement* node) {
        static_cast<Derived*>(this)->VisitContinueStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UnreachableStmt* node) {
        static_cast<Derived*>(this)->VisitUnreachableStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(DestructStmt* node) {
        static_cast<Derived*>(this)->VisitDeleteStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(DeallocStmt* node) {
        static_cast<Derived*>(this)->VisitDeallocStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ImportStatement* node) {
        static_cast<Derived*>(this)->VisitImportStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ReturnStatement* node) {
        static_cast<Derived*>(this)->VisitReturnStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(SwitchStatement* node) {
        static_cast<Derived*>(this)->VisitSwitchStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ThrowStatement* node) {
        static_cast<Derived*>(this)->VisitThrowStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(TypealiasStatement* node) {
        static_cast<Derived*>(this)->VisitTypealiasStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UsingStmt* node) {
        static_cast<Derived*>(this)->VisitUsingStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(VarInitStatement* node) {
        static_cast<Derived*>(this)->VisitVarInitStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(LoopBlock* node) {
        static_cast<Derived*>(this)->VisitLoopBlock(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ProvideStmt* node) {
        static_cast<Derived*>(this)->VisitProvideStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ComptimeBlock* node) {
        static_cast<Derived*>(this)->VisitComptimeBlock(node);
    }
    inline void VisitByPtrTypeNoNullCheck(WhileLoop* node) {
        static_cast<Derived*>(this)->VisitWhileLoopStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(DoWhileLoop* node) {
        static_cast<Derived*>(this)->VisitDoWhileLoopStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ForLoop* node) {
        static_cast<Derived*>(this)->VisitForLoopStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(IfStatement* node) {
        static_cast<Derived*>(this)->VisitIfStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(TryCatch* node) {
        static_cast<Derived*>(this)->VisitTryStmt(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ValueNode* node) {
        static_cast<Derived*>(this)->VisitValueNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ValueWrapperNode* node) {
        static_cast<Derived*>(this)->VisitValueWrapper(node);
    }
    inline void VisitByPtrTypeNoNullCheck(AccessChainNode* node) {
        static_cast<Derived*>(this)->VisitAccessChainNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(IncDecNode* node) {
        static_cast<Derived*>(this)->VisitIncDecNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(PatternMatchExprNode* node) {
        static_cast<Derived*>(this)->VisitPatternMatchExprNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(PlacementNewNode* node) {
        static_cast<Derived*>(this)->VisitPlacementNewNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(EnumDeclaration* node) {
        static_cast<Derived*>(this)->VisitEnumDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(EnumMember* node) {
        static_cast<Derived*>(this)->VisitEnumMember(node);
    }
    inline void VisitByPtrTypeNoNullCheck(FunctionDeclaration* node) {
        static_cast<Derived*>(this)->VisitFunctionDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(MultiFunctionNode* node) {
        static_cast<Derived*>(this)->VisitMultiFunctionNode(node);
    }
    inline void VisitByPtrTypeNoNullCheck(ImplDefinition* node) {
        static_cast<Derived*>(this)->VisitImplDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(InterfaceDefinition* node) {
        static_cast<Derived*>(this)->VisitInterfaceDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(InitBlock* node) {
        static_cast<Derived*>(this)->VisitInitBlock(node);
    }
    inline void VisitByPtrTypeNoNullCheck(StructDefinition* node) {
        static_cast<Derived*>(this)->VisitStructDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(StructMember* node) {
        static_cast<Derived*>(this)->VisitStructMember(node);
    }
    inline void VisitByPtrTypeNoNullCheck(Namespace* node) {
        static_cast<Derived*>(this)->VisitNamespaceDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UnionDef* node) {
        static_cast<Derived*>(this)->VisitUnionDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(VariantDefinition* node) {
        static_cast<Derived*>(this)->VisitVariantDecl(node);
    }
    inline void VisitByPtrTypeNoNullCheck(VariantMember* node) {
        static_cast<Derived*>(this)->VisitVariantMember(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UnnamedStruct* node) {
        static_cast<Derived*>(this)->VisitUnnamedStruct(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UnnamedUnion* node) {
        static_cast<Derived*>(this)->VisitUnnamedUnion(node);
    }
    inline void VisitByPtrTypeNoNullCheck(Scope* node) {
        static_cast<Derived*>(this)->VisitScope(node);
    }
    inline void VisitByPtrTypeNoNullCheck(UnsafeBlock* node) {
        static_cast<Derived*>(this)->VisitUnsafeBlock(node);
    }
    inline void VisitByPtrTypeNoNullCheck(FunctionParam* node) {
        static_cast<Derived*>(this)->VisitFunctionParam(node);
    }
    inline void VisitByPtrTypeNoNullCheck(GenericTypeParameter* node) {
        static_cast<Derived*>(this)->VisitGenericTypeParam(node);
    }
    inline void VisitByPtrTypeNoNullCheck(VariantMemberParam* node) {
        static_cast<Derived*>(this)->VisitVariantMemberParam(node);
    }
    inline void VisitByPtrTypeNoNullCheck(CapturedVariable* node) {
        static_cast<Derived*>(this)->VisitCapturedVariable(node);
    }
    inline void VisitByPtrTypeNoNullCheck(VariantCaseVariable* node) {
        static_cast<Derived*>(this)->VisitVariantCaseVariable(node);
    }
    inline void VisitByPtrTypeNoNullCheck(CharValue* value) {
        static_cast<Derived*>(this)->VisitCharValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(ShortValue* value) {
        static_cast<Derived*>(this)->VisitShortValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(IntValue* value) {
        static_cast<Derived*>(this)->VisitIntValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(LongValue* value) {
        static_cast<Derived*>(this)->VisitLongValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(BigIntValue* value) {
        static_cast<Derived*>(this)->VisitBigIntValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(Int128Value* value) {
        static_cast<Derived*>(this)->VisitInt128Value(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UCharValue* value) {
        static_cast<Derived*>(this)->VisitUCharValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UShortValue* value) {
        static_cast<Derived*>(this)->VisitUShortValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UIntValue* value) {
        static_cast<Derived*>(this)->VisitUIntValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(ULongValue* value) {
        static_cast<Derived*>(this)->VisitULongValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UBigIntValue* value) {
        static_cast<Derived*>(this)->VisitUBigIntValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UInt128Value* value) {
        static_cast<Derived*>(this)->VisitUInt128Value(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NumberValue* value) {
        static_cast<Derived*>(this)->VisitNumberValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(FloatValue* value) {
        static_cast<Derived*>(this)->VisitFloatValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(DoubleValue* value) {
        static_cast<Derived*>(this)->VisitDoubleValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(BoolValue* value) {
        static_cast<Derived*>(this)->VisitBoolValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(StringValue* value) {
        static_cast<Derived*>(this)->VisitStringValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(Expression* value) {
        static_cast<Derived*>(this)->VisitExpression(value);
    }
    inline void VisitByPtrTypeNoNullCheck(ArrayValue* value) {
        static_cast<Derived*>(this)->VisitArrayValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(StructValue* value) {
        static_cast<Derived*>(this)->VisitStructValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(LambdaFunction* value) {
        static_cast<Derived*>(this)->VisitLambdaFunction(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NewTypedValue* value) {
        static_cast<Derived*>(this)->VisitNewTypedValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NewValue* value) {
        static_cast<Derived*>(this)->VisitNewValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(PlacementNewValue* value) {
        static_cast<Derived*>(this)->VisitPlacementNewValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(IncDecValue* value) {
        static_cast<Derived*>(this)->VisitIncDecValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(PatternMatchExpr* value) {
        static_cast<Derived*>(this)->VisitPatternMatchExpr(value);
    }
    inline void VisitByPtrTypeNoNullCheck(IsValue* value) {
        static_cast<Derived*>(this)->VisitIsValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(DereferenceValue* value) {
        static_cast<Derived*>(this)->VisitDereferenceValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(RetStructParamValue* value) {
        static_cast<Derived*>(this)->VisitRetStructParamValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(AccessChain* value) {
        static_cast<Derived*>(this)->VisitAccessChain(value);
    }
    inline void VisitByPtrTypeNoNullCheck(CastedValue* value) {
        static_cast<Derived*>(this)->VisitCastedValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(VariableIdentifier* value) {
        static_cast<Derived*>(this)->VisitVariableIdentifier(value);
    }
    inline void VisitByPtrTypeNoNullCheck(IndexOperator* value) {
        static_cast<Derived*>(this)->VisitIndexOperator(value);
    }
    inline void VisitByPtrTypeNoNullCheck(FunctionCall* value) {
        static_cast<Derived*>(this)->VisitFunctionCall(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NegativeValue* value) {
        static_cast<Derived*>(this)->VisitNegativeValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NotValue* value) {
        static_cast<Derived*>(this)->VisitNotValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(NullValue* value) {
        static_cast<Derived*>(this)->VisitNullValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(SizeOfValue* value) {
        static_cast<Derived*>(this)->VisitSizeOfValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(UnsafeValue* value) {
        static_cast<Derived*>(this)->VisitUnsafeValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(ComptimeValue* value) {
        static_cast<Derived*>(this)->VisitComptimeValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(AlignOfValue* value) {
        static_cast<Derived*>(this)->VisitAlignOfValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(VariantCase* value) {
        static_cast<Derived*>(this)->VisitVariantCase(value);
    }
    inline void VisitByPtrTypeNoNullCheck(AddrOfValue* value) {
        static_cast<Derived*>(this)->VisitAddrOfValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(PointerValue* value) {
        static_cast<Derived*>(this)->VisitPointerValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(BlockValue* value) {
        static_cast<Derived*>(this)->VisitBlockValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(IfValue* value) {
        static_cast<Derived*>(this)->VisitIfValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(SwitchValue* value) {
        static_cast<Derived*>(this)->VisitSwitchValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(LoopValue* value) {
        static_cast<Derived*>(this)->VisitLoopValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(WrapValue* value) {
        static_cast<Derived*>(this)->VisitWrapValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(DestructValue* value) {
        static_cast<Derived*>(this)->VisitDestructValue(value);
    }
    inline void VisitByPtrTypeNoNullCheck(AnyType* type) {
        static_cast<Derived*>(this)->VisitAnyType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(ArrayType* type) {
        static_cast<Derived*>(this)->VisitArrayType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(StructType* type) {
        static_cast<Derived*>(this)->VisitStructType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(UnionType* type) {
        static_cast<Derived*>(this)->VisitUnionType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(BoolType* type) {
        static_cast<Derived*>(this)->VisitBoolType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(DoubleType* type) {
        static_cast<Derived*>(this)->VisitDoubleType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(FloatType* type) {
        static_cast<Derived*>(this)->VisitFloatType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(LongDoubleType* type) {
        static_cast<Derived*>(this)->VisitLongDoubleType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(ComplexType* type) {
        static_cast<Derived*>(this)->VisitComplexType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(Float128Type* type) {
        static_cast<Derived*>(this)->VisitFloat128Type(type);
    }
    inline void VisitByPtrTypeNoNullCheck(FunctionType* type) {
        static_cast<Derived*>(this)->VisitFunctionType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(GenericType* type) {
        static_cast<Derived*>(this)->VisitGenericType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(IntNType* type) {
        static_cast<Derived*>(this)->VisitIntNType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(PointerType* type) {
        static_cast<Derived*>(this)->VisitPointerType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(ReferenceType* type) {
        static_cast<Derived*>(this)->VisitReferenceType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(LinkedType* type) {
        static_cast<Derived*>(this)->VisitLinkedType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(StringType* type) {
        static_cast<Derived*>(this)->VisitStringType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(LiteralType* type) {
        static_cast<Derived*>(this)->VisitLiteralType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(DynamicType* type) {
        static_cast<Derived*>(this)->VisitDynamicType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(VoidType* type) {
        static_cast<Derived*>(this)->VisitVoidType(type);
    }
    inline void VisitByPtrTypeNoNullCheck(ExpressionType* type) {
        static_cast<Derived*>(this)->VisitExpressionType(type);
    }

    template<typename Thing>
    inline void VisitByPtrType(Thing* thing) {
        if(thing) VisitByPtrTypeNoNullCheck(thing);
    }

    // THE ULTIMATE VISITOR METHODS THAT CAN VISIT ALL
    // ITS IMPORTANT TO NOTE THAT THESE METHODS DO NOT PERFORM A NULL CHECK

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
    inline void visit(BaseType* type) {
        VisitTypeNoNullCheck(type);
    }
    inline void visit(TypeLoc& type) {
        VisitTypeNoNullCheck(const_cast<BaseType*>(type.getType()));
    }
    inline void visit(Scope& scope) {
        static_cast<Derived*>(this)->VisitScope(&scope);
    }

};