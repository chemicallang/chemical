// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

class IndexOperator : public Value {
public:

    IndexOperator(std::unique_ptr<Value> value) : value(std::move(value)) {

    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '[');
        rep.append(value->representation());
        rep.append(1, ']');
        return rep;
    }

    std::unique_ptr<Value> value;

};