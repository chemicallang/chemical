// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"
#include "ast/structures/InterfaceDefinition.h"

class ImplDefinition : public MembersContainer {
public:

    /**
     * @brief Construct a new ImplDefinition object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    ImplDefinition(
            std::string interface_name,
            std::optional<std::string> struct_name
    ) : struct_name(std::move(struct_name)), interface_name(std::move(interface_name)) {}

    void accept(Visitor *visitor) override {
        visitor->visit(this);
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

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void declare_and_link(SymbolResolver &linker) override;

    void interpret(InterpretScope &scope) override {
        type_check(scope);
    }

    std::string representation() const override {
        std::string ret("impl " + interface_name + " {\n");
        ret.append(MembersContainer::representation());
        ret.append("\n}");
        return ret;
    }

    ASTNode* linked;
    std::optional<std::string> struct_name; ///< The name of the struct.
    std::string interface_name;
};