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

    /** `unsafe impl Trait for T` — required for Send/Sync opt-ins. */
    bool is_unsafe;

    /** `impl !Trait for T` — a negative auto-trait impl. */
    bool is_negative;

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
            AccessSpecifier specifier = AccessSpecifier::Internal,
            bool is_unsafe = false,
            bool is_negative = false
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(interface_type),
        struct_type(struct_type), attrs(specifier, is_unsafe, is_negative)
    {

    }

    /**
     * constructor
     */
    ImplDefinition(
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal,
            bool is_unsafe = false,
            bool is_negative = false
    ) : MembersContainer(ASTNodeKind::ImplDecl, parent_node, location), interface_type(nullptr),
        struct_type(nullptr), attrs(specifier, is_unsafe, is_negative) {

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
            encoded_location(),
            attrs.specifier,
            attrs.is_unsafe,
            attrs.is_negative
        );
        impl->attrs = attrs;
        MembersContainer::shallow_copy_into(*impl, allocator);
        return impl;
    }

    ASTNode* copy(ASTAllocator& allocator) final {
        const auto impl = new (allocator.allocate<ImplDefinition>()) ImplDefinition(
            {interface_type->copy(allocator), interface_type.encoded_location()},
            {struct_type->copy(allocator), struct_type.encoded_location()},
            parent(),
            encoded_location(),
            attrs.specifier,
            attrs.is_unsafe,
            attrs.is_negative
        );
        impl->attrs = attrs;
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


};