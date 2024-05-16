// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

/**
 * Its named CharOperator because it holds a char, Char has no meaning in terms of syntax
 * The length of this token is always one
 */
class CharOperatorToken : public LexToken {
public:

    CharOperatorToken(const Position& position, std::string ope) : LexToken(position, std::move(ope)) {

    }

    LexTokenType type() const override {
        return LexTokenType::CharOperator;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitCharOperatorToken(this);
    }

};