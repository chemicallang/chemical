// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "EnumMember.h"
#include "ast/base/ExtendableAnnotableNode.h"

class EnumDeclaration : public ExtendableAnnotableNode {
public:

    ASTNode* parent_node;

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(
            std::string name,
            std::unordered_map<std::string, std::unique_ptr<EnumMember>> members,
            ASTNode* parent_node
    ) : name(std::move(name)), members(std::move(members)), parent_node(parent_node) {}

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    std::string ns_node_identifier() override {
        return name;
    }

    EnumDeclaration *as_enum_decl() override {
        return this;
    }

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        // do nothing
    }

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

    ASTNode *child(const std::string &name) override;

    std::string name; ///< The name of the enum.
    std::unordered_map<std::string, std::unique_ptr<EnumMember>> members; ///< The values of the enum.

};