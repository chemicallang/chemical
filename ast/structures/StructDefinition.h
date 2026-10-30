// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "MembersContainer.h"
#include <optional>
#include <map>
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/StructType.h"
#include "ast/types/LinkedType.h"

struct StructDeclAttributes {

    /**
     * the access specifier for the declaration
     */
    AccessSpecifier specifier;

    /**
     * is the struct and it's functions comptime
     */
    bool is_comptime = false;

    /**
     * is a compiler declaration (present inside the compiler, no need to import)
     */
    bool is_compiler_decl = false;

    /**
     * is direct initialization
     * constructor or de constructor allow functions to be called automatically
     */
    bool is_direct_init = false;

    /**
     * is this struct deprecated
     */
    bool deprecated = false;

    /**
     * if struct shouldn't be initialized (user asked)
     */
    bool no_init = false;

    /**
     * struct is marked to allow use after the move
     */
    bool use_after_move = false;

    /**
     * struct marked anonymous to keep it anonymous in generated code
     */
    bool anonymous = false;

    /**
     * is no mangle means, module name won't be added to this struct
     */
    bool is_no_mangle = false;

    /**
     * when user marks a struct extern, it means that this struct is defined in another module
     */
    bool is_extern = false;

    /**
     * are bodies of functions inside this struct retained across modules
     */
    bool retained_body = false;

    /**
     * the source explicitly requested packed layout (@packed / @repr("packed"))
     */
    bool packed = false;

    /**
     * the source requested the C transparent-newtype ABI (@repr("transparent"))
     */
    bool transparent = false;

};

class StructDefinition : public ExtendableMembersContainerNode {
private:
    StructDeclAttributes attrs;

public:
    LinkedType linked_type;

    /**
     * lifetime parameters declared on this struct, e.g. struct string_view 'a { }
     */
    std::vector<chem::string_view> lifetime_params;

    /**
     * alignment this struct requires, applied via @align(N) or @maxalign annotation.
     * zero means the struct does not force an alignment
     */
    uint32_t required_alignment = 0;

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            chem::string_view identifier,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ExtendableMembersContainerNode(identifier, ASTNodeKind::StructDecl, parent_node, location),
        attrs(specifier, false, false, false, false, false, false, false, false, false),
        linked_type(this) {

    }

    inline AccessSpecifier specifier() const noexcept {
        return attrs.specifier;
    }

    inline void set_specifier(AccessSpecifier specifier) noexcept {
        attrs.specifier = specifier;
    }

    inline bool is_comptime() const noexcept {
        return attrs.is_comptime;
    }

    inline void set_comptime(bool value) noexcept {
        attrs.is_comptime = value;
    }

    inline bool is_compiler_decl() const noexcept {
        return attrs.is_compiler_decl;
    }

    inline void set_compiler_decl(bool value) noexcept {
        attrs.is_comptime = value;
        attrs.is_compiler_decl = value;
        attrs.retained_body = value;
    }

    inline bool is_direct_init() const noexcept {
        return attrs.is_direct_init;
    }

    inline void set_direct_init(bool value) noexcept {
        attrs.is_direct_init = value;
    }

    inline bool is_deprecated() const noexcept {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) noexcept {
        attrs.deprecated = value;
    }

    inline bool is_no_init() const noexcept {
        return attrs.no_init;
    };

    inline void set_no_init(bool value) noexcept {
        attrs.no_init = value;
    }

    inline bool is_use_after_move() const noexcept {
        return attrs.use_after_move;
    };

    inline void set_use_after_move(bool value) noexcept {
        attrs.use_after_move = value;
    }

    inline bool is_anonymous() const noexcept {
        return attrs.anonymous;
    };

    inline void set_anonymous(bool value) noexcept {
        attrs.anonymous = value;
    }

    inline bool is_no_mangle() const noexcept {
        return attrs.is_no_mangle;
    }

    inline void set_no_mangle(bool no_mangle) noexcept {
        attrs.is_no_mangle = no_mangle;
    }

    inline bool is_extern() const noexcept {
        return attrs.is_extern;
    }

    inline void set_extern(bool value) noexcept {
        attrs.is_extern = value;
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

    inline bool is_transparent() const noexcept {
        return attrs.transparent;
    }

    inline void set_transparent(bool value) noexcept {
        attrs.transparent = value;
    }

    inline bool has_destructor() {
        return destructor_func() != nullptr;
    }

    inline uint32_t get_required_alignment() const noexcept {
        return required_alignment;
    }

    inline void set_required_alignment(uint32_t value) noexcept {
        required_alignment = value;
    }

    StructDefinition* shallow_copy(ASTAllocator& allocator) {
        const auto def = new (allocator.allocate<StructDefinition>()) StructDefinition(
                identifier, parent(), encoded_location(), specifier()
        );
        def->attrs = attrs;
        def->required_alignment = required_alignment;
        def->no_drop = no_drop;
        ExtendableMembersContainerNode::shallow_copy_into(*def, allocator);
        return def;
    }

    bool is_exported_fast() {
        return specifier() == AccessSpecifier::Public;
    }

    /**
     * generates any default constructors / destructors required
     * should be called after link signature
     */
    void generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode);

    inline BaseType* known_type() {
        return &linked_type;
    }

    uint64_t byte_size(const TargetData& target) final {
        return total_byte_size(target);
    }

//    [[nodiscard]]
//    BaseType* copy(ASTAllocator &allocator) const final;


};