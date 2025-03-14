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

    /**
     * constructor
     */
    ImplDefinition(
            BaseType* interface_type,
            BaseType* struct_type,
            ASTNode* parent_node,
            SourceLocation location
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(interface_type),
        struct_type(struct_type)
    {

    }

    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node,
            SourceLocation location
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location) {

    }

    uint64_t byte_size(bool is64Bit) final;

    ImplDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(
            interface_type,
            struct_type,
            parent(),
            encoded_location()
        );
        MembersContainer::shallow_copy_into(*impl, allocator);
        return impl;
    }

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

    void link_signature_no_scope(SymbolResolver &linker);

    void link_signature(SymbolResolver& linker) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

};