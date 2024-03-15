// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class StructDefinition : public ASTNode, public Value {
public:

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            std::vector<std::unique_ptr<VarInitStatement>> fields,
            std::optional<std::string> overrides
    ) : name(std::move(name)), fields(std::move(fields)), overrides(std::move(overrides)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    bool type_check(InterpretScope &scope) {
        if (overrides.has_value()) {
            auto inter = scope.find(overrides.value());
            if (inter.first) {
                auto interVal = inter.second->second->as_interface();
                if (interVal != nullptr) {
                    if (!interVal->verify(scope, name, fields)) {
                        return false;
                    }
                } else {
                    scope.error("provided overridden value is not an interface");
                }
            } else {
                scope.error("couldn't find the overridden interface " + overrides.value());
            }
        }
        return true;
    }

    void interpret(InterpretScope &scope) override {
        type_check(scope);
        scope.values[name] = this;
    }

    std::string representation() const override {
        std::string ret("struct " + name + " ");
        if(overrides.has_value()) {
            ret.append(": " + overrides.value() + " {\n");
        } else {
            ret.append("{\n");
        }
        int i = 0;
        while (i < fields.size()) {
            ret.append(fields[i]->representation());
            if (i < fields.size() - 1) {
                ret.append(1, '\n');
            }
            i++;
        }
        ret.append("\n}");
        return ret;
    }

    void scope_ends() override {
        // don't call destructor when scope ends
    }

private:
    std::string name; ///< The name of the struct.
    std::optional<std::string> overrides;
    std::vector<std::unique_ptr<VarInitStatement>> fields; ///< The members of the struct.
};