// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "MembersContainer.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"
#include "ast/base/InterfaceBits.h"

struct InterfaceDefinitionAttrs {

    /**
     * the access specifier
     */
    AccessSpecifier specifier;

    /**
     * this is set to true when even a single implementation is detected
     */
    bool has_implementation = false;

    /**
     * is this interface deprecated
     */
    bool deprecated = false;

    /**
     * is the interface a static interface (only one implementation allowed)
     */
    bool is_static = false;

    /**
     * this interface won't be prefixed with module name
     */
    bool is_no_mangle = false;

    /**
     * make the interface extern, meaning implementation comes from outside
     */
    bool is_extern = false;

    /**
     * every interface is by default object safe
     * unless user sets it explicitly or we check it during sym res
     */
    bool object_safe = true;

    /**
     * an interface can be made explicitly non dynamic
     * like the Drop trait, which allows us to prevent dynamic usage
     */
    bool non_dyn = false;

    /**
     * are bodies of functions inside this struct retained across modules
     */
    bool retained_body = false;

    /**
     * `unsafe interface` — implementing this interface is an unsafe contract.
     * A safe impl is rejected; the implementor takes responsibility for the
     * interface's invariants (e.g. a raw-pointer iterator contract).
     */
    bool is_unsafe = false;

};

class InterfaceDefinition : public ExtendableMembersContainerNode {
public:

    /**
     * linked type to self
     */
    LinkedType linked_type;

    /**
     * users of this interface,
     */
    std::vector<ExtendableMembersContainerNode*> users;

    /**
     * this maps structs that implement this interface with their global variable pointers
     * for the vtable generated
     */
    std::unordered_map<ExtendableMembersContainerNode*, bool> vtable_pointers;

    /**
     * the active user, can be retrieved by functions to see for which
     * struct/variant is code being generated for by interface
     */
    ExtendableMembersContainerNode* active_user = nullptr;

    /**
     * some data is stored in this struct to make it occupy less size
     */
    InterfaceDefinitionAttrs attrs;

    /**
     * trait bits for this interface (Copy, etc.)
     * computed during symbol resolution by OR-ing inherited interfaces' bits.
     */
    InterfaceBits interface_bits;

    /**
     * constructor
     */
    InterfaceDefinition(
            chem::string_view identifier,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal,
            bool is_unsafe = false
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::InterfaceDecl, parent_node, location),
        attrs(specifier, false, false, false), linked_type(this) {
        attrs.is_unsafe = is_unsafe;
    }

    [[nodiscard]]
    inline AccessSpecifier specifier() const {
        return attrs.specifier;
    }

    inline void set_specifier_fast(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    inline bool is_static() {
        return attrs.is_static;
    }

    inline void set_is_static(bool value) {
        attrs.is_static = value;
    }

    inline bool has_implementation() {
        return attrs.has_implementation;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    inline bool is_extern() {
        return attrs.is_extern;
    }

    inline bool is_unsafe() const {
        return attrs.is_unsafe;
    }

    inline void set_extern(bool value) {
        attrs.is_extern = value;
    }

    inline bool is_object_safe() {
        return attrs.object_safe;
    }

    inline void set_object_safe(bool value) {
        attrs.object_safe = value;
    }

    inline bool is_non_dynamic() {
        return attrs.non_dyn;
    }

    inline void set_non_dynamic(bool value) {
        attrs.non_dyn = value;
    }

    inline bool is_body_retained() const noexcept {
        return attrs.retained_body;
    }

    void set_is_body_retained(bool value) noexcept {
        attrs.retained_body = value;
    }

    inline bool generates_vtable() {
        return is_object_safe() && !is_non_dynamic() && !is_static();
    }

    InterfaceDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<InterfaceDefinition>()) InterfaceDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    /**
     * direct usage of this struct is registered with this interface
     * the interfaces inherited by this interface aren't registered
     * which should be registered by calling register_use_to_inherited_interfaces
     */
    void register_use(ExtendableMembersContainerNode* definition) {
        users.emplace_back(definition);
        attrs.has_implementation = true;
    }

    void register_impl(ImplDefinition* definition);

    uint64_t byte_size(const TargetData& target) final;

    inline BaseType* known_type() {
        return &linked_type;
    }


};