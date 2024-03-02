// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"

class ArrayValue : public Value {

public:

    ArrayValue(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    ArrayValue(ArrayValue&& other) : values(std::move(other.values)) {

    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '[');
        int i = 0;
        while (i < values.size()) {
            rep.append(values[i]->representation());
            if (i != values.size() - 1) {
                rep.append(1, ',');
            }
            i++;
        }
        rep.append(1, ']');
        return rep;
    }

    std::vector<std::unique_ptr<Value>> values;

};