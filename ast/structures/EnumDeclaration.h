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

class EnumDeclaration : public ExtendableAnnotableNode {
public:

    ASTNode* parent_node;
    SourceLocation location;
    AccessSpecifier specifier;
    LinkedType linked_type;
    IntType underlying_type;

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(
            std::string name,
            std::unordered_map<std::string, EnumMember*> members,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : name(std::move(name)), members(std::move(members)), parent_node(parent_node), location(location), linked_type(name, this, location), underlying_type(location), specifier(specifier) {

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

    const std::string& ns_node_identifier() final {
        return name;
    }

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final {
        return underlying_type.byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

    ASTNode *child(const std::string &name) final;

    std::string name; ///< The name of the enum.
    std::unordered_map<std::string, EnumMember*> members; ///< The values of the enum.

};