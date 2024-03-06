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

    void set_in_parent(InterpretScope &scope, Value *newValue) override {
        auto it = find(scope);
        if (!it.first) {
            std::cerr << "Couldn't set variable " << value
                      << " as there's no such variable in parent, or previous value doesn't exist ";
            return;
        }
        auto v = it.second->second;
        if (v->delete_value()) {
            delete v;
        }
        it.second->second = newValue;
    }

    void set_in_parent(InterpretScope &scope, Value *newValue, Operation op) override {
        auto it = find(scope);
        if (!it.first) {
            std::cerr << "Couldn't set variable " << value
                      << " as there's no such variable in parent, or previous value doesn't exist ";
            return;
        }
        auto v = it.second->second;

        // creating an expression
        auto nextValue = ExpressionEvaluator::functionVector[
                ExpressionEvaluator::index(v->value_type(), v->value_type(), op)
        ](v, newValue);

        if (v->delete_value()) {
            delete v;
        }
        it.second->second = nextValue;
    }

    std::pair<bool, std::unordered_map<std::string, Value *>::iterator> find(InterpretScope &scope) {
        // try to find the pointer of the value
        auto currentScope = &scope;
        while (currentScope != nullptr) {
            auto pointer = currentScope->values.find(value);
            if (pointer != currentScope->values.end()) {
                return std::pair(true, std::move(pointer));
            }
            currentScope = currentScope->parent;
        }
        return std::pair(false, scope.values.end());
    }

    Value *evaluated_value(InterpretScope &scope) override {
        auto found = find(scope);
        if (found.first) {
            return found.second->second;
        } else {
            return nullptr;
        }
    }

    Value *find_in_parent(InterpretScope &scope) override {
        auto found = find(scope);
        if (found.first) {
            return found.second->second;
        } else {
            return nullptr;
        }
    }

    Value *travel(InterpretScope &scope) override {
        return find_in_parent(scope);
    }

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.

};