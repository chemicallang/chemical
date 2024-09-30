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
    CSTToken* token;
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
            CSTToken* token,
            AccessSpecifier specifier = AccessSpecifier::Internal
    ) : name(std::move(name)), members(std::move(members)), parent_node(parent_node), token(token), linked_type(name, this, token), underlying_type(token), specifier(specifier) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::EnumDecl;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    const std::string& ns_node_identifier() override {
        return name;
    }

    BaseType* create_value_type(ASTAllocator& allocator) override;

    BaseType* known_type() override;

    uint64_t byte_size(bool is64Bit) override {
        return underlying_type.byte_size(is64Bit);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    ASTNode *child(const std::string &name) override;

    std::string name; ///< The name of the enum.
    std::unordered_map<std::string, EnumMember*> members; ///< The values of the enum.

};