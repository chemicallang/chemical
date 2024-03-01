// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class WhileLoop : public ASTNode {
public:

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(std::unique_ptr<Value> condition, Scope body)
            : condition(std::move(condition)), body(std::move(body)) {}

    std::string representation() const override {
        std::string ret;
        ret.append("while(");
        ret.append(condition->representation());
        ret.append(") {\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

private:
    std::unique_ptr<Value> condition;
    Scope body;
};