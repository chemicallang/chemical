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
class OperationToken : public LexToken {
public:

    Operation op;
    unsigned int len;

    OperationToken(const TokenPosition& position, unsigned int length, Operation op) : LexToken(position), len(length), op(op) {

    }

    unsigned int length() const override {
        return len;
    }

    LexTokenType type() const override {
        return LexTokenType::Operation;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_operator;
    }

    std::string representation() const override {
        std::string ret;
        ret.append(to_string(op));
        return ret;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string ret;
        ret.append("Operator:");
        ret.append(to_string(op));
        return ret;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};