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

    void visit(VarInitStatement *init) override;

    void visit(TypealiasStatement *statement) override;

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(VariantDefinition *variant_def) override;

    void visit(Namespace *ns) override;

    void visit(UnionDef *def) override;

    void visit(InterfaceDefinition *interfaceDefinition) override;

    void visit(ImplDefinition *implDefinition) override;

    void reset();

};