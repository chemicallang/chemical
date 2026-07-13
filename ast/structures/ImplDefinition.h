// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/structures/InterfaceDefinition.h"

class AnnotationController;

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

    uint64_t byte_size(const TargetData& target) final;

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

    ASTNode* copy(ASTAllocator& allocator) final {
        const auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(
            {interface_type->copy(allocator), interface_type.encoded_location()},
            {struct_type->copy(allocator), struct_type.encoded_location()},
            parent(),
            encoded_location()
        );
        MembersContainer::copy_into(*impl, allocator);
        return impl;
    }

    inline FunctionDeclaration* implementation_of(FunctionDeclaration* base) {
        return direct_child_function(base->name_view());
    }

    inline GenericFuncDecl* implementation_of(GenericFuncDecl* base) {
        const auto child = direct_child(base->name_view());
        return child->kind() == ASTNodeKind::GenericFuncDecl ? child->as_gen_func_decl_unsafe() : nullptr;
    }

#ifdef COMPILER_BUILD

    /**
     * responsible for generating code for a single function in a union decl
     * read the documentation in struct decl
     */
    void code_gen_function(Codegen& gen, FunctionDeclaration* decl, InterfaceDefinition* linked_interface, ExtendableMembersContainerNode* def);

    /**
     * this is a helper function
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    /**
     * code gen bodies of functions in the given interface by finding implementations in this impl
     */
    void code_gen_bodies(Codegen& gen, InterfaceDefinition* interface, ExtendableMembersContainerNode* user);

    void strengthen_static_declare(Codegen& gen, InterfaceDefinition* interface, ExtendableMembersContainerNode* node);

    void code_gen_declare(Codegen &gen) override;

    void code_gen(Codegen &gen) final;

    void code_gen_external_declare(Codegen &gen) override;

#endif

};