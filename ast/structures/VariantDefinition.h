// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/LinkedType.h"

struct VariantDeclAttributes {

    AccessSpecifier specifier;

    bool is_comptime = false;

    bool is_compiler_decl = false;

    bool deprecated = false;

    bool anonymous = false;

    bool is_no_mangle = false;

    /**
     * are bodies of functions inside this struct retained across modules
     */
    bool retained_body = false;

};

class VariantDefinition : public ExtendableMembersContainerNode {
public:

    VariantDeclAttributes attrs;
    LinkedType ref_type;

    /**
     * constructor
     */
    VariantDefinition(
        chem::string_view identifier,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::VariantDecl, parent_node, location),
        ref_type(this), attrs(specifier, false, false, false, false) {
    }

    AccessSpecifier specifier() {
        return attrs.specifier;
    }

    void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    inline bool is_comptime() {
        return attrs.is_comptime;
    }

    inline void set_is_comptime(bool value) {
        attrs.is_comptime = value;
    }

    inline bool is_compiler_decl() {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
        attrs.retained_body = value;
    }

    inline bool is_deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
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

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    VariantDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<VariantDefinition>()) VariantDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    void generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode);

    inline BaseType* known_type() {
        return &ref_type;
    }

    uint64_t byte_size(const TargetData& target) final;

    /**
     * check if it includes any member who has a struct, that requires a destructor
     */
    bool requires_destructor();


};