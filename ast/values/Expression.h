// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include "parser/utils/Operation.h"
#include "ast/utils/ExpressionEvaluator.h"
#include <memory>

class Expression : public Value {
public:

    /**
     * @brief Construct a new Expression object.
     *
     * @param firstValue The first value in the expression.
     * @param secondValue The second value in the expression.
     * @param operation The operation between the two values.
     */
    Expression(
            std::unique_ptr<Value> firstValue,
            std::unique_ptr<Value> secondValue,
            Operation operation
    ) : firstValue(std::move(firstValue)), secondValue(std::move(secondValue)), operation(operation) {

    }

    /**
     * evaluates both values and returns the result as unique_tr to Value
     * @return
     */
    inline std::unique_ptr<Value> evaluate(scope_vars scopeVars) {
        auto fEvl = firstValue->evaluated_value(scopeVars);
        auto sEvl = secondValue->evaluated_value(scopeVars);
        auto index = ((uint8_t) fEvl->value_type() << 20) | ((uint8_t) sEvl->value_type() << 10) | (uint8_t) operation;
        if (ExpressionEvaluator::functionVector.contains(index)) {
            return ExpressionEvaluator::functionVector[index](fEvl, sEvl);
        } else {
            throw std::runtime_error(
                    "Cannot evaluate expression as the method with index " + std::to_string(index) + " does not exist");
        }
    }

    Value *evaluated_value(std::unordered_map<std::string, Value *> &scopeVars) override {
        return evaluate(scopeVars).release();
    }

    /**
     * evaluates the current expression and also interprets the evaluated value
     * @param scope
     */
    void interpret(InterpretScope &scope) override {
        evaluate(scope.values)->interpret(scope);
    }

    /**
     * This returns a unique value, combining type of values given in the expression
     * This unique value can be used as a method caller
     * @return
     */
    inline int method_index() {
        return ((uint8_t) firstValue->value_type() << 20) | ((uint8_t) secondValue->value_type() << 10) |
               (uint8_t) operation;
    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '(');
        rep.append(firstValue->representation());
        rep.append(to_string(operation));
        rep.append(secondValue->representation());
        rep.append(1, ')');
        return rep;
    }


private:
    std::unique_ptr<Value> firstValue; ///< The first value in the expression.
    std::unique_ptr<Value> secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
};