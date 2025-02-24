// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast_fwd.h"

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

    void VisitAssignmentStmt(AssignStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitBreakStmt(BreakStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitCommentStmt(Comment* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitContinueStmt(ContinueStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUnreachableStmt(UnreachableStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitDeleteStmt(DestructStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitImportStmt(ImportStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitReturnStmt(ReturnStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitSwitchStmt(SwitchStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitThrowStmt(ThrowStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitTypealiasStmt(TypealiasStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUsingStmt(UsingStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitVarInitStmt(VarInitStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitLoopBlock(LoopBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitProvideStmt(ProvideStmt* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitComptimeBlock(ComptimeBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitWhileLoopStmt(WhileLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitDoWhileLoopStmt(DoWhileLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitSymResNode(SymResNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitForLoopStmt(ForLoop* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitIfStmt(IfStatement* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitTryStmt(TryCatch* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitValueNode(ValueNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitValueWrapper(ValueWrapperNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitEnumDecl(EnumDeclaration* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitEnumMember(EnumMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitFunctionDecl(FunctionDeclaration* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitExtensionFunctionDecl(ExtensionFunction* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitMultiFunctionNode(MultiFunctionNode* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitImplDecl(ImplDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitInterfaceDecl(InterfaceDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitInitBlock(InitBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitStructDecl(StructDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitStructMember(StructMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitNamespaceDecl(Namespace* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUnionDecl(UnionDef* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitVariantDecl(VariantDefinition* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitVariantMember(VariantMember* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUnnamedStruct(UnnamedStruct* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUnnamedUnion(UnnamedUnion* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitScope(Scope* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitUnsafeBlock(UnsafeBlock* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitFunctionParam(FunctionParam* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitExtensionFuncReceiver(ExtensionFuncReceiver* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitGenericTypeParam(GenericTypeParameter* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitVariantMemberParam(VariantMemberParam* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitCapturedVariable(CapturedVariable* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }
    void VisitVariantCaseVariable(VariantCaseVariable* node) {
        static_cast<Derived*>(this)->VisitCommonNode((ASTNode*) node);
    }

    // ---------- Values ----------

    void VisitCharValue(CharValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitShortValue(ShortValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitIntValue(IntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitLongValue(LongValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitBigIntValue(BigIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitInt128Value(Int128Value* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUCharValue(UCharValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUShortValue(UShortValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUIntValue(UIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitULongValue(ULongValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUBigIntValue(UBigIntValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUInt128Value(UInt128Value* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNumberValue(NumberValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitFloatValue(FloatValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitDoubleValue(DoubleValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitBoolValue(BoolValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitStringValue(StringValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitExpression(Expression* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitArrayValue(ArrayValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitStructValue(StructValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitLambdaFunction(LambdaFunction* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitIfValue(IfStatement* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitSwitchValue(SwitchStatement* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitLoopValue(LoopBlock* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNewTypedValue(NewTypedValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNewValue(NewValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitPlacementNewValue(PlacementNewValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitIncDecValue(IncDecValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitIsValue(IsValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitDereferenceValue(DereferenceValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitRetStructParamValue(RetStructParamValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitAccessChain(AccessChain* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitCastedValue(CastedValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitVariableIdentifier(VariableIdentifier* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitIndexOperator(IndexOperator* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitFunctionCall(FunctionCall* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNegativeValue(NegativeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNotValue(NotValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitNullValue(NullValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitSizeOfValue(SizeOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitSymResValue(SymResValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitUnsafeValue(UnsafeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitComptimeValue(ComptimeValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitAlignOfValue(AlignOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitVariantCall(VariantCall* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitVariantCase(VariantCase* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitAddrOfValue(AddrOfValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitPointerValue(PointerValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitBlockValue(BlockValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitWrapValue(WrapValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    void VisitDestructValue(DestructValue* value) {
        static_cast<Derived*>(this)->VisitCommonValue(value);
    }

    // Types begin here

    void VisitAnyType(AnyType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitArrayType(ArrayType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitStructType(StructType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitUnionType(UnionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitBoolType(BoolType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitDoubleType(DoubleType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitFloatType(FloatType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitLongDoubleType(LongDoubleType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitComplexType(ComplexType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitFloat128Type(Float128Type* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitFunctionType(FunctionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitGenericType(GenericType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitIntNType(IntNType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitPointerType(PointerType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitReferenceType(ReferenceType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitLinkedType(LinkedType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitStringType(StringType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitLiteralType(LiteralType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitDynamicType(DynamicType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitVoidType(VoidType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitExpressionType(ExpressionType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitUnknownType(BaseType* type) {
        static_cast<Derived*>(this)->VisitCommonType((BaseType*) type);
    }

    void VisitNodeUnsafe(ASTNode* node) {
        switch(node->kind()) {
            case ASTNodeKind::AssignmentStmt:
                static_cast<Derived*>(this)->VisitAssignmentStmt((AssignStatement*) node);
                return;
            case ASTNodeKind::BreakStmt:
                static_cast<Derived*>(this)->VisitBreakStmt((BreakStatement*) node);
                return;
            case ASTNodeKind::CommentStmt:
                static_cast<Derived*>(this)->VisitCommentStmt((Comment*) node);
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
            case ASTNodeKind::ImportStmt:
                static_cast<Derived*>(this)->VisitImportStmt((ImportStatement*) node);
                return;
            case ASTNodeKind::ReturnStmt:
                static_cast<Derived*>(this)->VisitReturnStmt((ReturnStatement*) node);
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
            case ASTNodeKind::SymResNode:
                static_cast<Derived*>(this)->VisitSymResNode((SymResNode*) node);
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
            case ASTNodeKind::EnumDecl:
                static_cast<Derived*>(this)->VisitEnumDecl((EnumDeclaration*) node);
                return;
            case ASTNodeKind::EnumMember:
                static_cast<Derived*>(this)->VisitEnumMember((EnumMember*) node);
                return;
            case ASTNodeKind::FunctionDecl:
                static_cast<Derived*>(this)->VisitFunctionDecl((FunctionDeclaration*) node);
                return;
            case ASTNodeKind::ExtensionFunctionDecl:
                static_cast<Derived*>(this)->VisitExtensionFunctionDecl((ExtensionFunction*) node);
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
            case ASTNodeKind::ExtensionFuncReceiver:
                static_cast<Derived*>(this)->VisitExtensionFuncReceiver((ExtensionFuncReceiver*) node);
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
#ifdef DEBUG
            default:
                throw "UNHANDLED: node kind in non recursive visitor";
#endif
        }
    }

    inline void VisitNode(ASTNode* node) {
        if(node) VisitNodeUnsafe(node);
    }

    void VisitValueUnsafe(Value* value) {
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
                static_cast<Derived*>(this)->VisitIfValue((IfStatement*) value);
                return;
            case ValueKind::SwitchValue:
                static_cast<Derived*>(this)->VisitSwitchValue((SwitchStatement*) value);
                return;
            case ValueKind::LoopValue:
                static_cast<Derived*>(this)->VisitLoopValue((LoopBlock*) value);
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
            case ValueKind::SymResValue:
                static_cast<Derived*>(this)->VisitSymResValue((SymResValue*) value);
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
            case ValueKind::VariantCall:
                static_cast<Derived*>(this)->VisitVariantCall((VariantCall*) value);
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
            case ValueKind::BlockValue:
                static_cast<Derived*>(this)->VisitBlockValue((BlockValue*) value);
                return;
            case ValueKind::WrapValue:
                static_cast<Derived*>(this)->VisitWrapValue((WrapValue*) value);
                return;
            case ValueKind::DestructValue:
                static_cast<Derived*>(this)->VisitDestructValue((DestructValue*) value);
                return;
#ifdef DEBUG
            default:
                throw "UNHANDLED: value kind in non recursive visitor";
#endif
        }
    }

    void VisitValue(Value* value) {
        if(value) VisitValueUnsafe(value);
    }

    void VisitTypeUnsafe(BaseType* type) {
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
            case BaseTypeKind::Unknown:
                static_cast<Derived*>(this)->VisitUnknownType(type);
                return;
#ifdef DEBUG
            default:
                throw "UNHANDLED: type kind in non recursive visitor";
#endif
        }
    }

    void VisitType(BaseType* type) {
        if(type) VisitTypeUnsafe(type);
    }

};