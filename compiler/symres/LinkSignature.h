// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymbolResolver;

class TopLevelLinkSignature : public RecursiveVisitor<TopLevelLinkSignature> {
public:

    SymbolResolver& linker;

    /**
     * this is set before visiting any type
     */
    SourceLocation type_location = 0;

    /**
     * this requires that all types that are linked
     * with structs, variants, unions, type aliases be public
     */
    bool require_exported = false;

    /**
     * constructor
     */
    TopLevelLinkSignature(SymbolResolver& linker) : linker(linker) {

    }

    void link_param(GenericTypeParameter* param);

    // TODO: we don't want to override this
    // function types require that signature resolved is set to true
    // which by default is false
    void VisitFunctionType(FunctionType* type) {
        RecursiveVisitor<TopLevelLinkSignature>::VisitFunctionType(type);
        // TODO: remove this method
        type->data.signature_resolved = true;
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
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
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

    void VisitVariableIdentifier(VariableIdentifier* value);

    void VisitLinkedType(LinkedType* type);

    void VisitGenericType(GenericType* type);

    void VisitArrayType(ArrayType* type);

    void VisitAccessChain(AccessChain* value);

    void VisitExpression(Expression* value);

    void VisitAddrOfValue(AddrOfValue* value);

    void VisitArrayValue(ArrayValue* value);

    void VisitComptimeValue(ComptimeValue* value);

    void VisitDereferenceValue(DereferenceValue* value);

    void VisitIncDecValue(IncDecValue* value);

    void VisitIndexOperator(IndexOperator* value);

    void VisitIsValue(IsValue* value);

    void VisitLambdaFunction(LambdaFunction* value);

    void VisitNegativeValue(NegativeValue* value);

    void VisitUnsafeValue(UnsafeValue* value);

    void VisitNewValue(NewValue* value);

    void VisitNewTypedValue(NewTypedValue* value);

    void VisitPlacementNewValue(PlacementNewValue* value);

    void VisitNotValue(NotValue* value);

    void VisitPatternMatchExpr(PatternMatchExpr* value);

    void VisitBlockValue(BlockValue* value);

    void VisitStructValue(StructValue* value);

    void VisitEmbeddedNode(EmbeddedNode* node);

    void VisitEmbeddedValue(EmbeddedValue* value);

    void VisitDynamicValue(DynamicValue* value);

    void VisitComptimeBlock(ComptimeBlock* node);

    void LinkVariablesNoScope(VariablesContainer* container);

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkMembersContainerNoScopeExposed(MembersContainer* container);

    void LinkVariables(VariablesContainer* container);

    void LinkMembersContainer(MembersContainer* container);

    void LinkMembersContainerExposed(MembersContainer* container);

    void LinkMembersContainer(MembersContainer* container, AccessSpecifier specifier);

    void VisitUsingStmt(UsingStmt* node);

    void VisitAliasStmt(AliasStmt* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitScope(Scope* node);

    void VisitUnnamedStruct(UnnamedStruct* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitVariantMember(VariantMember* node);

    void VisitUnnamedUnion(UnnamedUnion* node);

};