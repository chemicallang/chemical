// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CValueDeclarationVisitor;

class CTopLevelDeclarationVisitor : public Visitor, public SubVisitor {
public:

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

    void declare_variant(VariantDefinition* structDef);

    void declare_func(FunctionDeclaration* decl);

    void visit(VarInitStatement *init) final;

    void visit(TypealiasStatement *statement) final;

    void visit(FunctionDeclaration *functionDeclaration) final;

    void visit(ExtensionFunction *extensionFunc) final;

    void visit(StructDefinition *structDefinition) final;

    void visit(VariantDefinition *variant_def) final;

    void visit(Namespace *ns) final;

    void visit(UnionDef *def) final;

    void visit(InterfaceDefinition *interfaceDefinition) final;

    void visit(ImplDefinition *implDefinition) final;

    void reset();

};