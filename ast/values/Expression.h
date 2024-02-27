// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/utils/Operation.h"

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
        const Value &firstValue,
        const Value &secondValue,
        Operation operation
    ) : firstValue(firstValue), secondValue(secondValue), operation(operation) {

    }

private:
    Value firstValue; ///< The first value in the expression.
    Value secondValue; ///< The second value in the expression.
    Operation operation; ///< The operation between the two values.
};