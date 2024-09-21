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

class EnumDeclaration : public ExtendableAnnotableNode {
public:

    ASTNode* parent_node;
    // the type is contained inside here
    IntType type;
    CSTToken* token;
    AccessSpecifier specifier;

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
    ) : name(std::move(name)), members(std::move(members)), parent_node(parent_node), token(token), type(nullptr), specifier(specifier) {

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

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) override;

    const std::string& ns_node_identifier() override {
        return name;
    }

    BaseType* create_value_type(ASTAllocator& allocator) override;

//    hybrid_ptr<BaseType> get_value_type() override;

    BaseType* known_type() override;

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