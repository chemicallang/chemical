// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"

class EnumDeclaration : public ASTNode {
public:

    /**
     * @brief Construct a new EnumDeclaration object.
     *
     * @param name The name of the enum.
     * @param values The values of the enum.
     */
    EnumDeclaration(std::string name, std::unordered_map<std::string, unsigned int> members)
            : name(std::move(name)), members(std::move(members)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void declare_top_level(ASTLinker &linker) override {
        linker.current[name] = this;
    }

    std::string representation() const override {
        std::string rep("enum " + name + " {\n");
        unsigned int i = 0;
        for(const auto& member : members) {
            rep.append(member.first);
            if (i != members.size() - 1) {
                rep.append(",\n");
            }
            i++;
        }
        rep.append("\n}");
        return rep;
    }

private:
    std::string name; ///< The name of the enum.
    std::unordered_map<std::string, unsigned int> members; ///< The values of the enum.
};