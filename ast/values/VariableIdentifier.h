// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public Value {
public:
    /**
     * @brief Construct a new VariableIdentifier object.
     *
     * @param value The string value.
     */
    VariableIdentifier(std::string  value) : value(std::move(value)) {}


    void set_in_parent(scope_vars vars, InterpretValue* newValue) override {
        if(vars.contains(value)) {
            vars[value] = newValue;
        } else {
            std::cerr << "Couldn't set variable " << value << " as there's no such variable in parent";
        }
    }

    InterpretValue * find_in_parent(std::unordered_map<std::string, InterpretValue *> &scopeVars) override {
        if(scopeVars.contains(value)) {
            return scopeVars[value];
        } else {
            return nullptr;
        }
    }

    InterpretValue * travel(std::unordered_map<std::string, InterpretValue *> &scopeVars) override {
        return find_in_parent(scopeVars);
    }

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.
};