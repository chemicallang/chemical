// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class WhitespaceToken : public LexToken {
public:

    unsigned int len;

    WhitespaceToken(const TokenPosition& position, unsigned int length) : LexToken(position), len(length) {

    }

    unsigned int length() const override {
        return len;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
    }

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