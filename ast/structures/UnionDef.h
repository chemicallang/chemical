// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/UnionType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct UnionDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime = false;

    bool is_direct_init = false;

    bool deprecated = false;

    bool anonymous = false;

    bool is_no_mangle = false;

    /**
     * are bodies of functions inside this struct retained across modules
     */
    bool retained_body = false;

    /**
     * the source explicitly requested packed layout (@packed / @repr("packed"))
     */
    bool packed = false;

};

class UnionDef : public ExtendableMembersContainerNode {
public:

    UnionDeclAttributes attrs;
    LinkedType linked_type;

    UnionDef(
        chem::string_view identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::UnionDecl, parent_node, location),
        attrs(specifier, false, false, false, false), linked_type(this) {

    }

    inline AccessSpecifier specifier() {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_anonymous() {
        return attrs.anonymous;
    }

    inline void set_anonymous(bool value) {
        attrs.anonymous = value;
    }

    inline bool is_no_mangle() {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) {
        attrs.is_no_mangle = no_mangle;
    }

    inline bool is_body_retained() const noexcept {
        return attrs.retained_body;
    }

    void set_is_body_retained(bool value) noexcept {
        attrs.retained_body = value;
    }

    inline bool is_packed() const noexcept {
        return attrs.packed;
    }

    inline void set_packed(bool value) noexcept {
        attrs.packed = value;
    }

    UnionDef* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<UnionDef>()) UnionDef(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    uint64_t byte_size(const TargetData& target) final {
        return largest_member_byte_size(target);
    }

    inline BaseType* known_type() {
        return &linked_type;
    }


};