// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

class AccessChain : public Value {

public:

    AccessChain(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

    }

    std::vector<std::unique_ptr<Value>> values;

};