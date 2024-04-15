// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class RawToken : public AbstractStringToken {
public:

    RawToken(const Position& position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    LexTokenType type() const override {
        return LexTokenType::RawToken;
    }

    unsigned int length() const override {
        return value.length();
    }

    [[nodiscard]] std::string type_string() const override {
        return "Raw:" + value;
    }

    [[nodiscard]] std::string content() const override {
        return value;
    }

};