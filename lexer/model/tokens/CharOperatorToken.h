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

    char op;

    CharOperatorToken(const Position& position, char op) : LexToken(position), op(op) {

    }

    unsigned int length() const override {
        return 1;
    }

    LexTokenType type() const override {
        return LexTokenType::CharOperator;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    void append_representation(std::string &rep) const override {
        rep.append(1, op);
    }

    [[nodiscard]] std::string type_string() const override {
        std::string ret;
        ret.append("Operator:");
        ret.append(1, op);
        return ret;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};