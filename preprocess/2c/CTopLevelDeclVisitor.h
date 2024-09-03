// Copyright (c) Qinetik 2024.

#pragma once

#include "SubVisitor.h"
#include "ast/utils/CommonVisitor.h"
#include <string>

class CValueDeclarationVisitor;

struct DeclaredNodeData {
    union {
        // total iterations that are done
        int16_t iterations_done;
        bool declared_struct;
    } struct_def;
    union {
        // total iterations that are done
        int16_t iterations_done;
    } variant_def;
};

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
            ToCAstVisitor* visitor,
            CValueDeclarationVisitor* value_visitor
    );

    /**
     * nodes can be declared early if present in generics
     */
    std::unordered_map<ASTNode*, DeclaredNodeData> declared_nodes;

    // this will not declare it's contained functions
    void declare_struct_def_only(StructDefinition* def, bool check_declared);

    void declare_struct(StructDefinition* structDef, bool check_declared);

    void declare_variant(VariantDefinition* structDef);

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