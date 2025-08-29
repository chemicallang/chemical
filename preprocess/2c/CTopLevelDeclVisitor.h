// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "SubVisitor.h"
#include <string>
#include "preprocess/visitors/NonRecursiveVisitor.h"
#include <unordered_map>

class CValueDeclarationVisitor;

class CTopLevelDeclarationVisitor : public NonRecursiveVisitor<CTopLevelDeclarationVisitor>, public SubVisitor {
public:

    using NonRecursiveVisitor<CTopLevelDeclarationVisitor>::visit;

    CValueDeclarationVisitor* value_visitor;

    CTopLevelDeclarationVisitor(
            ToCAstVisitor& visitor,
            CValueDeclarationVisitor* value_visitor
    );

    // this will not declare it's contained functions
    void declare_struct_def_only(StructDefinition* def);

    void declare_struct_functions(StructDefinition* def);

    void early_declare_struct_def(StructDefinition* def);

    inline void declare_struct_iterations(StructDefinition* def) {
        early_declare_struct_def(def);
        declare_struct_functions(def);
    }

    void declare_union_def_only(UnionDef* def);

    void declare_union_functions(UnionDef* def);

    void early_declare_union_def(UnionDef* def);

    inline void declare_union_iterations(UnionDef* def) {
        early_declare_union_def(def);
        declare_union_functions(def);
    }

    void declare_interface(InterfaceDefinition* interface);

    void declare_variant_def_only(VariantDefinition* def);

    void declare_variant_functions(VariantDefinition* def);

    void early_declare_variant_def(VariantDefinition* def);

    inline void declare_variant_iterations(VariantDefinition* def) {
        early_declare_variant_def(def);
        declare_variant_functions(def);
    }

    void declare_func(FunctionDeclaration* decl);

    // Visitor methods

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitIfStmt(IfStatement* node);

    void VisitTypealiasStmt(TypealiasStatement *statement);

    void VisitFunctionDecl(FunctionDeclaration *functionDeclaration);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitStructDecl(StructDefinition *structDefinition);

    void VisitGenericTypeDecl(GenericTypeDecl* node);

    void VisitGenericStructDecl(GenericStructDecl* node);

    void VisitGenericUnionDecl(GenericUnionDecl* node);

    void VisitGenericInterfaceDecl(GenericInterfaceDecl* node);

    void VisitGenericVariantDecl(GenericVariantDecl* node);

    void VisitVariantDecl(VariantDefinition *variant_def);

    void VisitNamespaceDecl(Namespace *ns);

    void VisitUnionDecl(UnionDef *def);

    void VisitInterfaceDecl(InterfaceDefinition *interfaceDefinition);

    void VisitImplDecl(ImplDefinition *implDefinition);

    void reset();

};