// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class WhitespaceToken : public LexToken {
public:

    unsigned int len;

    WhitespaceToken(const Position& position, unsigned int length) : LexToken(position), len(length) {

    }

    LexTokenType type() const override {
        return LexTokenType::Whitespace;
    }

    unsigned int length() const override {
        return len;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_operator;
    }
#endif

    std::string representation() const override{
        std::string space;
        for(int i = 0; i< this->len;i ++) {
            space += ' ';
        }
        return space;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Whitespace:");
        buf.append(std::to_string(this->len));
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};