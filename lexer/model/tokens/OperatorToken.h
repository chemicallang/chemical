// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

#include <utility>

class CharOperatorToken : public LexToken {
public:

    char op;

    CharOperatorToken(unsigned int start, unsigned int lineNumber, char op) : LexToken(start, lineNumber), op(op) {

    }

    unsigned int length() const override {
        return 1;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
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