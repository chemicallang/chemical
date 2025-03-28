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

    /**
     * this boolean indicates that we're dealing with nodes imported from external module
     * that have been implemented before (for example var init needs to know about imported global variables)
     */
    bool external_module = false;

    /**
     * is the node declared in current module
     * this allows to not declare nodes twice in a single module
     */
    std::unordered_map<ASTNode*, bool> declared;

    CTopLevelDeclarationVisitor(
            ToCAstVisitor& visitor,
            CValueDeclarationVisitor* value_visitor
    );

    inline bool has_declared(ASTNode* node) {
        return declared.find(node) != declared.end();
    }

    inline void set_declared(ASTNode* node) {
        declared[node] = true;
    }

    // this will not declare it's contained functions
    void declare_struct_def_only(StructDefinition* def);

    void declare_struct_functions(StructDefinition* def);

    void early_declare_struct_def(StructDefinition* def);

    void declare_struct_iterations(StructDefinition* def);

    void declare_union_def_only(UnionDef* def);

    void declare_union_functions(UnionDef* def);

    void declare_union_iterations(UnionDef* def);

    void declare_interface(InterfaceDefinition* interface);

    void declare_variant_def_only(VariantDefinition* def);

    void declare_variant_functions(VariantDefinition* def);

    void early_declare_variant_def(VariantDefinition* def);

    void declare_variant_iterations(VariantDefinition* def);

    void declare_func(FunctionDeclaration* decl);

    // Visitor methods

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitIfStmt(IfStatement* node);

    void VisitTypealiasStmt(TypealiasStatement *statement);

    void VisitFunctionDecl(FunctionDeclaration *functionDeclaration);

    void VisitGenericFuncDecl(GenericFuncDecl* node);

    void VisitStructDecl(StructDefinition *structDefinition);

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