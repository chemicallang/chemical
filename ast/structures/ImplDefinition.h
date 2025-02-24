// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public MembersContainer {
public:

    BaseType* interface_type;
    BaseType* struct_type;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ImplDefinition(
            BaseType* interface_type,
            BaseType* struct_type,
            ASTNode* parent_node,
            SourceLocation location
    ) : MembersContainer(ASTNodeKind::ImplDecl, location), interface_type(interface_type), struct_type(struct_type), parent_node(parent_node) {

    }


    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node,
            SourceLocation location
    ) : MembersContainer(ASTNodeKind::ImplDecl, location), parent_node(parent_node) {

    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    uint64_t byte_size(bool is64Bit) final;

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

    void code_gen(Codegen &gen) final;

#endif

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};