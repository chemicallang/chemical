// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public MembersContainer {
public:

    std::unique_ptr<BaseType> interface_type;
    std::unique_ptr<BaseType> struct_type;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ImplDefinition(
            std::unique_ptr<BaseType> interface_type,
            std::unique_ptr<BaseType> struct_type,
            ASTNode* parent_node,
            CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ImplDecl;
    }

    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node
    );

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) override;

#ifdef COMPILER_BUILD

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in struct decl
     */
    void code_gen_function(Codegen& gen, FunctionDeclaration* decl, InterfaceDefinition* linked_interface, StructDefinition* struct_def);

    /**
     * this is a helper function
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    void code_gen(Codegen &gen) override;

#endif

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

};