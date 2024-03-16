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

    CharOperatorToken(const TokenPosition& position, char op) : LexToken(position), op(op) {

    }

    unsigned int length() const override {
        return 1;
    }

    LexTokenType type() const override {
        return LexTokenType::CharOperator;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_operator;
    }
#endif

    std::string representation() const override {
        std::string ret;
        ret.append(1, op);
        return ret;
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