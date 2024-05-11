// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class KeywordToken : public AbstractStringToken {
public:

    KeywordToken(const Position& position, std::string keyword) : AbstractStringToken(position, std::move(keyword)) {
        keyword.shrink_to_fit();
    }

    LexTokenType type() const override {
        return LexTokenType::Keyword;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Keyword:");
        buf.append(this->value);
        return buf;
    }

};