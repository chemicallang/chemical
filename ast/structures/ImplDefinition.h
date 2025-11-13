// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/structures/InterfaceDefinition.h"

struct ImplDeclAttrs {

    AccessSpecifier specifier;

};

class ImplDefinition : public MembersContainer {
public:

    TypeLoc interface_type;
    TypeLoc struct_type;
    ImplDeclAttrs attrs;

    /**
     * constructor
     */
    ImplDefinition(
            TypeLoc interface_type,
            TypeLoc struct_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(interface_type),
        struct_type(struct_type), attrs(specifier)
    {

    }

    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(nullptr),
        struct_type(nullptr), attrs(specifier) {

    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
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

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) final;

    void code_gen_external_declare(Codegen &gen) override;

#endif

};