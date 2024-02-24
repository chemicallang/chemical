// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class IntToken : public LexToken {
public:

    int value;

    IntToken(unsigned int start, unsigned int length, unsigned int lineNumber, int value) : LexToken(start, length, lineNumber), value(value) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_number;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Int:");
        buf.append(std::to_string(value));
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return std::to_string(value);
    }

};