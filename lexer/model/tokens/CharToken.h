// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CharToken : public LexToken {
public:

    char value;

    unsigned int len;

    CharToken(const TokenPosition& position, char value, unsigned int length) : LexToken(position), value(value), len(length) {

    }

    unsigned int length() const override {
        return len;
    }

    LexTokenType type() const override {
        return LexTokenType::Char;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_string;
    }

    std::string representation() const override {
        std::string buf;
        buf.append(1, '\'');
        buf.append(1,value);
        buf.append(1, '\'');
        return buf;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Char:");
        buf.append(1, value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        std::string buf;
        buf.append(1, value);
        return buf;
    }

};