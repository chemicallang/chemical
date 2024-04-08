// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class StringToken : public LexToken {
public:

    std::string value;

    StringToken(const Position& position, std::string value) : LexToken(position), value(std::move(value)) {
        value.shrink_to_fit();
    }

    unsigned int length() const override {
        // 2 is added bacause the token starts at quote, then length of the string, another quote
        return value.length() + 2;
    }

    LexTokenType type() const override {
        return LexTokenType::String;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_string;
    }
#endif

    void append_representation(std::string &rep) const override {
        rep.append(1, '"');
        rep.append(value);
        rep.append(1, '"');
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("String:");
        buf.append(value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return value;
    }

};