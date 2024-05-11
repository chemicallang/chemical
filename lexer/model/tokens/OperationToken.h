// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"
#include "ast/utils/Operation.h"

/**
 * Its named OperationToken because it holds a operation
 */
class OperationToken : public AbstractStringToken {
public:

    OperationToken(const Position& position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Operation;
    }

    [[nodiscard]] std::string type_string() const override {
        return "Operation:" + value;
    }

};