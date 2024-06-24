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

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(std::string name, std::unordered_map<std::string, std::unique_ptr<EnumMember>> members)
            : name(std::move(name)), members(std::move(members)) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_top_level(SymbolResolver &linker) override;

    EnumDeclaration *as_enum_decl() override {
        return this;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        // do nothing
    }

#endif

    ASTNode *child(const std::string &name) override;

    std::string name; ///< The name of the enum.
    std::unordered_map<std::string, std::unique_ptr<EnumMember>> members; ///< The values of the enum.

};