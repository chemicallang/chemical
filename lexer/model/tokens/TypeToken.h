// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class TypeToken : public LexToken {
public:

    std::string value;

    TypeToken(const Position& position, std::string type) : LexToken(position), value(std::move(type)) {
        value.shrink_to_fit();
    }

    unsigned int length() const override {
        return value.length();
    }

    LexTokenType type() const override {
        return LexTokenType::Type;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_type;
    }
#endif

    std::string representation() const override {
        return this->value;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Type:");
        buf.append(value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return value;
    }

};