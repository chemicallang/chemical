// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CharToken : public AbstractStringToken {
public:

    CharToken(const Position& position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Char;
    }

    [[nodiscard]] std::string type_string() const override {
        return "Char:" + value;
    }

};