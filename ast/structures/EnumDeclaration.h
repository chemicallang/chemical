// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "EnumMember.h"
#include "ast/base/ASTNode.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/types/LinkedType.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/LocatedIdentifier.h"
#include <unordered_map>

struct EnumDeclAttributes {

    AccessSpecifier specifier;

    bool deprecated;

};

class EnumDeclaration : public ASTNode {
public:

    EnumDeclAttributes attrs;
    LocatedIdentifier located_id; ///< The name of the enum.
    std::unordered_map<chem::string_view, EnumMember*> members; ///< The values of the enum.
    TypeLoc underlying_type;

    // this is calculated during symbol resolution
    IntNType* underlying_integer_type;

    /**
     * this is the index at which any enum that inherits this enum should start
     * set by the parser
     */
    int next_start;

    // TODO remove this linked_type, we don't want to store stuff, that's not required
    LinkedType linked_type;

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(
            LocatedIdentifier name_id,
            TypeLoc underlying_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : ASTNode(ASTNodeKind::EnumDecl, parent_node, location), located_id(name_id),
        linked_type(this), underlying_type(underlying_type), attrs(specifier, false) {

    }

    /**
     * get the name of node
     */
    inline LocatedIdentifier* get_located_id() {
        return &located_id;
    }

    inline const chem::string_view& name_view() {
        return located_id.identifier;
    }

    inline bool deprecated() {
        return attrs.deprecated;
    }

    inline void set_deprecated(bool value) {
        attrs.deprecated = value;
    }

    AccessSpecifier specifier() {
        return attrs.specifier;
    }

    void set_specifier(AccessSpecifier specifier) {
        attrs.specifier = specifier;
    }

    /**
     * this gives the underlying integer type for the enum, it will not return
     * nullptr, because we initialize enum with int type if no type is given by user
     */
    [[nodiscard]]
    inline IntNType* get_underlying_integer_type() const {
        return underlying_integer_type;
    }

    /**
     * this will only return a enum decl if this enum inherits one, otherwise
     * nullptr is returned
     */
    EnumDeclaration* get_inherited_enum_decl();

    EnumDeclaration* copy(ASTAllocator &allocator) override {
        const auto decl = new (allocator.allocate<EnumDeclaration>()) EnumDeclaration(
            located_id,
            underlying_type.copy(allocator),
            parent(),
            encoded_location(),
            specifier()
        );
        for(auto& member : members) {
            decl->members[member.first] = member.second->copy(allocator);
        }
        decl->next_start = next_start;
        return decl;
    }

    inline BaseType* known_type() {
        return &linked_type;
    }

    uint64_t byte_size(bool is64Bit) final {
        return underlying_type->byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};