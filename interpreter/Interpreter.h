// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include <memory>
#include <vector>

class Interpreter {

    std::vector<std::unique_ptr<ASTNode>> tokens;

    explicit Interpreter(std::vector<std::unique_ptr<ASTNode>> tokens) : tokens(std::move(tokens)) {

    }

};