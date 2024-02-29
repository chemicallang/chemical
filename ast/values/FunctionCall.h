// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"

class FunctionCall : public Value {

public:

    FunctionCall(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    FunctionCall(FunctionCall&& other) : values(std::move(other.values)) {

    }

    std::vector<std::unique_ptr<Value>> values;

};