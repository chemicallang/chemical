// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>
#include "preprocess/visitors/NonRecursiveVisitor.h"

class CValueDeclarationVisitor;

class CTopLevelDeclarationVisitor : public NonRecursiveVisitor<CTopLevelDeclarationVisitor>, public SubVisitor {
public:

    using NonRecursiveVisitor<CTopLevelDeclarationVisitor>::visit;

    CValueDeclarationVisitor* value_visitor;

    /**
     * this boolean is set to true when we have already declared the nodes
     * it means nodes are being declared for which code has already been generated
     * in another module, for example this allows to redefine generics that were already declared
     */
    bool redefining = false;

    CTopLevelDeclarationVisitor(
            ToCAstVisitor& visitor,
            CValueDeclarationVisitor* value_visitor
    );

    // this will not declare it's contained functions
    void declare_struct_def_only(StructDefinition* def);

    void declare_struct(StructDefinition* structDef);

    void declare_interface(InterfaceDefinition* interface);

    void declare_struct_iterations(StructDefinition* def);

    void declare_interface_iterations(InterfaceDefinition* def);

    void declare_variant_iterations(VariantDefinition* def);

    void declare_variant(VariantDefinition* structDef);

    void declare_func(FunctionDeclaration* decl);

    // Visitor methods

    void VisitVarInitStmt(VarInitStatement *init);

    void VisitTypealiasStmt(TypealiasStatement *statement);

    void VisitFunctionDecl(FunctionDeclaration *functionDeclaration);

    void VisitExtensionFunctionDecl(ExtensionFunction *extensionFunc);

    void VisitStructDecl(StructDefinition *structDefinition);

    void VisitVariantDecl(VariantDefinition *variant_def);

    void VisitNamespaceDecl(Namespace *ns);

    void VisitUnionDecl(UnionDef *def);

    void VisitInterfaceDecl(InterfaceDefinition *interfaceDefinition);

    void VisitImplDecl(ImplDefinition *implDefinition);

    void reset();

};