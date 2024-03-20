// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/base/GlobalInterpretScope.h"

class StructDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            std::vector<std::unique_ptr<ASTNode>> fields,
            std::optional<std::string> overrides
    ) : name(std::move(name)), fields(std::move(fields)), overrides(std::move(overrides)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    StructDefinition *as_struct_def() override {
        return this;
    }

    bool type_check(InterpretScope &scope) {
        if (overrides.has_value()) {
            auto inter = scope.global->values.find(overrides.value());
            if (inter == scope.global->values.end()) {
//                auto interVal = inter->second->as_interface();
//                if (interVal != nullptr) {
//                    if (!interVal->verify(scope, name, fields)) {
//                        return false;
//                    }
//                } else {
//                    scope.error("provided overridden value is not an interface");
//                }
            } else {
                scope.error("couldn't find the overridden interface " + overrides.value());
            }
        }
        return true;
    }

    void interpret(InterpretScope &scope) override {
        scope.global->nodes[name] = this;
    }

    void interpret_scope_ends(InterpretScope &scope) override {
        scope.global->erase_node(name);
    }

    std::string representation() const override {
        std::string ret("struct " + name + " ");
        if (overrides.has_value()) {
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

private:
    std::string name; ///< The name of the struct.
    std::optional<std::string> overrides;
    std::vector<std::unique_ptr<ASTNode>> fields; ///< The members of the struct.
};