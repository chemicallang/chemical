// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
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

    std::string representation() const override {
        std::string rep;
        rep.append(1, '(');
        rep.append(firstValue->representation());
        rep.append(std::to_string((int) operation));
        rep.append(secondValue->representation());
        rep.append(1, ')');
        return rep;
    }


private:
    std::unique_ptr<Value> firstValue; ///< The first value in the expression.
    std::unique_ptr<Value> secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
};