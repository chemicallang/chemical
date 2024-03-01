// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"

class IncDecValue : public Value {
public:
    /**
     * @brief Construct a new Expression object.
     *
     * @param firstValue The first value in the expression.
     * @param secondValue The second value in the expression.
     * @param operation The operation between the two values.
     */
    IncDecValue(
            std::unique_ptr<Value> value,
            bool increment
    ) : value(std::move(value)), increment(increment) {

    }

    std::string representation() const override {
        std::string rep;
        rep.append(value->representation());
        if(increment) {
            rep.append("++");
        } else {
            rep.append("--");
        }
        rep.append(1, ')');
        return rep;
    }


private:
    std::unique_ptr<Value> value; ///< The first value in the expression.
    bool increment;
};