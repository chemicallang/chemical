// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/NonRecursiveVisitor.h"

class TopLevelDeclSymDeclare : public NonRecursiveVisitor<TopLevelDeclSymDeclare> {
public:

    SymbolResolver& linker;

    /**
     * constructor
     */
    TopLevelDeclSymDeclare(SymbolResolver& linker) : linker(linker) {

    }

    void VisitAliasStmt(AliasStmt* node);

    void VisitImportStmt(ImportStatement* node);

    void VisitTypealiasStmt(TypealiasStatement* node);

    void VisitVarInitStmt(VarInitStatement* node);

    void VisitEnumDecl(EnumDeclaration* node);

    void VisitFunctionDecl(FunctionDeclaration* node);

    void VisitScope(Scope* node);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericImplDecl(GenericImplDecl* node);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitIfStmt(IfStatement* node);

    void VisitImplDecl(ImplDefinition* node);

    void VisitInterfaceDecl(InterfaceDefinition* node);

    void VisitNamespaceDecl(Namespace* node);

    void VisitStructMember(StructMember* node);

    void VisitStructDecl(StructDefinition* node);

    void VisitUnionDecl(UnionDef* node);

    void VisitVariantDecl(VariantDefinition* node);

};