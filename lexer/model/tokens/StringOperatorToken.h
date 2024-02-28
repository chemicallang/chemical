// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#pragma once

#include "LexToken.h"

/**
 * Its named CharOperator because it holds a char, Char has no meaning in terms of syntax
 * The length of this token is always one
 */
class StringOperatorToken : public LexToken {
public:

    std::string op;

    StringOperatorToken(const TokenPosition& position, std::string op) : LexToken(position), op(std::move(op)) {

    }

    unsigned int length() const override {
        return op.size();
    }

    LexTokenType type() const override {
        return LexTokenType::StringOperator;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
    }

    std::string representation() const override {
        return op;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string ret;
        ret.append("Operator:");
        ret.append(op);
        return ret;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};