// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "EnumMember.h"
#include "ast/base/ExtendableAnnotableNode.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/types/LinkedType.h"
#include "ast/base/LocatedIdentifier.h"

struct EnumDeclAttributes {

    AccessSpecifier specifier;

    bool deprecated;

};

class EnumDeclaration : public ExtendableAnnotableNode {
public:

    EnumDeclAttributes attrs;
    LocatedIdentifier located_id; ///< The name of the enum.
    std::unordered_map<chem::string_view, EnumMember*> members; ///< The values of the enum.
    ASTNode* parent_node;
    SourceLocation location;
    IntNType* underlying_type;

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
            IntNType* underlying_type,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : located_id(name_id), parent_node(parent_node), location(location),
        linked_type(name_id.identifier, this, location), underlying_type(underlying_type), attrs(specifier, false) {

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

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::EnumDecl;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final {
        return underlying_type->byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    ASTNode *child(const chem::string_view &name) final;

};