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
        op.shrink_to_fit();
    }

    unsigned int length() const override {
        return op.size();
    }

    LexTokenType type() const override {
        return LexTokenType::StringOperator;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_operator;
    }
#endif

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