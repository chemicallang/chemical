// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public MembersContainer {
public:

    TypeLoc interface_type;
    TypeLoc struct_type;

    /**
     * constructor
     */
    ImplDefinition(
            TypeLoc interface_type,
            TypeLoc struct_type,
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
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(nullptr), struct_type(nullptr) {

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

    void code_gen_function_primitive(Codegen &gen, FunctionDeclaration* decl, InterfaceDefinition* linked_interface);

    /**
     * this is a helper function
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) final;

#endif

};