// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/RecursiveVisitor.h"

class SymbolResolver;

class TopLevelLinkSignature : public RecursiveVisitor<TopLevelLinkSignature> {
public:

    SymbolResolver& linker;

    /**
     * constructor
     */
    TopLevelLinkSignature(SymbolResolver& linker) : linker(linker) {

    }

    void LinkVariablesNoScope(VariablesContainer* container);

    void LinkMembersContainerNoScope(MembersContainer* container);

    void LinkVariables(VariablesContainer* container) {
        linker.scope_start();
        LinkVariablesNoScope(container);
        linker.scope_end();
    }

    void LinkMembersContainer(MembersContainer* container) {
        linker.scope_start();
        LinkMembersContainerNoScope(container);
        linker.scope_end();
    }

    void VisitUsingStmt(UsingStmt* node);

    void VisitAliasStmt(AliasStmt* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitEnumDecl(EnumDeclaration* node);

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

    void VisitStructMember(StructMember* node);

    void VisitUnnamedStruct(UnnamedStruct* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

    void VisitVariantMember(VariantMember* node);

    void VisitUnnamedUnion(UnnamedUnion* node);

    void VisitVariantMemberParam(VariantMemberParam* node);

};