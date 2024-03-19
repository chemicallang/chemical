// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public ASTNode {
public:

    /**
     * @brief Construct a new ImplDefinition object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    ImplDefinition(
            std::string struct_name,
            std::string interface_name,
            std::vector<std::unique_ptr<ASTNode>> fields
    ) : struct_name(std::move(struct_name)), interface_name(std::move(interface_name)), members(std::move(fields)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    bool type_check(InterpretScope &scope) {
//        if (overrides.has_value()) {
//            auto inter = scope.find(overrides.value());
//            if (inter.first) {
//                auto interVal = inter.second->second->as_interface();
//                if (interVal != nullptr) {
//                    if (!interVal->verify(scope, name, members)) {
//                        return false;
//                    }
//                } else {
//                    scope.error("provided overridden value is not an interface");
//                }
//            } else {
//                scope.error("couldn't find the overridden interface " + overrides.value());
//            }
//        }
        return true;
    }

    void interpret(InterpretScope &scope) override {
        type_check(scope);
        scope.global->nodes[struct_name + ':' + interface_name] = this;
    }

    std::string representation() const override {
        std::string ret("impl " + interface_name + " for " + struct_name + " {\n");
        int i = 0;
        while (i < members.size()) {
            ret.append(members[i]->representation());
            if (i < members.size() - 1) {
                ret.append(1, '\n');
            }
            i++;
        }
        ret.append("\n}");
        return ret;
    }

private:
    std::string struct_name; ///< The name of the struct.
    std::string interface_name;
    std::vector<std::unique_ptr<ASTNode>> members; ///< The members of the struct.
};