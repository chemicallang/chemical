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
    EnumDeclaration(std::string name, std::vector<std::string> values)
            : name(std::move(name)), values(std::move(values)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void interpret(InterpretScope &scope) override {
        members = new std::unordered_map<std::string, std::unique_ptr<Value>>;
        unsigned int i = 0;
        for(const auto& member : values) {
            (*members)[member] = std::make_unique<IntValue>(i);
            i++;
        }
        scope.global->nodes[name] = this;
    }

    std::string representation() const override {
        std::string rep("enum " + name + " {\n");
        unsigned int i = 0;
        while (i < values.size()) {
            rep.append(values[i]);
            if (i != values.size() - 1) {
                rep.append(",\n");
            }
            i++;
        }
        rep.append("\n}");
        return rep;
    }

    Value * child(const std::string &child_name) override {
        return (*members)[child_name].get();
    }

    void interpret_scope_ends(InterpretScope &scope) override {
        delete members;
    }

private:
    std::string name; ///< The name of the enum.
    std::vector<std::string> values; ///< The values of the enum.
    std::unordered_map<std::string, std::unique_ptr<Value>>* members;
};