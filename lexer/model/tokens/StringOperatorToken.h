// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#pragma once

#include "LexToken.h"

/**
 * Its named StringOperator because it holds a string, String has no meaning in terms of syntax
 * The length of this token is always one
 */
class StringOperatorToken : public AbstractStringToken {
public:

    StringOperatorToken(const Position& position, std::string op) : AbstractStringToken(position, std::move(op)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::StringOperator;
    }

    [[nodiscard]] std::string type_string() const override {
        return "Operator:" + value;
    }

};