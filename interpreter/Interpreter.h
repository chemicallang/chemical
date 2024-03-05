// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/utils/ExpressionEvaluator.h"
#include <memory>
#include <vector>

class Interpreter : public ExpressionEvaluator {
public:

    void interpret() {

    }

    void error(const std::string& err) {
        errors.emplace_back(err);
    }

    /**
     * The global interpret scope
     */
     InterpretScope global;

     /**
      * The local interpret scope
      */
     InterpretScope local;

    /**
     * This contains errors that occur during interpretation
     */
    std::vector<std::string> errors;

};