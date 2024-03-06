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
    VariableIdentifier(std::string value) : value(std::move(value)) {}

    void set_in_parent(scope_vars vars, Value *newValue) override {
        try {
            auto v = vars.at(value);
            if (v != nullptr && v->delete_value()) {
                delete v;
            }
            vars[value] = newValue;
        } catch (const std::out_of_range &e) {
            std::cerr << "Couldn't set variable " << value << " as there's no such variable in parent, " << e.what();
        }
    }

    void set_in_parent(scope_vars vars, Value *newValue, Operation op) override {
        try {
            auto v = vars.at(value);
            auto nextValue = ExpressionEvaluator::functionVector[ExpressionEvaluator::index(v->value_type(), v->value_type(), op)](v, newValue);
            if (v->delete_value()) {
                delete v;
            }
            vars[value] = nextValue;
        } catch (const std::out_of_range &e) {
            std::cerr << "Couldn't set variable " << value << " as there's no such variable in parent, or previous value doesn't exist " << e.what();
        }
    }

    Value *evaluated_value(scope_vars &scopeVars) override {
        if (scopeVars.contains(value)) {
            pointing = scopeVars[value];
            return pointing;
        } else {
            return nullptr;
        }
    }

    Value *find_in_parent(scope_vars scopeVars) override {
        if (scopeVars.contains(value)) {
            pointing = scopeVars[value];
            return pointing;
        } else {
            return nullptr;
        }
    }

    Value *travel(scope_vars &scopeVars) override {
        return find_in_parent(scopeVars);
    }

    std::string representation() const override {
        return value;
    }

private:
    Value *pointing;
    std::string value; ///< The string value.
};