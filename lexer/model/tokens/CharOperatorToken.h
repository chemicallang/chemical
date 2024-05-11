// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

/**
 * Its named CharOperator because it holds a char, Char has no meaning in terms of syntax
 * The length of this token is always one
 */
class CharOperatorToken : public AbstractStringToken {
public:

    CharOperatorToken(const Position& position, std::string ope) : AbstractStringToken(position, std::move(ope)) {

    }

    LexTokenType type() const override {
        return LexTokenType::CharOperator;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]] std::string type_string() const override {
        return "Operator:" + value;
    }

};